PL_CFLAGS = -I../$(SYSTEM) -I../$(UI) -I../$(WEB_SERVER) -I../$(HAL) -I../. 
PL_CPPFLAGS = -lpthread -lm -lrt

CPPLIBS = -lpthread -lm -lrt -lseccomp

SYSTEM = system
UI = ui
WEB_SERVER = web_server
HAL = hal

CC = gcc
CPP = g++

HEADERS =  ../$(SYSTEM)/system_server.h\
		../$(SYSTEM)/shared_memory.h\
		../$(UI)/gui.h\
		../$(UI)/input.h\
		../$(WEB_SERVER)/web_server.h\
		../$(HAL)/oem/camera_HAL_oem.h\
		../$(HAL)/oem/ControlThread.h\
		../$(HAL)/toy/camera_HAL_toy.h\
		../$(HAL)/toy/ControlThread.h\
		../$(HAL)/hardware.h\
		../toy_message.h\
		

CPPINCLUDES = -l${SYSTEM} -l${UI} -l${WEB_SERVER} -l${HAL}		

CFLAGS = ${PL_CFLAGS} -g -o
CPPFLAGS = ${CPPINCLUDESDIRS} -g -O0 -std=c++14

RM = rm -f
