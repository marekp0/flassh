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
    cb->channel_close_function = staticOnClose;
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

    // TODO: change to libssh connectors if they become reliable enough
    // setup info for callbacks to forward data to/from the ssh channel
    for (auto& r : ioRedirs) {
        if (r.newfd == STDIN_FILENO) {
            stdinLocalFd = r.oldfd;
            ctx->getEvtLoop()->addFdRead(r.oldfd, &RemoteProcess::forwardFdToChannel, this);
        }
        else {
            if (r.newfd == STDOUT_FILENO) {
                stdoutLocalFd = r.oldfd;
            }
            else if (r.newfd == STDERR_FILENO) {
                stderrLocalFd = r.oldfd;
            }
        }
    }

    // start the process
    int rc = ssh_channel_request_exec(channel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw std::runtime_error("ssh_channel_request_exec failed");
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

void RemoteProcess::staticOnClose(ssh_session session, ssh_channel channel, void* userdata)
{
    // forward to member function
    ((RemoteProcess*)userdata)->onClose(session, channel);
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

    // stop listening for events on the stdin FD
    ctx->getEvtLoop()->removeFdRead(stdinLocalFd);

    exitStatus = status;

    // Don't call onFinish yet, because we may receive data after getting the
    // exit status. Wait until onClose()
}

void RemoteProcess::onClose(ssh_session session, ssh_channel channel)
{
    // cleanup channel
    ssh_set_blocking(session, 0);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    channel = nullptr;
    ssh_set_blocking(session, 1);

    if (onFinish)
        onFinish(exitStatus);
}

int RemoteProcess::forwardFdToChannel(int fd, int revents, void* userdata)
{
    char buf[4096];
    RemoteProcess* pThis = (RemoteProcess*) userdata;
    ssh_channel channel = pThis->channel;

    // should not block because this is being called by the libssh event loop
    int len = read(fd, buf, sizeof(buf));
    if (len < 0)
        return SSH_ERROR;       // ???
    else if (len == 0) {
        ssh_channel_send_eof(channel);
        pThis->ctx->getEvtLoop()->removeFdRead(fd);
    }
    else {
        ssh_channel_write(channel, buf, len);
    }

    return SSH_OK;
}
