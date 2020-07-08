CXXFLAGS := -std=c++17 -flto -fno-fat-lto-objects -O2 $(shell curl-config --cflags)
LDFLAGS += $(shell curl-config --libs)
