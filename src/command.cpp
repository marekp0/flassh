#include "command.hpp"
#include "process.hpp"

SimpleCommand::SimpleCommand(const std::vector<std::string>& args) : args(args) {}

int SimpleCommand::run()
{
    LocalProcess p(args);
    p.run();
    return p.wait();
}
