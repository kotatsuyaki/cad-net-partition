.POSIX:

RM = rm
CXX = /opt/rh/devtoolset-7/root/usr/bin/g++
CXXFLAGS = -O3 -Wall -std=c++17 -march=native

TARGET = pa2
SRC_DIRS = ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(addsuffix .o,$(basename $(SRCS)))
DEPS := $(OBJS:.o=.d)

# Add all subdirectories with name "include" in ./external to include path
INC_DIRS := $(shell find ./external -type d -name include) ./external/parallel-hashmap
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS = $(INC_FLAGS) -MMD -MP
LDFLAGS = -flto

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

.PHONY: clean run
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

run: $(TARGET)
	./$(TARGET) ./testcases/case00.in out

-include $(DEPS)
