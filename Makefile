CXX := clang++

CXXFLAGS := -std=c++17 -flto -O2 -fno-exceptions -fno-rtti $(shell curl-config --cflags)
LDFLAGS := $(shell curl-config --libs)

SRCS := $(wildcard *.cc) $(wildcard utils/*.cc)
DEPS := $(SRCS:.cc=.d)
OBJS := $(SRCS:.cc=.o)

libcurl_cpp.a: $(OBJS)
	llvm-ar rcsT $@ $^
	llvm-ranlib $@

$(DEPS): 
include $(wildcard $(DEPS))

# Autobuild dependency, adapted from:
#    http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#include
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

## Disable implict pattern
%.o : %.cc
%.o : %.cc %.d
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c -o $@ $<
	mv -f $*.Td $*.d && touch $@

test: libcurl_cpp.a
	$(MAKE) -C test/

clean:
	rm -f *.o $(DEPS) $(DEPS:.d=.Td) $(OBJS) libcurl_cpp.a
	$(MAKE) -C test/ clean

gendoc:
	doxygen Doxyfile

.PHONY: clean test gendoc
