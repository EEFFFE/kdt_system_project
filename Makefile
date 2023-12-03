include Makefile.inc

DIRS = system \ ui \ web_server \ hal
BUILD_DIRS = ${DIRS}

SYSTEM = ./system

TARGET = toy_system
CC = gcc
CPP = g++


FILENAME = main
MAINo = $(FILENAME).o
MAINc = $(FILENAME).c

MAINh = $(SYSTEM)/system_server.h $(UI)/gui.h $(UI)/input.h $(WEB_SERVER)/web_server.h ./toy_message.h
MAINf = -I$(SYSTEM) -I$(UI) -I$(WEB_SERVER) -I./ -c -g -o

DIROBJ = ./system/system_server.o ./ui/gui.o ./ui/input.o ./web_server/web_server.o
HALOBJ = ./hal/camera_HAL.o ./hal/ControlThread.o

$(TARGET): $(MAINo) 
	@ echo ${BUILD_DIRS}
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE}); \
		if test $$? -ne 0; then break; fi; done;	
	$(CPP) -o $(TARGET) $(MAINo) $(DIROBJ) $(HALOBJ) $(CPPLIBS) 

$(MAINo): $(MAINh) $(MAINc)
	$(CC) $(MAINf) $(MAINo) $(MAINc)

allgen:
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE} allgen); done


clean:
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE} clean); done
	rm -f $(TARGET) *.o

