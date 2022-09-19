// gcc server.c -I/usr/local/Cellar/openssl@1.1/1.1.1k/include -L/usr/local/Cellar/openssl@1.1/1.1.1k/lib/ -lssl -lcrypto -o server.out

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAX                 512
#define MAX_CLIENTS         2
#define SERVER_ADDRESS      "127.0.0.1"

typedef struct sockaddr     SA;
typedef struct sockaddr_in  SAI;
typedef struct {
    int     conn_fd[MAX_CLIENTS];
    char    data[MAX_CLIENTS][MAX];
    SAI     client_addr[MAX_CLIENTS];
} CLIENT_DATA;

CLIENT_DATA                 *clients;
int                         num_conn;

void custom_sig(int sig) {
    exit(EXIT_SUCCESS);
}

void *communicate(void* param) {

    unsigned long long int client = (unsigned long long int) param;
    while(1) {
        bzero(&clients->data[1 - client], sizeof(clients->data[1 - client]));
        if(read(clients->conn_fd[client], clients->data[1 - client], sizeof(clients->data[1 - client])) <= 0) {
            break;
        }
        printf("Data received from %s:%d, sending to %s:%d\n", inet_ntoa(clients->client_addr[client].sin_addr), ntohs(clients->client_addr[client].sin_port), inet_ntoa(clients->client_addr[1 - client].sin_addr), ntohs(clients->client_addr[1 - client].sin_port));
        write(clients->conn_fd[1 - client], clients->data[1 - client], sizeof(clients->data[1 - client]));
    }

    close(clients->conn_fd[client]);
    num_conn--;
    return NULL;
}

int main(int argc, char **argv)
{
    signal(SIGUSR1, custom_sig);
    clients = (CLIENT_DATA *) malloc(sizeof(CLIENT_DATA));
    clients->conn_fd[1] = 0;
    num_conn = 0;

    int PORT;
    if (argc < 2)
    {
        printf("Enter Port number: ");
        scanf("%d", &PORT);
    }
    else
    {
        PORT = atoi(argv[1]);
    }

    SAI server_addr;

    // create socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        printf("Socket creation failed...\n");
        exit(EXIT_FAILURE);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_port = htons(PORT);

    if ((bind(socket_fd, (SA *)&server_addr, sizeof(server_addr))) != 0)
    {
        printf("Socket bind failed...\n");
        exit(EXIT_FAILURE);
    }

    // listen with queue of 5
    if ((listen(socket_fd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(EXIT_FAILURE);
    }

    printf("Server initialised successfully\nAddress - %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    if(!fork()) {
        char buff[MAX];
        while(1) {
            scanf("%s", buff);
            if(strcmp(buff, "exit") == 0) {
                kill(getppid(), SIGUSR1);
                exit(EXIT_SUCCESS);
            }
        }

    }

    int cur = 0;

    pthread_t tid[2];
    int temp_conn_fd;
    SAI temp_cl_addr;
    while(1) {
        unsigned long long int param = cur % MAX_CLIENTS;
        printf("Waiting for a connection...\n");
        int len = sizeof(clients->client_addr[param]);
        temp_conn_fd = accept(socket_fd, (SA*)&temp_cl_addr, &len);
        if(temp_conn_fd < 0) {
            printf("Server acccept failed...\n");
            continue;
        }

        if(num_conn == 2) {
            write(temp_conn_fd, "SLOT_FULL_ERR", 13);
            close(temp_conn_fd);
            continue;
        }
        write(temp_conn_fd, "CONNECTED!", 10);
        clients->conn_fd[param] = temp_conn_fd;
        clients->client_addr[param] = temp_cl_addr;
        printf("Client %s:%d connected\n", inet_ntoa(clients->client_addr[param].sin_addr), ntohs(clients->client_addr[param].sin_port));
        // printf("Client %s:%d connected\n", inet_ntoa(temp_cl_addr.sin_addr), ntohs(temp_cl_addr.sin_port));
        num_conn++;

        pthread_create(&tid[param], NULL, communicate, (void *) param);
        cur++;
    }

    close(socket_fd);

    return 0;
}