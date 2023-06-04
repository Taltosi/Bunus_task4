#ifndef PROACTOR_H
#define PROACTOR_H

#include <stdbool.h>

typedef struct proactor_t* pst_proactor_t;
typedef int (*pst_handler_t)(int);

pst_proactor_t create_proactor();

int add_fd(pst_proactor_t proactor, int fd, pst_handler_t handler);

int remove_fd(pst_proactor_t proactor, int fd);

bool is_fd_ready(pst_proactor_t proactor, int fd);

int run_proactor(pst_proactor_t proactor);

void cancel_proactor(pst_proactor_t proactor);

#endif /* PROACTOR_H */
