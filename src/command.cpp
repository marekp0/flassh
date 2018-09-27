#include "command.hpp"
#include "process.hpp"
#include "context.hpp"
#include <fcntl.h>

NopCommand::NopCommand(ProcessFinishedCallback onFinish)
    : additionalOnFinish(onFinish) {}

void NopCommand::start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish)
{
    if (additionalOnFinish)
        additionalOnFinish(0);

    if (onFinish)
        onFinish(0);
}



SimpleCommand::SimpleCommand(const std::string& hostAlias, const std::vector<std::string>& args) 
    : hostAlias(hostAlias), args(args) {}

void SimpleCommand::start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish)
{
    auto p = c->createPocess(hostAlias, args, redirs);
    p->start([c, p, onFinish] (int status) {
        onFinish(status);

        // Since the process must have a reference to this lambda, deleting
        // the process would delete this lambda while it's still running.
        // Therefore, we must make sure it gets deleted later
        c->getEvtLoop()->enqueueTask([p](){ delete p; });

        // might work without the event loop if it's the last statement in the
        // function, but this feels sketchy
        //delete p;
    });
}



PipeCommand::PipeCommand(Command* left, Command* right)
    : leftCmd(left), rightCmd(right) {}

void PipeCommand::start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish)
{
    struct PipeCmdState {
        bool leftDone = false;
        bool rightDone = false;
        int rightStatus;
        int pipefd[2];
    };

    // FIXME: memory leak on exception
    auto state = new PipeCmdState;
    if (pipe2(state->pipefd, O_CLOEXEC) == -1) {
        delete state;
        throw std::runtime_error("pipe2 failed");
    }

    std::vector<IoRedir> leftRedirs, rightRedirs;

    // redirect
    for (auto& r : redirs) {
        if (r.newfd == STDIN_FILENO) {
            leftRedirs.push_back(r);
        }
        else {
            rightRedirs.push_back(r);
        }
    }

    // I/O redirection for the pipe
    leftRedirs.push_back({ state->pipefd[1], STDOUT_FILENO });
    rightRedirs.push_back({ state->pipefd[0], STDIN_FILENO });

    leftCmd->start(c, leftRedirs, [state, onFinish] (int status) {
        state->leftDone = true;
        close(state->pipefd[1]);
        if (state->rightDone) {
            int retStatus = state->rightStatus;
            delete state;
            onFinish(retStatus);
        }
    });

    rightCmd->start(c, rightRedirs, [state, onFinish] (int status) {
        state->rightDone = true;
        close(state->pipefd[0]);
        if (state->leftDone) {
            delete state;
            onFinish(status);
        }
    });
}



NewHostCommand::NewHostCommand(const std::string& alias, const HostInfo& info) : 
    alias(alias), hostInfo(info) {}

void NewHostCommand::start(Context* c, const std::vector<IoRedir>& redirs, ProcessFinishedCallback onFinish)
{
    c->addHost(alias, hostInfo);
    onFinish(0);
}
