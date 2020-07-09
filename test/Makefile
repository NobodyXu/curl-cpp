CXX := clang++
LDFLAGS := 

include ../linking.mk

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.out)
CXXFLAGS += -g -fsanitize=address

test: $(OBJS)

$(OBJS):

test_curl_version.out: test_curl_version.cc utility.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@
	./$@

%.out: %.cc ../libcurl_cpp.a utility.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< ../libcurl_cpp.a -o $@
	./$@

../libcurl_cpp.a: FORCE
	$(MAKE) -C ../
FORCE: ;

clean:
	rm -f $(OBJS)
.PHONY: clean test
