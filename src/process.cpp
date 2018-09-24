#include "process.hpp"
#include <libssh/callbacks.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <thread>

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
        // TODO: pipe, I/O redirection
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
    cb->channel_data_function = staticOnData;
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

    // TODO: pipe
    write(STDOUT_FILENO, data, len);
    
    return len;
}

void RemoteProcess::onExitStatus(ssh_session session, ssh_channel channel, int status)
{
    if (channel != this->channel) {
        // something has gone horribly wrong
        fprintf(stderr, "bad channel in exit status callback");
        exit(1);
    }

    // cleanup channel
    ssh_set_blocking(session, 0);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    channel = nullptr;
    ssh_set_blocking(session, 1);

    if (onFinish)
        onFinish(status);
}
