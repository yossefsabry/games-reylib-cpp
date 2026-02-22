CXX = g++
CXXFLAGS = -std=c++17 -O2
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

TARGET = dungeon_crawler
SRCS = main.cpp game.cpp game_state.cpp dungeon.cpp input.cpp render.cpp ui.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
