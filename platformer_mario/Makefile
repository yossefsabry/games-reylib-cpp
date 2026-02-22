CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS ?= -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

TARGET = platformer_mario
SRC = main.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
