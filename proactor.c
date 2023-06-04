#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "proactor.h"

#define MAX_EVENTS 10

typedef struct event_t {
    int fd;
    pst_handler_t handler;
} event_t;

struct proactor_t {
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
    int num_events;
    event_t* event_handlers[MAX_EVENTS];
};

pst_proactor_t create_proactor() {
    pst_proactor_t proactor = (pst_proactor_t)malloc(sizeof(struct proactor_t));
    if (proactor == NULL) {
        return NULL;
    }

    proactor->epoll_fd = epoll_create1(0);
    if (proactor->epoll_fd == -1) {
        free(proactor);
        return NULL;
    }

    proactor->num_events = 0;
    for (int i = 0; i < MAX_EVENTS; i++) {
        proactor->event_handlers[i] = NULL;
    }

    return proactor;
}

int add_fd(pst_proactor_t proactor, int fd, pst_handler_t handler) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(proactor->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        return -1;
    }

    event_t* event_handler = (event_t*)malloc(sizeof(event_t));
    if (event_handler == NULL) {
        return -1;
    }

    event_handler->fd = fd;
    event_handler->handler = handler;

    proactor->event_handlers[proactor->num_events++] = event_handler;

    return 0;
}

int remove_fd(pst_proactor_t proactor, int fd) {
    if (epoll_ctl(proactor->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        return -1;
    }

    for (int i = 0; i < proactor->num_events; i++) {
        event_t* event_handler = proactor->event_handlers[i];
        if (event_handler->fd == fd) {
            free(event_handler);
            for (int j = i + 1; j < proactor->num_events; j++) {
                proactor->event_handlers[j - 1] = proactor->event_handlers[j];
            }
            proactor->event_handlers[--proactor->num_events] = NULL;
            break;
        }
    }

    return 0;
}

bool is_fd_ready(pst_proactor_t proactor, int fd) {
    for (int i = 0; i < proactor->num_events; i++) {
        event_t* event_handler = proactor->event_handlers[i];
        if (event_handler->fd == fd) {
            return proactor->events[i].events & EPOLLIN;
        }
    }
    return false;
}

int run_proactor(pst_proactor_t proactor) {
    int num_ready_fds = epoll_wait(proactor->epoll_fd, proactor->events, MAX_EVENTS, -1);
    if (num_ready_fds == -1) {
        return -1;
    }

    for (int i = 0; i < num_ready_fds; i++) {
        int fd = proactor->events[i].data.fd;
        pst_handler_t handler = NULL;
        for (int j = 0; j < proactor->num_events; j++) {
            event_t* event_handler = proactor->event_handlers[j];
            if (event_handler->fd == fd) {
                handler = event_handler->handler;
                break;
            }
        }
        if (handler != NULL) {
            handler(fd);
        }
    }

    return 0;
}

void cancel_proactor(pst_proactor_t proactor) {
    for (int i = 0; i < proactor->num_events; i++) {
        event_t* event_handler = proactor->event_handlers[i];
        free(event_handler);
        proactor->event_handlers[i] = NULL;
    }
    proactor->num_events = 0;
    close(proactor->epoll_fd);
    free(proactor);
}
