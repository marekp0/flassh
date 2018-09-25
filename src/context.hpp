#pragma once

#include "eventLoop.hpp"
#include <map>
#include <string>
#include <vector>
#include <deque>

class Host;
struct HostInfo;
class Process;
class Command;
namespace std { class thread; }

/**
 * Manages connections, variables, etc
 */
class Context {
public:
    Context();
    ~Context();

    /**
     * Enqueues a command to be run. Takes ownership of the command.
     */
    void enqueueCommand(Command* cmd);

    /**
     * Blocks until all commands in the command queue have been run.
     */
    void flushCmdQueue();

    // the rest of these methods MUST be called on the event loop thread

    Host* addHost(const std::string& alias, const HostInfo& info);
    Host* getHost(const std::string& alias);

    Process* createPocess(const std::string& hostAlias, const std::vector<std::string>& args);

    EventLoop* getEvtLoop() { return &evtLoop; }
    
private:
    EventLoop evtLoop;
    std::thread* evtLoopThread;

    std::deque<Command*> cmdQueue;
    bool cmdExecuting = false;

    std::map<std::string, Host*> hosts;

    void execNextCommand();
};
