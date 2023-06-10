#ifndef PROACTOR_H
#define PROACTOR_H

#include <stdbool.h>

#define MAX_EVENTS 10

typedef void (*pst_handler_t)(int);

typedef struct proactor_t* pst_proactor_t;

pst_proactor_t create_proactor(void);

int add_fd(pst_proactor_t proactor, int fd, pst_handler_t handler);

int remove_fd(pst_proactor_t proactor, int fd);

int run_proactor(pst_proactor_t proactor);

void cancel_proactor(pst_proactor_t proactor);

#endif