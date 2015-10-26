#! /usr/bin/python
#---------------------------------------------------------------------------
#  . Ichrak Amdouni,   Infine Project-Team, Inria Saclay
#  Copyright 2015 Institut national de recherche en informatique et
#  en automatique.  All rights reserved.  Distributed only with permission.
#---------------------------------------------------------------------------
#echo 'get-rcv-photo' | nc localhost 3331

import sys, time
dbg = False
import threading
import socket
import collections
from SinkAddr import *
from PIL import Image
import binascii
from PIL import ImageFile
import struct
#ImageFile.LOAD_TRUNCATED_IMAGES = True # hack
import pprint
from commun import *
END = False
from LDPCDecoderNew import *
import subprocess, signal
 
class Photo(threading.Thread):
	
    def __init__(self):
        self.photo = {}
        self.packetSet = set()
        self.photoNb = {}
        self.multiplePhoto = 1
        self.initTimer()
        self.AllpacketSet = {}	
        self.fileTable = {}
        self.collectedPhotoSeqNum = []
        self.infoPerSeq = {}
        
    def getData(self, cmd, port):
        Port = port
        Host = ""
        sd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sd.connect((Host, Port))
        sd.send(cmd)
        data = b""
        while True:
            newData = sd.recv(1024)
            if len(newData) == 0:
                break
            data += newData
        sd.close()
        return data


    def requestFromSocket(self, request):
        #rawData = self.getData(request, 3331)
        print ("Send Command %s" %request)
        try: 
            rawData = self.getData(request, SinkPort)
        except: 
            exc_type, exc_value, exc_traceback = sys.exc_info()
            print ("[%s] get data error: %s" % (time.time(), exc_type))
            #print(exc_traceback)
            return None
        return eval(rawData)


    def eventTimer(self):
        allData = self.requestFromSocket(b"get-rcv-photo\n")
        data = allData
        end = False
        if data:
            end = self.buildPhoto(data)
            if end:
                print "Done."
                if self.multiplePhoto:
                    threading.Timer(2.0, self.eventTimer).start()
                else:
                    threading.Timer(2.0, self.eventTimer).cancel()
            else:
                threading.Timer(2.0, self.eventTimer).start()
        else:
            if not end:
                threading.Timer(2.0, self.eventTimer).start()
        
    def initTimer(self):
        threading.Timer(2.0, self.eventTimer).start()
        #self.eventTimer()
		
    def getPathLengthFromData(self, payload):
        i = DATA_HEADER_SIZE - 1
        d = payload[i*3+1:(i*3)+3]
        x = int(d, 16)
        return x
    
    def buildPhoto(self, data):
        headerSize = DATA_HEADER_SIZE 
        #print(data), 
        #print len(data)
        for (photoSeqNum, dataMsgSeqNum, t), info in data.iteritems():
            
            payload = str(info['payload'])
            pathLength = self.getPathLengthFromData(payload)
            headerSizePlusPath = headerSize + pathLength*2
           
            hexStr = payload.replace("x","") 
            rawPayload = "".join([chr(int(hexStr[i:i+2],16)) 
                                    for i in range(0, len(hexStr), 2)])
            #rawPayload = binascii.a2b_hex(hexStr)
            (hMessageType, hMessageSize, hSenderAddress,
             hSeqNum, hPhotoNum) = struct.unpack(">BBHHH", rawPayload[0:8])

            #if not (hSeqNum >= 800):
            reallyPayload = payload[(3*headerSizePlusPath):]
            if pathLength != 0:
                ad = payload[27:33]
                ad2 = ad.replace('x','')
                s = ad2.decode("hex")
                srcAddr = struct.unpack('>H', s)[0]
            else:
                ad = payload[6:12]
                ad2 = ad.replace('x','')
                s = ad2.decode("hex")
                srcAddr = struct.unpack('>H', s)[0]
            if not self.photoNb.has_key(srcAddr):
                #if source not in self.photoNb:
                self.photoNb[srcAddr] = 1
                
            if self.AllpacketSet.has_key(srcAddr):
                self.AllpacketSet[srcAddr].add((photoSeqNum, dataMsgSeqNum, t, reallyPayload))
            else:
                self.AllpacketSet[srcAddr] = set([(photoSeqNum, dataMsgSeqNum, t, reallyPayload)])                             
        end = self.checkEndPhotoPerSrcNew()
        return end
        
    def writePackets(self, packetList):
        receivedPacketList = []
        for (seqNum, t, p) in packetList:
            p = p.replace('x', '')
            p = "".join([chr(int(p[i:i+2],16)) 
                                    for i in range(0, len(p), 2)])
            receivedPacketList.append((seqNum, t, p))
        #pprint.pprint(receivedPacketList)
        return receivedPacketList
    
    def isPhotoHere(self, receivedPacketList):
        i = len(receivedPacketList) - 1;
        while(i > 0):
            lastPayload = receivedPacketList[i][2]
            index = lastPayload.find("\xFF\xD9")
            if index >= 0:
                return i
            i = i - 1
            
    
    def checkEndPhotoPerSrcNew(self):
        receivedPacketList = []
        decodedPacketTable = []
        withRefresh = True
        if len(self.AllpacketSet) == 0:
            return
        for source, listSource in self.AllpacketSet.iteritems():
            z = {}
            for info in listSource:
                (photoSeqNum, dataMsgSeqNum, t, reallyPayload) = info
                if photoSeqNum not in z:
                    z[photoSeqNum] = []
                z[photoSeqNum].append((dataMsgSeqNum, t, reallyPayload))
            if source not in self.infoPerSeq:
                self.infoPerSeq[source] = []                    
            self.infoPerSeq[source].append(z)
            
        for source in self.infoPerSeq:    
            for x in self.infoPerSeq[source]:
                for photoSeqNum, packetInfo in x.iteritems():
                    listSource = packetInfo
                
                    if photoSeqNum in self.collectedPhotoSeqNum:
                        print ("Photo nb %s already here" %photoSeqNum) 
                        continue
                    print("\n**** Managing photo nb %s from source %s" %(photoSeqNum, source))
                    
                    l = sorted(listSource, key=lambda tup: tup[0])
                    receivedPacketList = self.writePackets(l)
                    decoder = LDPCDecoder(receivedPacketList)
                    decodedPacketList = decoder.process()
                    #pprint.pprint(decodedPacketTable)
                    l = decodedPacketList
                    #pprint.pprint(l)
                    l2 = [ (i, "".join( ("x"+ ("%02x" % ord(x)).upper() for x in p) ) ) for i, t, p in l ]
                    #pprint.pprint(l2)
                    
                    #lastPayload = l[47][2]
                    lastPayloadIndex = self.isPhotoHere(l)
                    allSeqNum = [x[0] for x in l]
                    
                    if (lastPayloadIndex >= 0):
                        fileName = "ard-photo-%s-%s.jpg" %(photoSeqNum, source)
                        ffileName = "ard-photo-%s-%s.hex" %(photoSeqNum, source)
                        self.f = open(fileName, "w")
                        self.ff = open(ffileName, "w")
                        
                        print "Image Number %s From Source %s: " %(photoSeqNum, source)
                        allSeqNum = [x[0] for x in l]
                        #assert(allSeqNum == range(1, max(allSeqNum)+1))
                        dataSize = 0
                        l = l[:lastPayloadIndex+1]
                        #pprint.pprint(l)
                        for idpacket, t, payload in l:
                            i = payload.find("\xFF\xD9")  # last payload
                            if i > 0:
                                part = payload[:(i+2)]
                            else:
                                part = payload
                            #part = part.replace("x", '')
                            dataSize = dataSize + len(part)
                            self.f.write(part)
                            self.ff.write(part)
                        self.f.close()
                        try:
                            image = Image.open(fileName)
                        except:
                            print("ERROR on Image Number %s From Source %s: (Received %s packets)" %(photoSeqNum, source, len(l)))
                            #self.collectedPhotoSeqNum.append(photoSeqNum)
                            #self.initAll(source)
                            continue
                        
                        #try:
                        #os.system("xdg-open "+fileName+"&")
                        
                        self.refreshImage(fileName)
                        #image.show()
                        #except:
                        #    print("ERROR showing image!")
                        deltaTime = l[len(l) -1][1] - l[0][1]
                        if(allSeqNum == range(1, max(allSeqNum)+1)):# otherwise some pckts are lost
                            print("All packets are here (#%s)!" %len(l))
                        else:
                            missing = max(allSeqNum) - len(allSeqNum)
                            print("End Photo (%s packets) but Missing %d packets!" %(len(l), missing))
                        withRefresh = False
                        if not withRefresh:
                            self.collectedPhotoSeqNum.append(photoSeqNum)
                            self.initAll(source)
                        print ("Photo Size = %d octets, Delta time = %f minutes." %(dataSize, deltaTime/60.0))
                    
                        self.ff.close()
                        continue # return True
                    else:
                        print "Image not yet here, %d packets collected" % len(l)
                        
                        continue # return False
                    return False
        	
    def initAll(self, source):
        self.photoNb[source] = self.photoNb[source] + 1
        #self.packetSet = set()
        self.AllpacketSet = {}

    def refreshImage(self, fileName):
        p = subprocess.Popen(['ps', '-A'], stdout=subprocess.PIPE)
        out, err = p.communicate()
        for line in out.splitlines():
            if "xdg-open" in line and fileName in line:
                pid = int(line.split(None, 1)[0])
                
                try:
                    while 1:
                        os.kill(pid, SIGTERM)
                        time.sleep(1.0)
                except OSError, err:
                    err = str(err)
                    if err.find("No such process") > 0:
                        pass
                    else:
                        print str(err)
                        pass 
                
                #while not (os.kill(pid, signal.SIGKILL)):
                #    print(".")
              
        os.system("xdg-open "+fileName+"&")
        
if __name__ == "__main__":
    photoConstructor = Photo()


#---------------------------------------------------------------------------
