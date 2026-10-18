#include <sys/poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "epoll-shim/include/sys/epoll.h"

#define MAX_POLL_FDS 1024

struct fake_epoll {
    struct pollfd pfds[MAX_POLL_FDS];
    epoll_data_t data[MAX_POLL_FDS];
    int count;
};

int epoll_create(int size) {
    (void)size;
    return epoll_create1(0);
}

int epoll_create1(int flags) {
    (void)flags;
    struct fake_epoll *ep = (struct fake_epoll*)calloc(1, sizeof(struct fake_epoll));
    if (!ep) { errno = ENOMEM; return -1; }
    return (int)(intptr_t)ep;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    struct fake_epoll *ep = (struct fake_epoll*)(intptr_t)epfd;
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
    struct fake_epoll *ep = (struct fake_epoll*)(intptr_t)epfd;
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
            if (ep->pfds[i].revents & POLLHUP) ev |= EPOLLHUP;

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
    struct fake_epoll *ep = (struct fake_epoll*)(intptr_t)fd;
    // освобождаем память
    if (ep && (uintptr_t)ep > 65535) {
        free(ep);
        return 0;
    }
    return close(fd);
}