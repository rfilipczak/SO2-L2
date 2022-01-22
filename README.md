# SO2-L2

Program creates N [3...100] threads. Threads end in a specified order, based on command-line arguments and their ID. The program should work on both Linux and Windows and use os-specific syscalls/libraries (i.e pthreads for Linux, WinAPI for Windows).

### Note

This is one source file for both Linux and Windows as OS-dependencies are abstracted away in mythreading.h header.

## TODO

- [ ] syscalls error handling
- [ ] tests

## 2A (linux)

### Requirements
* g++
* c++20
* make
* [fmt](https://github.com/fmtlib/fmt)

### Build and run

```console
$ make

$ ./prog 10 inc
[THREAD: 140528981755456] Start...
[THREAD: 140528964970048] Start...
[THREAD: 140528846894656] Start...
[THREAD: 140528956577344] Start...
[THREAD: 140528948184640] Start...
[THREAD: 140528923006528] Start...
[THREAD: 140528939791936] Start...
[THREAD: 140528931399232] Start...
[THREAD: 140528973362752] Start...
[THREAD: 140528838501952] Start...
[THREAD: 140528838501952] Stop...
[THREAD: 140528846894656] Stop...
[THREAD: 140528923006528] Stop...
[THREAD: 140528931399232] Stop...
[THREAD: 140528939791936] Stop...
[THREAD: 140528948184640] Stop...
[THREAD: 140528956577344] Stop...
[THREAD: 140528964970048] Stop...
[THREAD: 140528973362752] Stop...
[THREAD: 140528981755456] Stop...

$ ./prog 3 dec
[THREAD: 140422288131648] Start...
[THREAD: 140422279738944] Start...
[THREAD: 140422271346240] Start...
[THREAD: 140422288131648] Stop...
[THREAD: 140422279738944] Stop...
[THREAD: 140422271346240] Stop...

$ ./prog 1
Invalid number of arguments
Usage: ./prog N[3-100] direction[inc/dec]
```

## 2B (windows)

### Requirements
* c++20

### Build
use your favourite compiler/build system/IDE

### Run
API is the same as for linux
