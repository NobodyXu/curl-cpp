CXX := clang++

CXXFLAGS := -std=c++17 -flto -O2 -fno-exceptions $(shell curl-config --cflags)
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

TEST_SRCS := $(wildcard test/*.cc)
TEST_OBJS := $(TEST_SRCS:.cc=.out)
TEST_CXXFLAGS := -std=c++17 -flto -g -fsanitize=address

test: $(TEST_OBJS:.out=_run)

test/%_run: test/%.out
	./$<

test/test_curl_version.out: test/test_curl_version.cc test/utility.hpp
	$(CXX) $(TEST_CXXFLAGS) $(LDFLAGS) $< -o $@

test/%.out: test/%.cc libcurl_cpp.a test/utility.hpp
	$(CXX) $(TEST_CXXFLAGS) $(LDFLAGS) $< libcurl_cpp.a -o $@

clean:
	rm -f *.o $(DEPS) $(DEPS:.d=.Td) $(OBJS) $(TEST_OBJS) libcurl_cpp.a

.PHONY: clean test
