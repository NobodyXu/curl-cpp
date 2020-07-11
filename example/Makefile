CXX := clang++
LDFLAGS := -lpthread

include ../linking.mk

CXXFLAGS += -g -flto -O2 -fno-exceptions

SRCS := $(wildcard *.cc)
DEPS := $(SRCS:.cc=.d)
OBJS := $(SRCS:.cc=.out)

# Autobuild dependency, adapted from:
#    http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#include
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

examples: $(OBJS) ;

$(OBJS):

$(DEPS): 
include $(wildcard $(DEPS))

%.out: %.cc %.d ../libcurl_cpp.a
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(LDFLAGS) $< ../libcurl_cpp.a -o $@
	mv -f $*.Td $*.d && touch $@
	web_server/run_nginx.sh
	./$@

../libcurl_cpp.a: FORCE
	$(MAKE) -C ../

FORCE: ;

clean:
	rm -f $(OBJS) $(DEPS)
	web_server/stop_nginx.sh

.PHONY: clean test
