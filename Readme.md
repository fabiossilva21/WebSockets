# Sha1Algorithm
This library implements a simple WebSocket server as described in [RFC6455](https://tools.ietf.org/html/rfc6455)

## Usage
This is meant to be used along side with [SHA1Hash](https://github.com/fabiossilva21/SHA1Hash) and [BaseEncode](https://github.com/fabiossilva21/BaseEncode) libraries available in my GitHub profile.
I still have to find a better way to do their inclusion, but the local repos should be located in the directory before this one ".."

## Supported platforms 
This library makes use of C++ version 14, although it should be able to be compiled in previous versions.
The library was tested in Linux (Debian) but should run in any other platform.

### Compilation process
For compilation purposes, a Makefile was included in the project to ease the compilation process, however the Makefile only generates the object (.o) file and the linkage should be handled by the programmer using it.

## Prerequisites
* [GNU Make](https://www.gnu.org/software/make/) (version 4.2.1 or higher)
* C++14 compatible toolchain (g++ 8.3.0 works fine)
* [SHA1Hash](https://github.com/fabiossilva21/SHA1Hash)
* [BaseEncode](https://github.com/fabiossilva21/BaseEncode)

## Warning
This project it's in his really WIP phase, as such, you won't get many functionalities and may encounter many bugs while using it. As such, there is no real documentation and you shouldn't use it to be honest.
