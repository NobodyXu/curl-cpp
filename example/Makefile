CXX := clang++
LDFLAGS := -lpthread -Wl,--plugin-opt=O3,-O2,--discard-all,--strip-all,--gc-sections -fuse-ld=lld

include ../linking.mk

CXXFLAGS += -flto -O3 -fno-exceptions -fno-rtti

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

curl_multi_socket_action_uv.out: curl_multi_socket_action_uv.cc curl_multi_socket_action_uv.d ../libcurl_cpp.a
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(LDFLAGS) -luv $< ../libcurl_cpp.a -o $@
	mv -f $*.Td $*.d && touch $@
	web_server/run_nginx.sh
	./$@

curl_multi_socket_action_event.out: ../libcurl_cpp.a
curl_multi_socket_action_event.out: curl_multi_socket_action_event.cc curl_multi_socket_action_event.d
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(LDFLAGS) -levent $< ../libcurl_cpp.a -o $@
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
