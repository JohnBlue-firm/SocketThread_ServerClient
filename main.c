
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define PORT 8080
#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 1024

void *clientHandler(void *arg) {
    int clientSocket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Receive data from client
    if (recv(clientSocket, buffer, BUFFER_SIZE, 0) <= 0) {
        perror("Error: Receive failed");
    } else {
        printf("Message from client: %s\n", buffer);
    }

    // Close socket
    close(clientSocket);

    pthread_exit(NULL);
}

void *server_side(void *arg) {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    pthread_t tid;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error: Server side could not create socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error: Server side bind failed");
        pthread_exit(NULL);
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CONNECTIONS) < 0) {
        perror("Error: Server side listen failed");
        pthread_exit(NULL);
    }

    printf("Server side listening on port %d\n", PORT);

    // Accept incoming connections and create a thread for each client
    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            perror("Error: Server side accept failed");
            continue;
        }

        // Create thread for client
        if (pthread_create(&tid, NULL, clientHandler, &clientSocket) != 0) {
            perror("Error: Server side could not create thread for client");
            continue;
        }
    }

    // Close server socket
    close(serverSocket);

    pthread_exit(NULL);
}






void *sendMessage(void *arg) {
    int clientSocket = *((int *)arg);
    char buffer[BUFFER_SIZE] = "Hello, Server!";

    // Send message to server
    if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
        perror("Error: Client side send failed");
    } else {
        printf("Message sent to server: %s\n", buffer);
    }

    // Close socket
    close(clientSocket);

    pthread_exit(NULL);
}

void *client_side(void *arg) {
    int clientSocket;
    struct sockaddr_in serverAddr;
    pthread_t tid;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error: Client side could not create socket");
        pthread_exit(NULL);
    }

    // Initialize server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address
    serverAddr.sin_port = htons(PORT);

    // Connect to server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error: Client side connection failed");
        pthread_exit(NULL);
    }

    // Create thread to send message
    if (pthread_create(&tid, NULL, sendMessage, &clientSocket) != 0) {
        perror("Error: Client side could not create thread");
        pthread_exit(NULL);
    }

    // Wait for thread to complete
    pthread_join(tid, NULL);

    pthread_exit(NULL);
}



// timeout flag
sem_t timeout;
// timeout handler: to signal timeout
void stop_signal(int signum) {
    sem_post(&timeout);
}

int main()
{
    // Create server thread
    pthread_t tid;
    if (pthread_create(&tid, NULL, server_side, NULL) != 0) {
        perror("Error: Could not create Server side");
        exit(EXIT_FAILURE);
    }
    
    // sleep
    sleep(1);
    
    // Create client thread
    pthread_t clt;
    if (pthread_create(&clt, NULL, client_side, NULL) != 0) {
        perror("Error: Could not create Client side");
        exit(EXIT_FAILURE);
    }
    
    // Register the timeout handler for SIGALRM signal
    signal(SIGALRM, stop_signal);
    alarm(10); // Set a timeout of 10 seconds
    // Wait for time out signal
    sem_init(&timeout, 0, 0);
    sem_wait(&timeout);
    // Cancel threads
    pthread_cancel(tid);
    pthread_cancel(clt);
    pthread_join(tid, NULL);
    pthread_join(clt, NULL);
    
   return EXIT_SUCCESS;
}