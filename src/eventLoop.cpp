#include "eventLoop.hpp"
#include <poll.h>
#include <fcntl.h>
#include <stdexcept>

EventLoop::EventLoop()
{
    if (pipe2(pipefd, O_CLOEXEC) != 0) {
        throw std::runtime_error("pipe() failed");
    }

    evt = ssh_event_new();
    ssh_event_add_fd(evt, pipefd[0], POLLIN, &EventLoop::onPollFd, this);
}

EventLoop::~EventLoop()
{
    ssh_event_free(evt);

    close(pipefd[0]);
    close(pipefd[1]);
}

void EventLoop::run()
{
    exit = false;
    while (!exit) {
        ssh_event_dopoll(evt, -1);
    }
}

void EventLoop::stop()
{
    enqueueTask([this] () { exit = true; } );
}

void EventLoop::addSession(ssh_session session)
{
    enqueueTask([this, session] () {
        ssh_event_add_session(evt, session);
    });
}

void EventLoop::removeSession(ssh_session session)
{
    enqueueTask([this, session] () {
        ssh_event_remove_session(evt, session);
    });
}

void EventLoop::addConnector(ssh_connector conn)
{
    enqueueTask([this, conn] () {
        ssh_event_add_connector(evt, conn);
    });
}

void EventLoop::removeConnector(ssh_connector conn)
{
    enqueueTask([this, conn] () {
        ssh_event_remove_connector(evt, conn);
    });
}

void EventLoop::enqueueTask(EventLoop::Task t)
{
    std::lock_guard lck(taskQueueMtx);
    taskQueue.push_back(t);

    // interrupt the event loop
    char c = 0;
    write(pipefd[1], &c, 1);
}

int EventLoop::onPollFd(socket_t fd, int revents, void* userdata)
{
    // clear pipe
    char buf[128];
    read(fd, buf, sizeof(buf));

    // run tasks
    ((EventLoop*)userdata)->runTasks();

    return SSH_OK;  // ???
}

void EventLoop::runTasks()
{
    std::unique_lock lck(taskQueueMtx);
    while (!taskQueue.empty()) {
        auto tsk = taskQueue.front();
        taskQueue.pop_front();
        lck.unlock();
        tsk();
        lck.lock();
    }
}
