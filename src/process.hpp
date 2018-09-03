#pragma once

#include <vector>
#include <string>

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
