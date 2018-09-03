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

    // used to signal exit status
    bool exitStatusValid = false;
    int exitStatus;

    std::mutex exitStatusMutex;
    std::condition_variable exitStatusCond;

    static void staticOnExitStatus(ssh_session session, ssh_channel channel, int status, void* userdata);
    void onExitStatus(ssh_session session, ssh_channel channel, int status);
};
