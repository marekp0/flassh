#pragma once

#include <libssh/libssh.h>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <functional>

typedef std::function<void(int)> ProcessFinishedCallback;


class Process {
public:
    virtual ~Process() = default;

    /**
     * Starts the process, and calls the callback function when finished.
     */
    virtual void start(ProcessFinishedCallback onFinish) = 0;
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
    RemoteProcess(ssh_session session, const std::vector<std::string>& args);

    void start(ProcessFinishedCallback onFinish);

private:
    ssh_channel channel = nullptr;
    std::string cmd;
    ProcessFinishedCallback onFinish;

    static int staticOnData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata);
    static void staticOnExitStatus(ssh_session session, ssh_channel channel, int status, void* userdata);

    int onData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr);
    void onExitStatus(ssh_session, ssh_channel channel, int status);
};
