#pragma once

#include <functional>
#include <libssh/libssh.h>
#include <mutex>
#include <deque>

/**
 * Wrapper around the libssh event loop to make asynchronous stuff easier.
 */
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void run();
    void stop();

    void addSession(ssh_session session);
    void removeSession(ssh_session session);

    void addConnector(ssh_connector conn);
    void removeConnector(ssh_connector conn);

    /**
     * A task that should be run on the event loop thread
     */
    typedef std::function<void()> Task;

    /**
     * Executes the task on the event loop thread
     */
    void enqueueTask(Task t);

private:
    ssh_event evt;
    bool exit = true;

    std::deque<Task> taskQueue;
    std::mutex taskQueueMtx;

    // pipe used to interrupt the event loop when we get a new task
    int pipefd[2];

    static int onPollFd(socket_t fd, int revents, void* userdata);

    // runs all queued tasks
    void runTasks();
};
