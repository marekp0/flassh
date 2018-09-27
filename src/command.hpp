#pragma once

#include "process.hpp"
#include "host.hpp"
#include <vector>
#include <string>
#include <memory>
#include <functional>

class Context;

/**
 * Represents a linked-list of command trees
 */
class Command {
public:
    /**
     * Constructs a command
     * 
     * @param line  A line number associated with the command
     */
    //Command(int line);

    /**
     * Starts running the command.
     * 
     * Making calls to the event loop may be requied for the command to
     * properly finish. Running this function while the command is already
     * running should work, e.g. for function calls or loops that start
     * background processes.
     * 
     * @param c         The context in which the command will be run
     * @param redirs    A list of I/O redirections to be done
     * @param onFinish  Called when the command finishes running. The function
     *                  signature is `void onFinish(int exitCode)`. This may be
     *                  called on any thread.
     */
    virtual void start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish) = 0;
};

/**
 * A command that does nothing
 */
class NopCommand : public Command {
public:
    /**
     * @param onFinish  An additional callback to be called when the command is
     *                  finished.
     */
    NopCommand(ProcessFinishedCallback onFinish);

    void start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish);

private:
    ProcessFinishedCallback additionalOnFinish;
};

/**
 * A simple command
 */
class SimpleCommand : public Command {
public:
    SimpleCommand(const std::string& hostAlias, const std::vector<std::string>& args);

    void start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish);

private:
    std::string hostAlias;
    std::vector<std::string> args;
};

class PipeCommand : public Command {
public:
    PipeCommand(Command* left, Command* right);

    void start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish);

private:
    Command* leftCmd;
    Command* rightCmd;
};

/**
 * Opens a new connection to a host
 */
class NewHostCommand : public Command {
public:
    NewHostCommand(const std::string& alias, const HostInfo& info);

    void start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish);

private:
    std::string alias;
    HostInfo hostInfo;
};
