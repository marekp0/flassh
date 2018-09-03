#include "process.hpp"
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
