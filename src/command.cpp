#include "command.hpp"
#include "process.hpp"
#include "context.hpp"

SimpleCommand::SimpleCommand(const std::string& hostAlias, const std::vector<std::string>& args) 
    : hostAlias(hostAlias), args(args) {}

int SimpleCommand::run(Context* c)
{
    auto p = c->createPocess(hostAlias, args);
    p->run();    // TODO: handle error from here
    int exitCode = p->wait();
    delete p;   // TODO: not deleted if exception

    return exitCode;
}



NewHostCommand::NewHostCommand(const std::string& alias, const HostInfo& info) : 
    alias(alias), hostInfo(info) {}

int NewHostCommand::run(Context* c)
{
    c->addHost(alias, hostInfo);
    return 0;
}
