CXX := clang++
LDFLAGS := 

include ../linking.mk

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.out)
CXXFLAGS += -g -fsanitize=address

test: $(OBJS:.out=_run)

%_run: %.out
	./$<

$(OBJS):

test_curl_version.out: test_curl_version.cc utility.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

%.out: %.cc ../libcurl_cpp.a utility.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< ../libcurl_cpp.a -o $@

../libcurl_cpp.a:
	$(MAKE) -C ../

clean:
	rm -f $(OBJS)
.PHONY: clean test