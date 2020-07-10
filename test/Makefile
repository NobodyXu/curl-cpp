CXX := clang++
LDFLAGS := 

include ../linking.mk

CXXFLAGS += -g -fsanitize=address

SRCS := $(wildcard *.cc)
DEPS := $(SRCS:.cc=.d)
OBJS := $(SRCS:.cc=.out)

# Autobuild dependency, adapted from:
#    http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#include
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

test: $(OBJS) ;

$(OBJS):

$(DEPS): 
include $(wildcard $(DEPS))

%.out: %.cc %.d ../libcurl_cpp.a utility.hpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(LDFLAGS) $< ../libcurl_cpp.a -o $@
	mv -f $*.Td $*.d && touch $@
	web_server/run_nginx.sh
	./$@

test_curl_version.out: test_curl_version.cc utility.hpp test_curl_version.d
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(LDFLAGS) $< -o $@
	mv -f $*.Td $*.d && touch $@
	./$@

../libcurl_cpp.a: FORCE
	$(MAKE) -C ../

FORCE: ;

clean:
	rm -f $(OBJS) $(DEPS)
	web_server/stop_nginx.sh

.PHONY: clean test
