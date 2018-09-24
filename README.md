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

For now, it is capable only of performing simple commands on local or remote
hosts with no I/O redirection:
```
srv := user@example.com

ps aux              # list all local processes
srv: ls -lah        # list all remote processes
```

For more examples, see the [syntax overview](doc/syntax-overview.md) page.


## Features
 * Run commands on multiple ssh hosts from a single script

### Planned Features
 * I/O between commands running on different hosts
 * Built-in scp-like functionality
 * Full compatibility with bash

### Supported Systems
 * Linux

### Dependencies
 * [libssh](https://www.libssh.org/)


## Building
Requires CMake 3.1+ and a C++17 compiler.

```
mkdir ./build
cd build
cmake ..
make
make install
```

## License
flassh is [MIT licensed](LICENSE.txt).
