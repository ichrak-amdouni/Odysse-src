#! /usr/bin/python
#---------------------------------------------------------------------------
#  . Cedric Adjih,   Infine Project-Team, Inria Saclay
#  Copyright 2015 Institut national de recherche en informatique et
#  en automatique.  All rights reserved.  Distributed only with permission.
#---------------------------------------------------------------------------

#HOW-TO-RUN: sage -python LDPCDecoder.py
from sage.all import * # ZZ, GF, Matrix
#from sage.matrix.matrix2 import Matrix
from sage.misc.latex import MathJax
#import random as randomModule
import warnings
NB_SOURCE_PACKET = 100
#MAX_UNDECODED_PACKET_NB = 55

from coefSetOfSeqNum import * 
 


#----------------------------------
def xorPacket(p1, p2):
    if p1 == None:
        return p2
    if p2 == None:
        return p1
    return "".join(( chr(ord(p1[i]).__xor__(ord(p2[i]))) for i in range(len(p1)) ))

#----------------------------------
# Decode packets

dbg = False

class LDPCDecoder:
    def __init__(self, receivedPacketList):
        self.receivedPacketList = receivedPacketList
        self.coefSetOfSeqNum = coefSetOfSeqNum
        self.K = GF(2)
        
    def getEncodingMatrix(self, realNbSourcePacket, verbose):
        print("realNbSourcePacket: %s" %realNbSourcePacket)
        #PacketSpace = VectorSpace(K,8*100)
        
        # need to compute a bound of n from lastSeqNum (could divide by coding rate) XXX:do better
        #nBound = receivedPacketList[-1][0]
        nBound = realNbSourcePacket ###  NB_SOURCE_PACKET
        #coefSpace = VectorSpace(K,nBound)
        
        # Compute current matrix based on received seqnum and known associated coefficients
        coefVectorList = []
        if realNbSourcePacket == None:
            allCoefSet = set()
        else:
            allCoefSet = set(range(1, realNbSourcePacket+1))
        for j,(seqNum, t, packet) in enumerate(self.receivedPacketList):
            ###coef = self.coefSetOfSeqNum[seqNum]
            
            coef = set([ x for x in coefSetOfSeqNum[seqNum] 
                        if realNbSourcePacket == None or x <= realNbSourcePacket ])
            
            #if verbose: print coef
            ####allCoefSet.update(coef)
            extraCoefVector = [0] * len(self.receivedPacketList)
            extraCoefVector[j] = 1
            coefVector = [0]*(nBound+1) #+ extraCoefVector ### XXX: check (+1)!
            for i in coef:
                if 0 < i <= realNbSourcePacket: ###self.getNbPacketSent(): #MAX_UNDECODED_PACKET_NB:
                    coefVector[i] = 1
            
            coefVectorList.append(coefVector)
            
        m = Matrix(self.K, coefVectorList)
        #print m
        # See which packets are coded
        uncodedSet = set()
        for v in Matrix(ZZ, m): # need to convert 'm' so that v is vector of integers (ZZ), not vector of GF(2)
            vl = list(v)
            if sum(vl) == 1:
                uncodedSet.add(vl.index(1))
        
        if verbose:
            print "rank:", m.rank()
            print "%s uncoded+coded packets" % len(self.receivedPacketList)
            print "%s uncoded source packets:" % len(uncodedSet), uncodedSet
            print "%s missing source packets:" % (len(allCoefSet)-len(uncodedSet)), allCoefSet.difference(uncodedSet)
            
        return m, allCoefSet, uncodedSet


    
    def getNbPacketSent(self):
        seqNum = [seqNum for (seqNum, t, p) in self.receivedPacketList]
        maxSeqNum = max(seqNum)
        if (maxSeqNum > 0x256):
            x = maxSeqNum/0x256
            #print ("UNDECODED :%d "%x)
            return maxSeqNum/0x256
        return maxSeqNum
    
    def decode(self, allCoefSet, m, verbose):
        mRref = m.rref()
        solvingMatrix = m.solve_left(mRref) # -> solvingMatrix * m = mRref
        #if dbg: print solvingMatrix
        
        decodedPacketList = []
        receivedPacketTable = {i: (t, p) for (i, t, p) in self.receivedPacketList}
        # Decode based on solvingMatrix
        for i,v in enumerate(Matrix(ZZ, mRref)):
            if sum(v) != 1:
                continue # cannot get output vector
            sourcePacketIndex = list(v).index(1)
            packet = None #getZeroPacket()
            if dbg: print "S%s=" % sourcePacketIndex,
            for j, e in enumerate(solvingMatrix[i]):
                if e != 0:
                    if dbg: print "+Q%s"%j,
                    #packetTime = self.receivedPacketList[j][1]  
                    packet = xorPacket(packet, self.receivedPacketList[j][-1]) 
            if dbg: print
            #receivedPacketTable[seqNUm] = (time, payload)
            
            if sourcePacketIndex in receivedPacketTable:
                packetTime = receivedPacketTable[sourcePacketIndex][0]
            else:
                for sign in [+1, -1]:
                    for delta in range(1, max(receivedPacketTable.keys())):
                        warnings.warn("XXX:change bound!")
                        # QED
                        otherPacketIndex = sourcePacketIndex+sign*delta
                        if otherPacketIndex in receivedPacketTable:
                            packetTime = receivedPacketTable[otherPacketIndex][0]
                            break
            decodedPacketList.append((sourcePacketIndex, packetTime, packet))
        
        # Verify 
        #for i,p in decodedPacketTable.iteritems():
            #print sourcePacketList[i]
            #print decodedPacketTable[i]
            #if dbg: print i, repr(p)
            #assert p == sourcePacketList[i] #!!!!
            
        # Statistics:
        decodedSet = set(i for (i, t, j) in decodedPacketList) #set(decodedPacketTable.keys())
        if verbose:
            print "%s uncoded+coded packets" % len(self.receivedPacketList)
            print "%s decoded source packets:" % len(decodedSet), decodedSet
            print "%s unrecovered source packets:" % (len(allCoefSet)-len(decodedSet)), allCoefSet.difference(decodedSet)
        return decodedPacketList #decodedSet

    def process(self):
        print "--- computing matrix"
        realNbSourcePacket = self.getNbPacketSent()
        m, allCoefSet, uncodedSet = self.getEncodingMatrix(realNbSourcePacket, verbose=True)
        print "--- decoding"
        decodedPacketTable = self.decode(allCoefSet, m, verbose=True)
        return decodedPacketTable
if __name__ == "__main__":
    allPacketList = []
    
    Decoder = LDPCDecoder(allPacketList)
    Decoder.process()