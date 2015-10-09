#---------------------------------------------------------------------------
# Main makefile
#---------------------------------------------------------------------------

INO_OPT=--arduino-dist /home/amdouni/ArduinoTestbed-1.6.3/arduino-1.6.3
MODEL_ARG=  -m mega2560
all:
	@echo "please read Makefile"

really-clean:
	rm -rf BUILD-*

#---------------------------------------------------------------------------
# External libraries
#---------------------------------------------------------------------------

external-lib: EXT-LIB/xbee-arduino EXT-LIB/SimpleTimer \
              EXT-TOOL/python-xbee EXT-LIB/LSY201

EXT-TOOL/python-xbee:
	make EXT-TOOL
	cd EXT-TOOL && hg clone https://code.google.com/p/python-xbee/

EXT-LIB/xbee-arduino:
	make EXT-LIB
	cd EXT-LIB && git clone https://code.google.com/p/xbee-arduino/

EXT-LIB/SimpleTimer:
	make EXT-LIB
	cd EXT-LIB && git clone https://github.com/infomaniac50/SimpleTimer

EXT-LIB/LSY201:
	make EXT-LIB
	cd EXT-LIB && git clone https://github.com/chendry/LSY201
EXT-LIB:
	mkdir EXT-LIB

EXT-TOOL:
	mkdir EXT-TOOL

#---------------------------------------------------------------------------
# TOOL-sniffer
#---------------------------------------------------------------------------

#TOOL-sniffer:
#	mkdir TOOL-sniffer-tmp
#	cd TOOL-sniffer-tmp \
#  && git clone https://gforge.inria.fr/git/contiki-hiper/contiki-hiper.git \
#     -b sniffer contiki-sniffer
#	mv TOOL-sniffer-tmp TOOL-sniffer

#---------------------------------------------------------------------------
# Automatic ino sketch building directory
#---------------------------------------------------------------------------

BUILD-%:
	make external-lib #TOOL-sniffer
	@test -e src/$*.ino || { echo "ERROR: No sketch src/$*.ino" ; exit 1 ;} 
	mkdir $@ && cd $@ && ino init
	for i in EXT-LIB/* ; do (cd $@/lib && ln -s ../../$$i) ; done
	for i in src/*.h src/*.cpp src/*.c; do (cd $@/src && ln -s ../../$$i) ; done
	rm -f $@/src/sketch.ino
	ln -s ../../src/$*.ino $@/src

all-build-dir: BUILD-router BUILD-sink BUILD-source

#---------------------------------------------------------------------------
# Build and flash
#---------------------------------------------------------------------------

compile-%:
	make BUILD-$*
	cd BUILD-$* && ino build ${INO_OPT} ${MODEL_ARG}

flash-%:
	make BUILD-$*
	make compile-$* "MODEL_ARG=${MODEL_ARG}"
	fuser -k $$(echo ${PORT_ARG} | sed 's/-p//g') || true
	cd BUILD-$* && ino upload ${INO_OPT} ${MODEL_ARG} ${PORT_ARG}
	@echo "=== end upload ==="

#---------------------------------------------------------------------------
# Identifiers in EEPROM memory
#---------------------------------------------------------------------------

NODE_ID=please.define.NODE_ID

flash-id: BUILD-write-id
	(cd BUILD-write-id \
         && ino clean \
         && ino build ${MODEL_ARG} ${PORT_ARG} \
                      CXX_ARG='--cxxflags="-DNODE_ID=${NODE_ID}"' \
         && ino upload ${MODEL_ARG} ${PORT_ARG})

flash-id-uno:
	make flash-id NODE_ID=${NODE_ID} PORT_ARG=${PORT_ARG}

flash-id-mega:
	make flash-id NODE_ID=${NODE_ID} MODEL_ARG="-m atmega2560" \
             PORT_ARG=${PORT_ARG}

#---------------------------------------------------------------------------
#for LOG 
# python3 ArduinoRun.py --log --display --speed 9600:38400 /dev/ttyACM2=mega04 /dev/ttyACM3=sender /dev/ttyACM4=ttyACM4 /dev/ttyACM1=mega01 /dev/ttyACM0=ttyACM0
