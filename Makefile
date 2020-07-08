CXX := clang++

CXXFLAGS := -std=c++17 -flto -O2 $(shell curl-config --cflags)
LDFLAGS := $(shell curl-config --libs)

SRCS := $(wildcard *.cc)
DEPS := $(SRCS:.cc=.d)
OBJS := $(SRCS:.cc=.o)

# Autobuild dependency, adapted from:
#    http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#include
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

$(DEPS): 
include $(wildcard $(DEPS))

## Disable implict pattern
%.o : %.cc
%.o : %.cc %.d
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c -o $@ $<
	mv -f $*.Td $*.d && touch $@

libcurl_cpp.a: $(OBJS)
	llvm-ar rcsT $@ $^
	llvm-ranlib $@

clean:
	rm -f *.o $(DEPS) $(DEPS:.d=.Td) $(OBJS)

.PHONY: clean test
