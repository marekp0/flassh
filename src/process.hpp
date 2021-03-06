#pragma once

#include <libssh/libssh.h>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <functional>

typedef std::function<void(int)> ProcessFinishedCallback;
class Context;

struct IoRedir {
    int oldfd;
    int newfd;
};

class Process {
public:
    virtual ~Process() = default;

    /**
     * Starts the process, and calls the callback function when finished.
     */
    virtual void start(ProcessFinishedCallback onFinish) = 0;

    /**
     * Redirects local FD `fdLocal` to the process FD `fdProc`. Must be called
     * before the process is started.
     * 
     * @param fdLocal  A currently open file descriptor
     * @param fdProc   A file descriptor in the child process. Must be one of
     *                 `STDIN_FILENO`, `STDOUT_FILENO`, or`STDERR_FILENO`.
     */
    void redirectIo(int fdLocal, int fdProc);

protected:
    // TODO: shouldn't need to keep this around
    std::vector<IoRedir> ioRedirs;
};

class LocalProcess : public Process {
public:
    LocalProcess(const std::vector<std::string>& args);

    void start(ProcessFinishedCallback onFinish);

private:
    // TODO: too much copying?
    std::vector<std::string> args;
    std::vector<const char*> argv;

    pid_t pid = 0;
};

class RemoteProcess : public Process {
public:
    RemoteProcess(ssh_session session, Context* ctx, const std::vector<std::string>& args);

    void start(ProcessFinishedCallback onFinish);

private:
    Context* ctx = nullptr;
    ssh_session session = nullptr;
    ssh_channel channel = nullptr;
    std::string cmd;

    int exitStatus = 1;
    ProcessFinishedCallback onFinish;

    // workaround for libssh connectors bug
    // connectors sometimes cut off data at the end
    // connector for stdin doesn't really work well with pipes
    int stdinLocalFd;
    int stdoutLocalFd;
    int stderrLocalFd;

    static int staticOnData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata);
    static void staticOnExitStatus(ssh_session session, ssh_channel channel, int status, void* userdata);
    static void staticOnClose(ssh_session session, ssh_channel channel, void* userdata);

    int onData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr);
    void onExitStatus(ssh_session, ssh_channel channel, int status);
    void onClose(ssh_session, ssh_channel channel);

    static int forwardFdToChannel(int fd, int revents, void* userdata);
};
