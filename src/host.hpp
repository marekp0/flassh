#pragma once

#include <libssh/libssh.h>
#include <string>

struct HostInfo {
    std::string userName;
    std::string hostName;
    unsigned int port;
};

class Host {
public:
    Host();
    ~Host();

    void connect(const HostInfo& info);
    void authHost();
    void authUser();
    void disconnect();

private:
    ssh_session session;
    HostInfo info;

    void sshException(const std::string& what);
};
