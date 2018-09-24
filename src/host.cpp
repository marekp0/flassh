#include "host.hpp"
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <regex>

static int authenticate_kbdint(ssh_session session);



bool HostInfo::parse(const std::string& str)
{
    static std::regex reHostInfo(R"(^(?:(.+?)@)?(.+?)(?::(\d+))?$)");
    try {
        std::smatch m;
        if (!std::regex_match(str, m, reHostInfo)) {
            return false;
        }

        // username optional
        if (m[1].matched)
            userName = m[1].str();

        // host name required
        hostName = m[2].str();

        // port number optional, default 22
        if (m[3].matched) {
            port = std::stoul(m[3].str());
        }
        else {
            port = 22;
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

std::string HostInfo::toString()
{
    std::string str;
    if (!userName.empty())
        str += userName + "@";

    str += hostName;

    if (port != 22) {
        str += ":";
        str += std::to_string(port);
    }

    return str;
}



Host::Host()
{
    session = ssh_new();
    if (!session)
        throw std::runtime_error("Failed to create ssh_session");
}

Host::~Host()
{
    ssh_free(session);
}

void Host::connect(const HostInfo& info)
{
    this->info = info;

    // set options
    ssh_options_set(session, SSH_OPTIONS_HOST, info.hostName.c_str());
    ssh_options_set(session, SSH_OPTIONS_PORT, &info.port);
    if (!info.userName.empty()) {
        ssh_options_set(session, SSH_OPTIONS_USER, info.userName.c_str());
    }

    // connect
    int rc = ssh_connect(session);
    if (rc != SSH_OK) {
        sshException("Failed to connect to " + info.hostName);
    }

    //authHost();   // TODO
    authUser();
}

void Host::authHost()
{
    // mostly copy and pasted from libssh examples
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char buf[10];
    char *hexa;
    char *p;
    int cmp;
    int rc;
    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        sshException("Failed to get server public key");
    }
    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA256,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        sshException("Failed to get public key hash");
    }
    state = ssh_session_is_known_server(session);
    switch (state) {
        case SSH_KNOWN_HOSTS_OK:
            /* OK */
            break;
        case SSH_KNOWN_HOSTS_CHANGED:
            fprintf(stderr, "Host key for server changed: it is now:\n");
            ssh_print_hexa("Public key hash", hash, hlen);
            fprintf(stderr, "For security reasons, connection will be stopped\n");
            ssh_clean_pubkey_hash(&hash);
            sshException("Host key changed");
        case SSH_KNOWN_HOSTS_OTHER:
            // libssh doesn't seem to go through all keys returned by the server
            // but rather picks just one, which might not necessarily match the
            // type in known_hosts
            fprintf(stderr, "The host key for this server was not found but another\n"
                    "type of key exists.\n\n");
            fprintf(stderr, "An attacker might change the default server key to\n"
                    "confuse your client into thinking the key does not exist\n\n");
            fprintf(stderr, "If you can confirm you are not being attacked, try\n"
                    "deleting the key from known_hosts and try again.\n\n");
            ssh_clean_pubkey_hash(&hash);
            return sshException("Host key type changed");
        case SSH_KNOWN_HOSTS_NOT_FOUND:
            fprintf(stderr, "Could not find known host file.\n");
            /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
        case SSH_KNOWN_HOSTS_UNKNOWN:
            hexa = ssh_get_hexa(hash, hlen);
            fprintf(stderr, "Public key hash: %s\n", hexa);
            fprintf(stderr, "The server is unknown. Do you trust the host key? (yes/no): \n");
            ssh_string_free_char(hexa);
            ssh_clean_pubkey_hash(&hash);
            p = fgets(buf, sizeof(buf), stdin);
            if (p == NULL) {
                throw std::runtime_error("fgets returned NULL");
            }
            cmp = strncasecmp(buf, "yes", 3);
            if (cmp != 0) {
                sshException("Host key not trusted");
            }
            rc = ssh_session_update_known_hosts(session);
            if (rc < 0) {
                fprintf(stderr, "Failed to update known hosts %s\n", strerror(errno));
            }
            break;
        case SSH_KNOWN_HOSTS_ERROR:
            ssh_clean_pubkey_hash(&hash);
            sshException("Failed to get known hosts");
    }
    ssh_clean_pubkey_hash(&hash);
}

void Host::authUser()
{
    // TODO: doesn't work well for public keys

    // try authenticating with public key first
    int rc;
    while ((rc = ssh_userauth_publickey_auto(session, nullptr, nullptr)) == SSH_AUTH_AGAIN)
    {
    }

    if (rc == SSH_AUTH_ERROR)
        sshException("Error while trying public key authentication");

    if (rc == SSH_AUTH_SUCCESS)
        return;

    // else we need to try another authentication method

    // password authentication
    std::string prompt = "Enter password for " + info.toString();
    char* password = getpass(prompt.c_str());
    rc = ssh_userauth_password(session, nullptr, password);
    
    // zero the password for security
    memset(password, 0, strlen(password));

    if (rc == SSH_AUTH_ERROR)
        sshException("Error while trying password authentication");

    if (rc == SSH_AUTH_SUCCESS)
        return;

    // else we need to try another authentication method

    // keyboard interactive authentication
    rc = authenticate_kbdint(session);

    if (rc == SSH_AUTH_ERROR)
        sshException("Error while trying keyboard interactive authentication");
    if (rc != SSH_AUTH_SUCCESS)
        sshException("Failed to authenticate user");
}

void Host::sshException(const std::string& what)
{
    const char* sshErr = ssh_get_error(session);
    if (sshErr == nullptr || strlen(sshErr) == 0) {
        throw std::runtime_error(what);
    }
    throw std::runtime_error(what + ": " + sshErr);
}

/**
 * Copied from libssh documentation to handle keyboard interactive authentication
 */
static int authenticate_kbdint(ssh_session session)
{
    int rc;
    rc = ssh_userauth_kbdint(session, NULL, NULL);
    while (rc == SSH_AUTH_INFO)
    {
        const char *name, *instruction;
        int nprompts, iprompt;
        name = ssh_userauth_kbdint_getname(session);
        instruction = ssh_userauth_kbdint_getinstruction(session);
        nprompts = ssh_userauth_kbdint_getnprompts(session);
        if (strlen(name) > 0)
            printf("%s\n", name);
        if (strlen(instruction) > 0)
            printf("%s\n", instruction);
        for (iprompt = 0; iprompt < nprompts; iprompt++)
        {
            const char *prompt;
            char echo;
            prompt = ssh_userauth_kbdint_getprompt(session, iprompt, &echo);
            if (echo)
            {
                char buffer[128], *ptr;
                printf("%s", prompt);
                if (fgets(buffer, sizeof(buffer), stdin) == NULL)
                    return SSH_AUTH_ERROR;
                buffer[sizeof(buffer) - 1] = '\0';
                if ((ptr = strchr(buffer, '\n')) != NULL)
                    *ptr = '\0';
                if (ssh_userauth_kbdint_setanswer(session, iprompt, buffer) < 0)
                    return SSH_AUTH_ERROR;
                memset(buffer, 0, strlen(buffer));
            }
            else
            {
                char *ptr;
                ptr = getpass(prompt);
                if (ssh_userauth_kbdint_setanswer(session, iprompt, ptr) < 0)
                    return SSH_AUTH_ERROR;
            }
        }
        rc = ssh_userauth_kbdint(session, NULL, NULL);
    }
    return rc;
}
