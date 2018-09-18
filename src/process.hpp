#pragma once

#include <libssh/libssh.h>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

class Process {
public:
    virtual ~Process() = default;

    virtual void run() = 0;
    virtual int wait() = 0;
};

class LocalProcess : public Process {
public:
    LocalProcess(const std::vector<std::string>& args);

    void run();
    int wait();

private:
    // TODO: too much copying?
    std::vector<std::string> args;
    std::vector<const char*> argv;

    pid_t pid = 0;
};

class RemoteProcess : public Process {
public:
    RemoteProcess(ssh_session session, const std::vector<std::string>& args);

    void run();
    int wait();

private:
    ssh_channel channel = nullptr;
    std::string cmd;

    static int staticOnData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata);

    int onData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata);
};
