#pragma once

#include <libssh/libssh.h>
#include <string>

struct HostInfo {
    std::string userName;
    std::string hostName;
    unsigned int port;

    /**
     * Parse a string of the form [username@]hostname[:port]
     * 
     * @return true if succeeded, false otherwise
     */
    bool parse(const std::string& str);

    std::string toString();
};

class Host {
public:
    Host();
    ~Host();

    void connect(const HostInfo& info);
    void authHost();
    void authUser();
    void disconnect();

    ssh_session getSession() const { return session; }

private:
    ssh_session session;
    HostInfo info;

    void sshException(const std::string& what);
};
