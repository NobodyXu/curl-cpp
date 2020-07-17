# curl-cpp

![C/C++ CI](https://github.com/NobodyXu/curl-cpp/workflows/C/C++%20CI/badge.svg)

cpp wrapper for libcurl that support -fno-exceptions and static linking.

curl-cpp provides simple measures to test whether certain features is supported before any easy handle is created
(except for cookies support, which can only be detected when setting a cookie in easy handler).

[Document](https://nobodyxu.github.io/curl-cpp/index.html)
[Example](https://github.com/NobodyXu/curl-cpp/tree/master/example)

## How to build and statically link with this lib

```
git clone https://github.com/NobodyXu/curl-cpp.git
cd curl-cpp/
make
```
