INC_FLAGS := -I../include
LIB_FLAGS := -L../lib
LIB := -lisp -lsns_imx122 -lmpi -laec -lanr -lresampler -lVoiceEngine
OPENCV_FLAGS := -lpthread -lrt `pkg-config --libs --cflags hisiv100-opencv`
CFLAGS := -Wall  $(INC_FLAGS) $(LIB_FLAGS) $(LIB) $(OPENCV_FLAGS)
CC := arm-hisiv100nptl-linux-g++

all:
	$(CC) test.cpp $(CFLAGS)  -o test

install:
	cp test ~/hi3516c/nfs/

