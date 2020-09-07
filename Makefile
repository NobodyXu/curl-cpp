CXX := clang++

CXXFLAGS := $(CXXFLAGS) -std=c++17 -flto -O3 -fno-exceptions -fno-rtti $(shell curl-config --cflags)
LDFLAGS := $(LDFLAGS) $(shell curl-config --libs) -fuse-ld=lld

SRCS := $(wildcard *.cc) $(wildcard utils/*.cc)
DEPS := $(SRCS:.cc=.d)
OBJS := $(SRCS:.cc=.o)

libcurl_cpp.a: $(OBJS)
	llvm-ar rcsT $@ $^

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

cleandoc:
	rm -rf doc/*

.PHONY: clean test gendoc cleandoc
