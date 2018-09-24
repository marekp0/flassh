#include "command.hpp"
#include "process.hpp"
#include "context.hpp"

NopCommand::NopCommand(ProcessFinishedCallback onFinish)
    : additionalOnFinish(onFinish) {}

void NopCommand::start(Context* c, ProcessFinishedCallback onFinish)
{
    if (additionalOnFinish)
        additionalOnFinish(0);

    if (onFinish)
        onFinish(0);
}


SimpleCommand::SimpleCommand(const std::string& hostAlias, const std::vector<std::string>& args) 
    : hostAlias(hostAlias), args(args) {}

void SimpleCommand::start(Context* c, ProcessFinishedCallback onFinish)
{
    auto p = c->createPocess(hostAlias, args);
    p->start(onFinish);
}



NewHostCommand::NewHostCommand(const std::string& alias, const HostInfo& info) : 
    alias(alias), hostInfo(info) {}

void NewHostCommand::start(Context* c, ProcessFinishedCallback onFinish)
{
    c->addHost(alias, hostInfo);
    onFinish(0);
}
