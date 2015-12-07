OPENCV_FLAGS := -lpthread -lrt `pkg-config --libs --cflags hisiv100-opencv`
CC := arm-hisiv100nptl-linux-g++

SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=%.o)
all: $(OBJ)
	
$(OBJ): %.o: %.cpp
	$(CC) $(OPENCV_FLAGS) -c -O3 $< -o $@


clean:
	rm *.o
