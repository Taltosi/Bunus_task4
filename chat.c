
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "proactor.h"
#include "chat.h"

#include <netinet/in.h>

pthread_mutex_t client_fds_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_fds[MAX_CLIENTS];

void handle_client_message(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            printf("Client disconnected\n");
        } else {
            perror("recv");
        }
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    printf("Received message from client: %s\n", buffer);

    // Send message to all other clients
    pthread_mutex_lock(&client_fds_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int other_client_fd = client_fds[i];
        if (other_client_fd != -1 && other_client_fd != client_fd) {
            ssize_t bytes_sent = send(other_client_fd, buffer, strlen(buffer), 0);
            if (bytes_sent < 0) {
                perror("send");
            }
        }
    }
    pthread_mutex_unlock(&client_fds_mutex);
}

void* proactor_thread(void* arg) {
    pst_proactor_t proactor = (pst_proactor_t)arg;

    while (1) {
        // Wait for events using the proactor
        if (run_proactor(proactor) == -1) {
            perror("run_proactor");
            break;
        }
    }

    return NULL;
}

int add_client_fd(int client_fd) {
    pthread_mutex_lock(&client_fds_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] == -1) {
            client_fds[i] = client_fd;
            pthread_mutex_unlock(&client_fds_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&client_fds_mutex);
    return -1;
}

int remove_client_fd(int client_fd) {
    pthread_mutex_lock(&client_fds_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] == client_fd) {
            client_fds[i] = -1;
            pthread_mutex_unlock(&client_fds_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&client_fds_mutex);
    return -1;
}

void handle_new_connection(pst_proactor_t proactor, int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        perror("accept");
        return;
    }

    printf("New client connected\n");

    if (add_client_fd(client_fd) == -1) {
        fprintf(stderr, "Maximum number of clients reached\n");
        close(client_fd);
        return;
    }

    // Add client socket to proactor
    add_fd(proactor, client_fd, handle_client_message);
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Prepare server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // Bind socket to server address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Create proactor
    pst_proactor_t proactor = create_proactor();

    // Create proactor thread
    pthread_t proactor_thread_id;
    if (pthread_create(&proactor_thread_id, NULL, proactor_thread, proactor) != 0) {
        fprintf(stderr, "Failed to create proactor thread\n");
        exit(EXIT_FAILURE);
    }

    // Add server socket to proactor
    add_fd(proactor, server_fd, handle_new_connection);

    // Main loop
    while (1) {
        // Wait for events using the proactor
        if (run_proactor(proactor) == -1) {
            perror("run_proactor");
            break;
        }
    }

    // Cleanup
    cancel_proactor(proactor);
    pthread_join(proactor_thread_id, NULL);
    close(server_fd);

    return 0;
}