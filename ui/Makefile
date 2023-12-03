include ../Makefile.inc

FILENAME1 = gui
FILENAME2 = input
GEN_EXE1 = $(FILENAME1).o
GEN_EXE2 = $(FILENAME2).o
SRC1 = $(FILENAME1).c
SRC2 = $(FILENAME2).c

EXE = $(GEN_EXE1) $(GEN_EXE2)

all: $(EXE)

$(GEN_EXE1): $(HEADERS) $(SRC1)
	$(CC) -c $(CFLAGS) $(GEN_EXE1) $(SRC1)


$(GEN_EXE2): $(HEADERS) $(SRC2)
	$(CC) -c $(CFLAGS) $(GEN_EXE2) $(SRC2) 

allgen : ${GEN_EXE}

clean:
	${RM} ${EXE} *.o
