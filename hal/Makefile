include ../Makefile.inc

FILENAME1 = libcamera.oem
FILENAME2 = libcamera.toy
FILENAME3 = ControlThread

GEN_EXE1 = $(FILENAME1).so
GEN_EXE2 = $(FILENAME2).so
GEN_EXE3 = hardware.o

SRC1_1 = camera_HAL_oem.cpp
SRC1_2 = $(FILENAME3).cpp

SRC2_1 = camera_HAL_toy.cpp
SRC2_2 = $(FILENAME3).cpp

SRC3 = hardware.c

EXE = $(GEN_EXE1) $(GEN_EXE2) $(GEN_EXE3)

all: $(EXE)

$(GEN_EXE3): $(HEADERS) $(SRC3)
	$(CC) -g -r $(CFLAGS) $(GEN_EXE3) $(SRC3)

#.PHONY: libcamera.oem.so
$(GEN_EXE1): $(HEADERS)
	$(CC) -g -shared -fPIC -o $(GEN_EXE1) $(PL_CFLAGS) $(CPPFLAGS) ./oem/$(SRC1_1) ./oem/$(SRC1_2)

#.PHONY: libcamera.toy.so
$(GEN_EXE2): $(HEADERS)
	$(CC) -g -shared -fPIC -o $(GEN_EXE2) $(PL_CFLAGS) $(CPPFLAGS) ./toy/$(SRC2_1) ./toy/$(SRC2_2)

allgen : ${GEN_EXE}

clean:
	${RM} ${EXE} *.o
