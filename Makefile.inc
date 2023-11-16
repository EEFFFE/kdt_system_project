IMPL_CFLAGS = -I../$(SYSTEM) -I../$(UI) -I../$(WEB_SERVER) -g -o

SYSTEM = system
UI = ui
WEB_SERVER = web_server

CC = gcc

HEADERS =  ../$(SYSTEM)/system_server.h\
		../$(UI)/gui.h\
		../$(UI)/input.h\
		../$(WEB_SERVER)/web_server.h

CFLAGS = ${IMPL_CFLAGS}

RM = rm -f
