#include "process.hpp"
#include <libssh/callbacks.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>

LocalProcess::LocalProcess(const std::vector<std::string>& args) : args(args)
{
    if (args.empty())
        throw std::invalid_argument("Tried to create process with no args");

    for (auto& argStr : args) {
        argv.push_back(argStr.c_str());
    }
    argv.push_back(nullptr);
}

void LocalProcess::run()
{
    // TODO: strerror
    pid = fork();
    if (pid == 0) {
        // child
        // TODO: pipe, I/O redirection
        execvp(argv[0], (char* const*)argv.data());

        // if we got here, then exec failed
        fprintf(stderr, "exec failed: %s\n", strerror(errno));
        exit(1);
    }
    else if (pid > 0) {
        // TODO: pipe
    }
    else {
        throw std::runtime_error("fork failed");
    }
}

int LocalProcess::wait()
{
    if (pid == 0) {
        throw std::runtime_error("Attempted to wait on pid 0");
    }

    int status;
    int rc = waitpid(pid, &status, 0);

    if (rc == -1) {
        // TODO: should this throw an exception?
        fprintf(stderr, "waitpid failed\n");
        return -1;
    }

    return status;
}



RemoteProcess::RemoteProcess(ssh_session session, const std::vector<std::string>& args)
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
    cb->channel_exit_status_function = staticOnExitStatus;
    cb->channel_data_function = staticOnData;
    ssh_callbacks_init(cb);

    if (SSH_OK != ssh_set_channel_callbacks(channel, cb)) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        channel = nullptr;
        throw std::runtime_error(ssh_get_error(session));
    }
}

void RemoteProcess::run()
{
    int rc = ssh_channel_request_exec(channel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw std::runtime_error("ssh_channel_request_exec failed");
    }
}

int RemoteProcess::wait()
{
    std::unique_lock<std::mutex> lock(exitStatusMutex);
    while (!exitStatusValid) {
        exitStatusCond.wait(lock);
    }

    // cleanup channel
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    channel = nullptr;

    return exitStatus;
}

void RemoteProcess::staticOnExitStatus(ssh_session session, ssh_channel channel, int status, void* userdata)
{
    // forward to member function
    ((RemoteProcess*)userdata)->onExitStatus(session, channel, status);
}

int RemoteProcess::staticOnData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata)
{
    // forward to member function
    return ((RemoteProcess*)userdata)->onData(session, channel, data, len, is_stderr, userdata);
}

void RemoteProcess::onExitStatus(ssh_session session, ssh_channel channel, int status)
{
    if (channel != this->channel) {
        // something has gone horribly wrong
        fprintf(stderr, "bad channel in exit status callback");
        exit(1);
    }

    // set the exit status and notify anyone waiting on the condition variable
    std::unique_lock<std::mutex> lock(exitStatusMutex);
    exitStatusValid = true;
    exitStatus = status;
    exitStatusCond.notify_all();
}

int RemoteProcess::onData(ssh_session session, ssh_channel channel, void* data, uint32_t len, int is_stderr, void* userdata)
{
    if (channel != this->channel) {
        // something has gone horribly wrong
        fprintf(stderr, "bad channel in exit status callback");
        exit(1);
    }

    // TODO: pipe
    write(STDOUT_FILENO, data, len);
    
    return len;
}
