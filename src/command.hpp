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
     * Starts running the command. Making calls to the event loop may be
     * requied for the command to properly finish.
     * 
     * @param onFinish  Called when the command finishes running. The function
     *                  signature is `void onFinish(int exitCode)`. This may be
     *                  called on any thread.
     */
    virtual void start(Context* c, ProcessFinishedCallback onFinish) = 0;
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

    void start(Context* c, ProcessFinishedCallback onFinish);

private:
    ProcessFinishedCallback additionalOnFinish;
};

/**
 * A simple command
 */
class SimpleCommand : public Command {
public:
    SimpleCommand(const std::string& hostAlias, const std::vector<std::string>& args);

    int run(Context* c);
    void start(Context* c, ProcessFinishedCallback onFinish);

private:
    std::string hostAlias;
    std::vector<std::string> args;
};

/**
 * Opens a new connection to a host
 */
class NewHostCommand : public Command {
public:
    NewHostCommand(const std::string& alias, const HostInfo& info);

    int run(Context* c);
    void start(Context* c, ProcessFinishedCallback onFinish);

private:
    std::string alias;
    HostInfo hostInfo;
};
