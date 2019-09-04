# libConfuseFS

[![Build Status](https://api.travis-ci.org/yossizap/libconfusefs.svg?branch=master)](https://api.travis-ci.org/yossizap/libconfusefs)

Easily embeddable library that exposes a program's configuration as a filesystem using FUSE. Written in C++11.
The configuration is currently backed by a JSON Schema(using [JConf](https://github.com/yossizap/jconf)).

The goal is to provide users with an easily accessible interface similar to linux kernel's [sysfs](https://github.com/torvalds/linux/blob/master/Documentation/filesystems/sysfs.txt) with type safety and schema validation.

## Building
```
git submodule update --init --recursive
mkdir build && cd build
cmake ..
cmake --build .
```
