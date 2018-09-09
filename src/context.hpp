#pragma once

#include <map>
#include <string>
#include <vector>

class Host;
struct HostInfo;
class Process;

/**
 * Manages connections, variables, etc
 */
class Context {
public:
    Host* addHost(const std::string& alias, const HostInfo& info);
    Host* getHost(const std::string& alias);

    Process* createPocess(const std::string& hostAlias, const std::vector<std::string>& args);
    
private:
    std::map<std::string, Host*> hosts;
};
