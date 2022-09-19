// gcc client.c -I/usr/local/Cellar/openssl@1.1/1.1.1k/include -L/usr/local/Cellar/openssl@1.1/1.1.1k/lib/ -lssl -lcrypto -o client.out

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

#define MAX                 256
#define KEY_SIZE            4096

#define BRED                "\e[1;31m"
#define BGRN                "\e[1;32m"
#define BYEL                "\e[1;33m"
#define BBLU                "\e[1;34m"
#define BMAG                "\e[1;35m"
#define BCYN                "\e[1;36m"
#define reset               "\e[0m"

typedef struct sockaddr     SA;
typedef struct sockaddr_in  SAI;

RSA*                        this_client;
RSA*                        other_client;

int check_args(int argc) {
    if(argc < 5) printf(BRED "Insufficient arguments\n" reset);
    else if(argc > 5) printf(BRED "Too many arguments\n" reset);
    else return 0;
    printf("Run the client in the following way - \n" BGRN "./client.out <server_IP> <server_port> <this_private_key> <other_public_key>\n" reset);
    return 1;
}

void custom_sig(int sig) {
    exit(EXIT_SUCCESS);
}

RSA *get_rsa(char *keyfile, int public) {
    
    FILE *fk = fopen(keyfile, "rb");

    if(fk == NULL) {
        printf(BRED "error: " reset "Unable to open file %s\n", keyfile);
        return NULL;
    }

    RSA *rsa = RSA_new();

    if(public) {
        rsa = PEM_read_RSA_PUBKEY(fk, &rsa, NULL, NULL);
    }
    else {
        rsa = PEM_read_RSAPrivateKey(fk, &rsa, NULL, NULL);
    }

    if(rsa == NULL) {
        printf(BRED "error: " reset "Could not read the %s key from %s.\n", public ? "public" : "private", keyfile);
        return NULL;
    }

    fclose(fk);
    return rsa;
}

int communicate(int server) {
    char data[MAX];
    char recd[RSA_size(this_client)], enc[RSA_size(other_client)], dec[RSA_size(this_client)];

    pid_t sender, receiver;

    if(!(sender = fork())) {
        signal(SIGUSR1, custom_sig);
        while(1) {
            bzero(&data, sizeof(data));
            printf(BCYN "you> " reset);
            fgets(data, sizeof(data), stdin);
            data[strlen(data) - 1] = '\0';

            if(RSA_public_encrypt(RSA_size(other_client) - 42, data, enc, other_client, RSA_PKCS1_OAEP_PADDING) == -1) {
                printf(BRED "Encryption error\n" reset);
                exit(EXIT_FAILURE);
            }

            write(server, enc, RSA_size(other_client));

            if(strcmp(data, "exit") == 0) {
                exit(EXIT_SUCCESS);
            }
        }

        exit(EXIT_SUCCESS);
    }

    if(!(receiver = fork())) {
        signal(SIGUSR1, custom_sig);
        while(1) {
            bzero(&dec, sizeof(dec));

            if(read(server, recd, RSA_size(this_client)) <= 0) {
                exit(EXIT_FAILURE);
            }

            if(RSA_private_decrypt(RSA_size(this_client), recd, dec, this_client, RSA_PKCS1_OAEP_PADDING) == -1) {
                printf(BRED "Decryption error\n" reset);
                exit(EXIT_FAILURE);
            }
            dec[strlen(dec)] = '\0';

            if(strcmp(dec, "exit") == 0) {
                exit(EXIT_SUCCESS);
            }
            printf(BCYN "\nFrom other client:\nEncrypted:\n" reset);
            fwrite(recd, sizeof(*recd), RSA_size(this_client), stdout);
            printf(BCYN "\nDecrypted:\n" reset "%s\n" BCYN "you> " reset, dec);
            // printf("\nFrom other client: %s\nyou> ", dec);
            fflush(stdout);
        }

        exit(EXIT_SUCCESS);
    }

    wait(NULL);
    kill(sender, SIGUSR1);
    kill(receiver, SIGUSR1);

    return 0;
}

int main(int argc, char **argv) {
    if(check_args(argc)) exit(EXIT_FAILURE);

    this_client = get_rsa(argv[3], 0);
    other_client = get_rsa(argv[4], 1);

    // create socket
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (socket_fd == -1) { 
		printf(BRED "error: " reset "socket creation failed...\n"); 
		exit(EXIT_FAILURE); 
	}

    SAI server_addr, client_addr;

    bzero(&server_addr, sizeof(server_addr));

    // assign IP, PORT 
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = inet_addr(argv[1]); 
    server_addr.sin_port = htons(atoi(argv[2])); 

    printf("Connecting to %s:%d...\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    // connect the client socket to server socket 
    if (connect(socket_fd, (SA*)&server_addr, sizeof(server_addr)) != 0) { 
        printf(BRED "Server unreachable. " reset "Check server address/port and try again...\n"); 
        exit(EXIT_FAILURE);
    }

    char buff[20];
    bzero(&buff, sizeof(buff));
    if(read(socket_fd, buff, sizeof(buff)) <= 0) {
        printf(BRED "error: " reset "unknown\n");
    }

    if(strcmp(buff, "SLOT_FULL_ERR") == 0) {
        printf(BRED "error: " reset "server slots full. try again later\n");
    }
    else {
        printf(BGRN "%s\n" reset, buff);
        communicate(socket_fd);
    }

	// close the socket 
	close(socket_fd); 
    printf(BRED "\nBYE\n" reset);

    return 0;
}