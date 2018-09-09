#pragma once

#include "process.hpp"
#include "host.hpp"
#include <vector>
#include <string>
#include <memory>

class Context;

/**
 * Represents a parsed command
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
     * Runs the command and waits for it to complete.
     * 
     * @return The return code of the command.
     */
    virtual int run(Context* c) = 0;
};

/**
 * A simple command
 */
class SimpleCommand : public Command {
public:
    SimpleCommand(const std::string& hostAlias, const std::vector<std::string>& args);

    int run(Context* c);

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

private:
    std::string alias;
    HostInfo hostInfo;
};
