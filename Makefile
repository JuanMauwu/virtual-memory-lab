CXX      = g++
CXXFLAGS = -Wall -Werror -std=c++11
TARGET   = vm
SRC      = src/main.cpp src/vm.cpp
TEST     = tests/test1_basico.txt

all: $(TARGET)

$(TARGET): $(SRC) src/vm.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run: all
	./$(TARGET) $(TEST) -v

clean:
	rm -f $(TARGET)

.PHONY: all run clean
