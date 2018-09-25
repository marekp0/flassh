#include "process.hpp"
#include "context.hpp"
#include <libssh/callbacks.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <thread>

void Process::redirectIo(int fdLocal, int fdProc)
{
    ioRedirs.push_back({ fdLocal, fdProc });
}



LocalProcess::LocalProcess(const std::vector<std::string>& args) : args(args)
{
    if (args.empty())
        throw std::invalid_argument("Tried to create process with no args");

    for (auto& argStr : args) {
        argv.push_back(argStr.c_str());
    }
    argv.push_back(nullptr);
}

void LocalProcess::start(ProcessFinishedCallback onFinish)
{
    // TODO: more robust error handling
    pid = fork();
    if (pid == 0) {
        // child
        
        // apply I/O redirection
        for (auto& r : ioRedirs) {
            if (dup2(r.oldfd, r.newfd) == -1) {
                fprintf(stderr, "dup2 failed: %s\n", strerror(errno));
                exit(1);    // TODO: is there a special error code for this?
            }
        }

        execvp(argv[0], (char* const*)argv.data());

        // if we got here, then exec failed
        fprintf(stderr, "exec failed: %s\n", strerror(errno));
        exit(1);    // TODO: is there a special error code for this?
    }
    else if (pid > 0) {
        // parent

        // wait for process to finish in another thread
        std::thread t([this, onFinish] () {
            int status = -1;
            int rc = waitpid(pid, &status, 0);
            if (rc == -1) {
                fprintf(stderr, "waitpid failed\n");
            }
            pid = 0;

            if (onFinish)
                onFinish(status);
        });

        // run thread in background
        t.detach();
    }
    else {
        throw std::runtime_error("fork failed");
    }
}



RemoteProcess::RemoteProcess(ssh_session session, Context* ctx, const std::vector<std::string>& args)
    : ctx(ctx), session(session)
{
    channel = ssh_channel_new(session);
    if (channel == nullptr) {
        throw std::runtime_error(ssh_get_error(session));
    }

    int rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        ssh_channel_free(channel);
        channel = nullptr;
        throw std::runtime_error(ssh_get_error(session));
    }

    // TODO: no libssh function that takes a list of strings as args?
    // TODO: probably missing some escape sequences
    for (auto& a : args) {
        cmd += a + " ";
    }

    // setup callback so we can get exit status
    auto cb = new ssh_channel_callbacks_struct;
    memset(cb, 0, sizeof(*cb));
    cb->userdata = this;
    cb->channel_data_function = staticOnData;   // TODO: remove once libssh connectors work better
    cb->channel_exit_status_function = staticOnExitStatus;
    // TODO: signals
    ssh_callbacks_init(cb);

    if (SSH_OK != ssh_set_channel_callbacks(channel, cb)) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        channel = nullptr;
        throw std::runtime_error(ssh_get_error(session));
    }
}

void RemoteProcess::start(ProcessFinishedCallback onFinish)
{
    this->onFinish = onFinish;

    int rc = ssh_channel_request_exec(channel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw std::runtime_error("ssh_channel_request_exec failed");
    }

    // setup default connectors, validate filenos
    bool stdinRedirected = false;
    bool stdoutRedirected = false;
    bool stderrRedirected = false;
    for (auto& r : ioRedirs) {
        if (r.newfd == STDIN_FILENO)
            stdinRedirected = true;
        else if (r.newfd == STDOUT_FILENO)
            stdoutRedirected = true;
        else if (r.newfd == STDERR_FILENO)
            stderrRedirected = true;
        else {
            throw std::runtime_error("Bad fileno for remote process");
        }
    }

    if (!stdinRedirected)
        redirectIo(STDIN_FILENO, STDIN_FILENO);
    if (!stdoutRedirected)
        redirectIo(STDOUT_FILENO, STDOUT_FILENO);
    if (!stderrRedirected)
        redirectIo(STDERR_FILENO, STDERR_FILENO);

    // create connectors
    // TODO: these seem unreliable?
    for (auto& r : ioRedirs) {
        // TODO: handle errors!!!
        //ssh_connector conn = ssh_connector_new(session);
        if (r.newfd == STDIN_FILENO) {
            ssh_connector conn = ssh_connector_new(session);
            ssh_connector_set_in_fd(conn, r.oldfd);
            ssh_connector_set_out_channel(conn, channel, SSH_CONNECTOR_STDOUT);
            connectors.push_back(conn);
            ctx->getEvtLoop()->addConnector(conn);
        }
        else {
            // FIXME: libssh connectors with output file descriptors seem to
            // sometimes drop the end of the output. For now, do things manually
            if (r.newfd == STDOUT_FILENO) {
                stdoutLocalFd = r.oldfd;
            }
            else if (r.newfd == STDERR_FILENO) {
                stderrLocalFd = r.oldfd;
            }
            //auto flags = r.newfd == STDOUT_FILENO ? SSH_CONNECTOR_STDOUT : SSH_CONNECTOR_STDERR;
            //ssh_connector_set_in_channel(conn, channel, flags);
            //ssh_connector_set_out_fd(conn, r.oldfd);
        }
        //connectors.push_back(conn);
        //ctx->getEvtLoop()->addConnector(conn);
    }
}

int RemoteProcess::staticOnData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata)
{
    // forward to member function
    return ((RemoteProcess*)userdata)->onData(session, channel, data, len, is_stderr);
}

void RemoteProcess::staticOnExitStatus(ssh_session session, ssh_channel channel, int status, void* userdata)
{
    // forward to member function
    ((RemoteProcess*)userdata)->onExitStatus(session, channel, status);
}

int RemoteProcess::onData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr)
{
    if (channel != this->channel) {
        // something has gone horribly wrong
        fprintf(stderr, "bad channel in exit status callback");
        exit(1);
    }

    // forward output
    if (is_stderr) {
        write(stderrLocalFd, data, len);
    }
    else {
        write(stdoutLocalFd, data, len);
    }
    
    return len;
}

void RemoteProcess::onExitStatus(ssh_session session, ssh_channel channel, int status)
{
    if (channel != this->channel) {
        // something has gone horribly wrong
        fprintf(stderr, "bad channel in exit status callback");
        exit(1);
    }

    // cleanup connectors
    for (auto c : connectors) {
        ctx->getEvtLoop()->removeConnector(c);
        ssh_connector_free(c);
    }
    connectors.clear();

    // cleanup channel
    ssh_set_blocking(session, 0);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    channel = nullptr;
    ssh_set_blocking(session, 1);

    if (onFinish)
        onFinish(status);
}
