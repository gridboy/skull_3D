CXX ?= c++
CXXFLAGS ?= -O2 -std=c++11 -Wall -Wextra
CPPFLAGS += -Iinclude -DSKULL_DATA_DIR='"$(CURDIR)/data/cthead"'
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
GLFW_PREFIX ?= $(shell brew --prefix glfw 2>/dev/null)
CPPFLAGS += -I$(GLFW_PREFIX)/include
LDLIBS += -L$(GLFW_PREFIX)/lib -lglfw -framework OpenGL -framework CoreGraphics -framework CoreFoundation -framework CoreText
else
CPPFLAGS += $(shell pkg-config --cflags glfw3)
LDLIBS += $(shell pkg-config --libs glfw3) -lGL
endif

all: bin/skull
bin/skull: src/main.cpp src/volume_renderer.cpp include/volume_renderer.h
	mkdir -p bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp src/volume_renderer.cpp -o $@ $(LDFLAGS) $(LDLIBS)

bin/renderer_test: tests/renderer_test.cpp src/volume_renderer.cpp include/volume_renderer.h
	mkdir -p bin
	$(CXX) -Iinclude $(CXXFLAGS) -UNDEBUG tests/renderer_test.cpp src/volume_renderer.cpp -o $@ $(LDFLAGS)

test: bin/renderer_test
	./bin/renderer_test

run: bin/skull
	./bin/skull

clean:
	rm -f bin/skull bin/renderer_test
.PHONY: all test run clean
