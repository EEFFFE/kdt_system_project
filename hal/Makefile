include ../Makefile.inc

FILENAME1 = ControlThread
FILENAME2 = camera_HAL
GEN_EXE1 = $(FILENAME1).o
GEN_EXE2 = $(FILENAME2).o
SRC1 = $(FILENAME1).cpp
SRC2 = $(FILENAME2).cpp

EXE = $(GEN_EXE1) $(GEN_EXE2)

all: $(EXE)

$(GEN_EXE1): $(HEADERS) $(SRC1)
	$(CPP) -g $(CPPINCLUDES) $(CPPFLAGS) -c  $(GEN_EXE1) $(SRC1)


$(GEN_EXE2): $(SRC2)
	$(CPP) -g $(CPPINCLUDES) $(CPPFLAGS) -c  $(GEN_EXE2) $(SRC2)

allgen : ${GEN_EXE}

clean:
	${RM} ${EXE} *.o
