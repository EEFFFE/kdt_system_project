PL_CFLAGS = -I../$(SYSTEM) -I../$(UI) -I../$(WEB_SERVER) -I../$(HAL)  -g -o
PL_CPPFLAGS = -lpthread -lm -lrt

CPPLIBS = -lpthread -lm -lrt

SYSTEM = system
UI = ui
WEB_SERVER = web_server
HAL = hal

CC = gcc
CPP = g++

HEADERS =  ../$(SYSTEM)/system_server.h\
		../$(UI)/gui.h\
		../$(UI)/input.h\
		../$(WEB_SERVER)/web_server.h\
		../$(HAL)/ControlThread.h\
		../$(HAL)/camera_HAL.h\

CPPINCLUDES = -l${SYSTEM} -l${UI} -l${WEB_SERVER} -l${HAL}		

CFLAGS = ${PL_CFLAGS}
CPPFLAGS = ${CPPINCLUDESDIRS} -g -O0 -std=c++14

RM = rm -f
