#include "command.hpp"
#include "process.hpp"

SimpleCommand::SimpleCommand(const std::vector<std::string>& args) : args(args) {}

int SimpleCommand::run()
{
    LocalProcess p(args);
    p.run();
    return p.wait();
}



NewHostCommand::NewHostCommand(const HostInfo& info) : hostInfo(info) {}

int NewHostCommand::run()
{
    Host h;
    h.connect(hostInfo);
    return 0;
}
