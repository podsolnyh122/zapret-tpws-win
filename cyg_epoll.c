#include <sys/poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "epoll-shim/include/sys/epoll.h"

#define MAX_POLL_FDS 2048
#define MAX_EPOLL_INSTANCES 16

struct fake_epoll {
    int active;
    struct pollfd pfds[MAX_POLL_FDS];
    epoll_data_t data[MAX_POLL_FDS];
    int count;
};

static struct fake_epoll epoll_instances[MAX_EPOLL_INSTANCES];
#define EPOLL_FD_OFFSET 1000

int epoll_create(int size) {
    (void)size;
    return epoll_create1(0);
}

int epoll_create1(int flags) {
    (void)flags;
    for (int i = 0; i < MAX_EPOLL_INSTANCES; i++) {
        if (!epoll_instances[i].active) {
            memset(&epoll_instances[i], 0, sizeof(struct fake_epoll));
            epoll_instances[i].active = 1;
            return i + EPOLL_FD_OFFSET;
        }
    }
    errno = EMFILE;
    return -1;
}

static struct fake_epoll *get_ep(int epfd) {
    int idx = epfd - EPOLL_FD_OFFSET;
    if (idx < 0 || idx >= MAX_EPOLL_INSTANCES || !epoll_instances[idx].active) {
        return NULL;
    }
    return &epoll_instances[idx];
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    struct fake_epoll *ep = get_ep(epfd);
    if (!ep) { errno = EBADF; return -1; }

    short events = 0;
    if (event) {
        if (event->events & EPOLLIN)  events |= POLLIN;
        if (event->events & EPOLLOUT) events |= POLLOUT;
        if (event->events & EPOLLERR) events |= POLLERR;
        if (event->events & EPOLLHUP) events |= POLLHUP;
    }

    if (op == EPOLL_CTL_ADD) {
        if (ep->count >= MAX_POLL_FDS) { errno = ENOMEM; return -1; }
        ep->pfds[ep->count].fd = fd;
        ep->pfds[ep->count].events = events;
        ep->pfds[ep->count].revents = 0;
        ep->data[ep->count] = event->data;
        ep->count++;
        return 0;
    } else if (op == EPOLL_CTL_MOD) {
        for (int i = 0; i < ep->count; i++) {
            if (ep->pfds[i].fd == fd) {
                ep->pfds[i].events = events;
                ep->data[i] = event->data;
                return 0;
            }
        }
        errno = ENOENT; return -1;
    } else if (op == EPOLL_CTL_DEL) {
        for (int i = 0; i < ep->count; i++) {
            if (ep->pfds[i].fd == fd) {
                ep->pfds[i] = ep->pfds[ep->count - 1];
                ep->data[i] = ep->data[ep->count - 1];
                ep->count--;
                return 0;
            }
        }
        errno = ENOENT; return -1;
    }
    errno = EINVAL;
    return -1;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    struct fake_epoll *ep = get_ep(epfd);
    if (!ep) { errno = EBADF; return -1; }

    int res = poll(ep->pfds, ep->count, timeout);
    if (res <= 0) return res;

    int ready = 0;
    for (int i = 0; i < ep->count && ready < maxevents; i++) {
        if (ep->pfds[i].revents != 0) {
            uint32_t ev = 0;
            if (ep->pfds[i].revents & POLLIN)  ev |= EPOLLIN;
            if (ep->pfds[i].revents & POLLOUT) ev |= POLLOUT;
            if (ep->pfds[i].revents & POLLERR) ev |= EPOLLERR;
            if (ep->pfds[i].revents & POLLHUP) ev |= POLLHUP;

            events[ready].events = ev;
            events[ready].data = ep->data[i];
            ep->pfds[i].revents = 0;
            ready++;
        }
    }
    return ready;
}

#undef close
int epoll_shim_close(int fd) {
    int idx = fd - EPOLL_FD_OFFSET;
    if (idx >= 0 && idx < MAX_EPOLL_INSTANCES && epoll_instances[idx].active) {
        epoll_instances[idx].active = 0;
        epoll_instances[idx].count = 0;
        return 0;
    }
    return close(fd);
}