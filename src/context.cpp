#include "context.hpp"
#include "host.hpp"
#include "process.hpp"
#include "command.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

Context::Context()
{
    evtLoopThread = new std::thread([this] () { evtLoop.run(); });
}

Context::~Context()
{
    evtLoop.stop();
    evtLoopThread->join();

    // TODO: cleanup hosts
}

void Context::enqueueCommand(Command* cmd)
{
    evtLoop.enqueueTask([this, cmd] () {
        cmdQueue.push_back(cmd);

        if (!cmdExecuting)
            execNextCommand();
    });
}

void Context::flushCmdQueue()
{
    std::mutex mtx;
    std::unique_lock lck(mtx);
    std::condition_variable cv;
    bool done = false;

    enqueueCommand(new NopCommand([&done, &mtx, &cv] (int) {
        std::lock_guard lg(mtx);
        done = true;
        cv.notify_all();
    }));

    while (!done) {
        cv.wait(lck);
    }
}

Host* Context::addHost(const std::string& alias, const HostInfo& info)
{
    if (hosts.find(alias) != hosts.end()) {
        throw std::runtime_error("Host with name " + alias + " already exists");
    }

    Host* h = new Host();
    h->connect(info);
    hosts[alias] = h;
    evtLoop.addSession(h->getSession());
    return h;
}

Host* Context::getHost(const std::string& alias)
{
    auto it = hosts.find(alias);
    if (it == hosts.end()) {
        throw std::runtime_error("No host with alias " + alias);
    }
    return it->second;
}

Process* Context::createPocess(const std::string& hostAlias, const std::vector<std::string>& args)
{
    if (hostAlias.empty()) {
        return new LocalProcess(args);
    }
    else {
        Host* h = getHost(hostAlias);
        return new RemoteProcess(h->getSession(), args);
    }
}

void Context::execNextCommand()
{
    if (cmdQueue.empty())
        return;

    auto cmd = cmdQueue.front();
    cmdQueue.pop_front();
    cmdExecuting = true;
    Context* ctx = this;    // for clarity
    cmd->start(this, [ctx, cmd] (int exitStatus) {
        // this callback could be in any thread, so wrap in enqueueTask
        ctx->evtLoop.enqueueTask([ctx, cmd, exitStatus] () {
            ctx->cmdExecuting = false;
            delete cmd;
            // TODO: set exit status variable `$?`
            ctx->execNextCommand();
        });
    });
}
