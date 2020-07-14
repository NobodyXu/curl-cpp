CXXFLAGS := -std=c++17 $(shell curl-config --cflags)
LDFLAGS += $(shell curl-config --libs) -flto
