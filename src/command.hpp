#pragma once

#include "process.hpp"
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

typedef std::vector<std::unique_ptr<Command>> CommandList;

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
