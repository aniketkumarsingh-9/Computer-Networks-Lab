#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>   
#define MAX 80
#define PORT 8080
#define SA struct sockaddr
#define MAX_CLIENTS 5   

// ip address - 192.168.0.221

int client_count = 0;          
pthread_mutex_t lock;          // Mutex to protect client_count

// Chat function between client and server
void *func(void *arg)
{
    int connfd = *((int *)arg);
    free(arg); 

    char buff[MAX];
    int n;

    for (;;) {
        bzero(buff, MAX);
        int bytes_read = read(connfd, buff, sizeof(buff));
        if (bytes_read <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        printf("From client: %s\t To client : ", buff);
        bzero(buff, MAX);
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;

        write(connfd, buff, sizeof(buff));

        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit for this client...\n");
            break;
        }
    }

    close(connfd);

    pthread_mutex_lock(&lock);
    client_count--;  // Decrease active client count
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    socklen_t len;

    pthread_mutex_init(&lock, NULL);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Listening
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

    len = sizeof(cli);

    // Accept multiple clients
    while (1) {
        connfd = accept(sockfd, (SA *)&cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        }

        pthread_mutex_lock(&lock);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&lock);
            printf("Max clients reached. Rejecting new connection.\n");
            close(connfd);
            continue;
        }
        client_count++;
        pthread_mutex_unlock(&lock);

        printf("Client connected. Active clients: %d\n", client_count);

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_create(&tid, NULL, func, pclient);
        pthread_detach(tid); // Automatically free resources after thread ends
    }

    close(sockfd);
    pthread_mutex_destroy(&lock);
    return 0;
}
