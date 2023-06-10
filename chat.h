#ifndef CHAT_H
#define CHAT_H

#include <stdbool.h>

#define SERVER_PORT 9034
#define BUFFER_SIZE 256
#define MAX_CLIENTS 10

// Function prototypes
void handle_new_connection(pst_proactor_t proactor, int server_fd);
void* proactor_thread(void* arg);
int add_client_fd(int client_fd);
int remove_client_fd(int client_fd);

#endif /* CHAT_H */