CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = load_balancer_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

docs:
	doxygen Doxyfile

.PHONY: all clean run docs