include ../Makefile.inc

GEN_EXE = system_server.o
SRC = system_server.c

EXE = $(GEN_EXE) $(ETC_EXE)

all: $(EXE)

$(EXE): $(HEADERS) $(SRC)
	$(CC) -c $(CFLAGS) $(GEN_EXE) $(SRC)


allgen : ${GEN_EXE}


clean:
	${RM} ${EXE} *.o
