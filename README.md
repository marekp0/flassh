# flassh
flassh allows the creation of shell scripts that run commands on multiple
machines via ssh.

## Example
Eventually, flassh should be able to do something like this:
```
srv1 := user@example.com
srv2 := user@example2.com

srv: ps aux | grep httpd > srv2::~/srv1_httpd_list.txt
srv1::./script1.sh | srv2::./script2.sh > local_file.txt
```

For now, several core features, such as I/O redirection to a file and `scp`
functionality, are missing. At the moment, something like this should work:
```
srv := user@example.com

ps aux              # list all local processes
srv: ps aux         # list all remote processes

# run ./script.sh on the local machine, pipe into `tee file.txt` on the server
./script.sh | srv::tee file.txt
```

For more examples, see the [syntax overview](doc/syntax-overview.md) page.


## Features
*Note: flassh is still in the early stages of development. Many important
features are currently missing, and things may break without warning.*
 * Run commands on multiple ssh hosts from a single script
 * Pipe stdin/stdout/stderr between processes on any host

### Planned Features
 * I/O redirection to files on any local or remote host
 * Built-in scp-like functionality
 * Full compatibility with bash

### Supported Systems
 * Linux

### Dependencies
 * [libssh](https://www.libssh.org/)


## Building
Requires CMake 3.1+ and a C++17 compiler (gcc 7 or clang 6).

```
mkdir ./build
cd build
cmake ..
make
make install
```

## License
flassh is [MIT licensed](LICENSE.txt).
