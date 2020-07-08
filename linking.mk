CXXFLAGS := -std=c++17 -freg-struct-return -fshort-enums -flto -fno-fat-lto-objects -O2 $(shell curl-config --cflags)
LDFLAGS += $(shell curl-config --libs)
