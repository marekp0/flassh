# flassh
flassh allows the creation of shell scripts that run commands on multiple
machines via ssh.


## Features
None

### Planned Features
 * Run commands on multiple ssh hosts from a single script
 * I/O between commands running on different hosts
 * Interactive Mode

### Supported Systems
 * Linux

### Dependencies
 * libssh


## Building
Requires CMake 3.1+ and a C++17 compiler.

```
mkdir ./build
cd build
cmake ..
make
make install
```

