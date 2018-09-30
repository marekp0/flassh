# flassh
[![Build Status](https://travis-ci.org/marekp0/flassh.svg?branch=master)](https://travis-ci.org/marekp0/flassh)

flassh is a shell with built-in syntax for running commands on a remote server
via ssh. 

## Example
*Note: flassh is still in the early stages of development, and this example
probably does not work yet. See [Project Status](#project-status) for more
information.*

Here is a sample flassh script:
```
srv1 := user@example.com
srv2 := user@example2.com

srv: ps aux | grep httpd > srv2::~/srv1_httpd_list.txt
srv1::./script1.sh | srv2::./script2.sh > local_file.txt
```

The equivalent of this in bash using the `ssh` command would be:
```
SRV1="ssh user@example.com"
SRV2="ssh user@example2.com"

$SRV1 "ps aux | grep httpd" | $SRV2 "cat - > srv1_httpd_list.txt"
$SRV1 "./script1.sh" | $SRV2 "./script2.sh" > local_file.txt
```

Here, flassh has several advantages over pure bash:
 * **Faster Execution**: Each usage of the `ssh` command in the above example
   must create a new ssh session to the server, while flassh only creates a
   single session at the start of the script
 * **Simpler Syntax**: The flassh script doesn't need quotes around its
   commands, and has no need for hacks such as using `cat` for I/O redirection.
 * **Fewer Passwords**: If `ssh-agent` was not being used, then the flassh
   script only requires a password for each server once at the beginning, while
   the bash version would require a password each time a remote command was run.

For more examples, see the [syntax overview](doc/syntax-overview.md) page.


## Project Status
flassh is still in the early stages of development. Many important features
are currently missing, and things may change or break without warning. At the 
moment, a script like this should work:

```
srv := user@example.com

ps aux              # list all local processes
srv: ps aux         # list all remote processes

# run ./script.sh on the local machine, pipe into `tee file.txt` on the server
./script.sh | srv::tee file.txt
```


## Features
 * Run commands on multiple ssh hosts from a single script
 * Pipe stdin/stdout/stderr between processes on any host

### Planned Features
 * I/O redirection to files on any local or remote host
 * Built-in scp-like functionality
 * Full compatibility with bash

### Supported Systems
 * Linux
 * Coming soon: OSX, FreeBSD

### Dependencies
 * [libssh](https://www.libssh.org/)


## Building
Requires CMake 3.1+ and a C++17 compiler (gcc 7 or clang 6).

```
mkdir build
cd build
cmake ..
make
make install
```


## License
flassh is [MIT licensed](LICENSE.txt).
