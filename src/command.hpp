#pragma once

#include "process.hpp"
#include "host.hpp"
#include <vector>
#include <string>
#include <memory>

/**
 * Represents a parsed command
 */
class Command {
public:
    /**
     * Runs the command and waits for it to complete.
     * 
     * @return The return code of the command.
     */
    virtual int run() = 0;
};

/**
 * A simple command
 */
class SimpleCommand : public Command {
public:
    SimpleCommand(const std::vector<std::string>& args);

    int run();

private:
    std::vector<std::string> args;
};

/**
 * Opens a new connection to a host
 */
class NewHostCommand : public Command {
public:
    NewHostCommand(const HostInfo& info);

    int run();

private:
    HostInfo hostInfo;
};
