include ../Makefile.inc

FILENAME = web_server
GEN_EXE = $(FILENAME).o
SRC = $(FILENAME).c

EXE = $(GEN_EXE) $(ETC_EXE)

all: $(EXE)

$(EXE): $(HEADERS) $(SRC)
	$(CC) -c $(CFLAGS) $(GEN_EXE) $(SRC)


allgen : ${GEN_EXE}


clean:
	${RM} ${EXE} *.o
