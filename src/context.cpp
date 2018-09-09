#include "context.hpp"
#include "host.hpp"
#include "process.hpp"

Host* Context::addHost(const std::string& alias, const HostInfo& info)
{
    if (hosts.find(alias) != hosts.end()) {
        throw std::runtime_error("Host with name " + alias + " already exists");
    }

    Host* h = new Host();
    h->connect(info);
    hosts[alias] = h;
    return h;
}

Host* Context::getHost(const std::string& alias)
{
    auto it = hosts.find(alias);
    if (it == hosts.end()) {
        throw std::runtime_error("No host with alias " + alias);
    }
    return it->second;
}

Process* Context::createPocess(const std::string& hostAlias, const std::vector<std::string>& args)
{
    if (hostAlias.empty()) {
        return new LocalProcess(args);
    }
    else {
        Host* h = getHost(hostAlias);
        return new RemoteProcess(h->getSession(), args);
    }
}
