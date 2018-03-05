# Read Evaluate Print Loop ++

![demo](https://drive.google.com/uc?export=download&id=0B53g2Y3z7rWNT2dCRGVVNldaRnc)
[![Build Status](https://travis-ci.org/AmokHuginnsson/replxx.svg?branch=master)](https://travis-ci.org/AmokHuginnsson/replxx)

A small, portable GNU readline replacement for Linux, Windows and
MacOS which is capable of handling UTF-8 characters. Unlike GNU
readline, which is GPL, this library uses a BSD license and can be
used in any kind of program.

## Origin

This replxx implementation is based on the work by
[ArangoDB Team](https://github.com/arangodb/linenoise-ng) and
[Salvatore Sanfilippo](https://github.com/antirez/linenoise) and
10gen Inc.  The goal is to create a zero-config, BSD
licensed, readline replacement usable in Apache2 or BSD licensed
programs.

## Features

* single-line and multi-line editing mode with the usual key bindings implemented
* history handling
* completion
* syntax highlighting
* hints
* BSD license source code
* Only uses a subset of VT100 escapes (ANSI.SYS compatible)
* UTF8 aware
* support for Linux, MacOS and Windows

It deviates from Salvatore's original goal to have a minimal readline
replacement for the sake of supporting UTF8 and Windows. It deviates
from 10gen Inc.'s goal to create a C++ interface to linenoise. This
library uses C++ internally, but to the user it provides a pure C
interface that is compatible with the original linenoise API.
C interface.

## Requirements

To build this library, you will need a C++11-enabled compiler and
some recent version of CMake.

## Build instructions

To build this library on Linux, first create a build directory

```bash
mkdir -p build
```

and then build the library:

```bash
(cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make)
```

To build and install the library at the default target location, use

```bash
(cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && sudo make install)
```

The default installation location can be adjusted by setting the `DESTDIR`
variable when invoking `make install`:

```bash
(cd build && make DESTDIR=/tmp install)
```

To build the library on Windows, use these commands in an MS-DOS command 
prompt:

```
md build
cd build
```

After that, invoke the appropriate command to create the files for your
target environment:

* 32 bit: `cmake -G "Visual Studio 12 2013" -DCMAKE_BUILD_TYPE=Release ..`
* 64 bit: `cmake -G "Visual Studio 12 2013 Win64" -DCMAKE_BUILD_TYPE=Release ..`

After that, open the generated file `replxx.sln` from the `build`
subdirectory with Visual Studio.

## Tested with...

 * Linux text only console ($TERM = linux)
 * Linux KDE terminal application ($TERM = xterm)
 * Linux xterm ($TERM = xterm)
 * Linux Buildroot ($TERM = vt100)
 * Mac OS X iTerm ($TERM = xterm)
 * Mac OS X default Terminal.app ($TERM = xterm)
 * OpenBSD 4.5 through an OSX Terminal.app ($TERM = screen)
 * IBM AIX 6.1
 * FreeBSD xterm ($TERM = xterm)
 * ANSI.SYS
 * Emacs comint mode ($TERM = dumb)
 * Windows

Please test it everywhere you can and report back!

## Let's push this forward!

Patches should be provided in the respect of linenoise sensibility for
small and easy to understand code that and the license
restrictions. Extensions must be submitted under a BSD license-style.
A contributor license is required for contributions.

