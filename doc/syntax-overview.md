# flassh syntax overview
The syntax of flassh is supposed to be similar to that of a typical UNIX shell,
with a few additions to handle SSH stuff.

## Declaring remote hosts
The syntax for declaring a remote host is:
```
<remote_name> := <user>@<domain>[:port]
```

When the script is run, or when this is entered in an interactive shell, the
user will be prompted for logon details. Currently, flassh will not store
passwords. If you want to log in without prompting, use `ssh-agent`.

Some examples:
```
remote1 := root@example.com
remote2 := root@example.com:42
```

## Local command syntax
Running local commands is similar to bash:
```
cmd arg1 arg2 ...
cmd1 arg1 arg2 | cmd2 arg1 arg2 > /some/file
```

## Remote command syntax
The easiest way to run a remote command is:
```
<remote_name>: cmd arg1 arg2 ...
```

All arguments are passed as-is, meaning any file paths are on the remote host.
With this syntax, all I/O redirection and pipes will also be on the remote
host.

To run a series of commands on a remote host, use curly brackets:
```
<remote_name>: {
    cmd1 arg1 arg2 ...
    cmd2 arg1 arg2 ...
}
```

## Mixing hosts
I/O redirection and piping can happen across different hosts. This is done
using a C++-style namespace syntax:
```
[default_host]: [[remote_host]::]cmd > [[remote_host]::]/path/to/file
```

`default_host` will define the default remote host to be used if the remote
host for a command or I/O redirection is not specified. If this is not
specified, then the local machine is the default host.

If `default_host` is specified, and you want to pipe or I/O redirect into the
local machine, use `::<cmd>` or `::/path/to/file`.

## Examples
```
echo "Hello world" > remote1::file.txt
```
This will run `echo "Hello world"` on the local machine, and output the result
to `file.txt` on `remote1`.

```
remote1: cat file.txt | remote2::grep flassh > ::result.txt
```
This will run `cat file.txt` on `remote1`, pipe the results to `grep flassh` on
`remote2`, and write the results to `result.txt` on the local machine.

```
remote1: remote2::grep something /home/root > result.txt
```
This wil run `grep something /home/root` on `remote2`, and write the results to
`result.txt` on `remote1`.
