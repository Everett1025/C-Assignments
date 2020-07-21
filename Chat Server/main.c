#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

#define PORT 5000
#define MAXLINE 1024
typedef struct Node
{
    int connfd;
    char *userid;
    struct Node *next;

} Node;

Node *users;

Node *initNode(int connfd, char *name)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node->userid = strdup(name);
    node->connfd = connfd;
    if (users == NULL)
    {
        users = node;
        node->next = NULL;
    }
    else
    {
        Node *user;
        Node *prev = NULL;
        for (user = users; user != NULL; prev = user, user = user->next)
        {
            if (strcmp(user->userid, node->userid) > 0)
            {
                break;
            }
        }
        node->next = user;
        if (prev)
        {
            prev->next = node;
        }
        else
        {

            users = node;
        }
    }
    return node;
}

int max(int x, int y)
{
    return (x > y ? x : y);
}
void reply(int connfd, char *message)
{
    send(connfd, message, strlen(message), 0);
}

void printUsers(int connfd)
{
    char buffer[MAXLINE * 10] = "OK!\n";
    for (Node *user = users; user != NULL; user = user->next)
    {
        strcat(buffer, user->userid);
        strcat(buffer, "\n");
    }
    reply(connfd, buffer);
}

bool validateName(char *username)
{
    int count = 0;
    for (int i = 0; username[i] != '\0'; ++i)
    {
        if (!isalnum(username[i]))
        {
            return false;
        }
        count += 1;
    }
    if (count < 4 || count > 16)
    {
        return false;
    }
    return true;
}

Node *alreadyLoggedIn(char *name)
{
    for (Node *user = users; user != NULL; user = user->next)
    {
        if (!(strcmp(user->userid, name)))
        {
            return user;
        }
    }
    return NULL;
}

bool validMessageLen(int i_length)
{
    return (1 <= i_length && i_length <= 990);
}

void *tcpProto(void *arg)
{
    Node *user = NULL;
    char buffer[MAXLINE];
    int connfd = (int)(intptr_t)arg;
    ssize_t bytes;
    ssize_t length = 0;
    while (true)
    {
        bytes = recv(connfd, buffer + length, MAXLINE - length, 0);

        if (bytes < 0)
        {
            perror("recv");
            break;
        }
        if (bytes == 0)
        {
            printf("Child %ld: Client disconnected\n", pthread_self());
            break;
        }
        else if (bytes > 0)
        {
            length += bytes;
            char *ret;
            while ((ret = strchr(buffer, '\n')) != NULL)
            {
                *ret = '\0';

                char *saveptr = NULL;
                char *token = strtok_r(buffer, " ", &saveptr);

                if (!strcmp(token, "LOGIN"))
                {
                    char *name = buffer + 6;
                    printf("Child %ld: Rcvd LOGIN request for userid %s\n", pthread_self(), name);

                    if (user != NULL)
                    {
                        reply(connfd, "ERROR Already connected\n");
                        printf("Child %ld: Sent ERROR (Already connected)\n", pthread_self());
                    }

                    else if (!validateName(name))
                    {
                        reply(connfd, "ERROR Invalid userid\n");
                        printf("Child %ld: Sent ERROR (Invalid userid)\n", pthread_self());
                    }
                    else if (alreadyLoggedIn(name))
                    {
                        reply(connfd, "ERROR Already connected\n");
                        printf("Child %ld: Sent ERROR (Already connected)\n", pthread_self());
                    }
                    else
                    {
                        user = initNode(connfd, name);

                        reply(connfd, "OK!\n");
                    }
                }
                else if (!strcmp(token, "WHO"))
                {
                    printf("Child %ld: Rcvd WHO request\n", pthread_self());
                    printUsers(connfd);
                }
                else if (!strcmp(token, "LOGOUT"))
                {
                    if (users == user)
                    {
                        users = user->next;
                    }
                    else
                    {
                        for (Node *node = users; node != NULL; node = node->next)
                        {

                            if (node->next == user)
                            {
                                node->next = user->next;
                                break;
                            }
                        }
                    }
                    free(user->userid);
                    free(user);
                    user = NULL;
                    reply(connfd, "OK!\n");
                    printf("Child %ld: Rcvd LOGOUT request\n", pthread_self());
                }
                else if (!strcmp(token, "SEND"))
                {

                    char *recipient;
                    int msgLen;
                    char *message;

                    if ((recipient = strtok_r(NULL, " ", &saveptr)) == NULL)
                    {
                        reply(connfd, "ERROR Invalid SEND format\n");
                        goto next;
                    }
                    char *msgLenStr = strtok_r(NULL, " ", &saveptr);
                    if (msgLenStr == NULL)
                    {
                        reply(connfd, "ERROR Invalid SEND format\n");
                        goto next;
                    }
                    msgLen = atoi(msgLenStr);
                    message = ret + 1;

                    if (!(validMessageLen(msgLen)))
                    {
                        fprintf(stderr, "ERROR Invalid msglen\n");
                        goto next;
                    }

                    while (message + msgLen > buffer + length)
                    {
                        bytes = recv(connfd, buffer + length, MAXLINE - length, 0);
                        if (bytes < 0)
                        {
                            perror("bytes error");
                        }
                        else if (bytes == 0)
                        {
                            break;
                        }
                        else
                        {
                            length += bytes;
                        }
                    }

                    message[msgLen] = '\0';
                    ret = message + msgLen;
                    Node *receiver = NULL;
                    printf("Child %ld: Rcvd SEND request to userid %s\n", pthread_self(), recipient);

                    if (!(receiver = alreadyLoggedIn(recipient)))
                    {
                        reply(connfd, "ERROR Unknown userid\n");
                        printf("Child %ld: Sent ERROR (Unknown userid)\n", pthread_self());
                        goto next;
                    }
                    reply(connfd, "OK!\n");

                    char *newMessage;
                    asprintf(&newMessage, "FROM %s %s %s\n", user->userid, msgLenStr, message);
                    reply(receiver->connfd, newMessage);
                    free(newMessage);
                }
                else if (!strcmp(token, "BROADCAST"))
                {

                    int msgLen;
                    char *message;

                    char *msgLenStr = strtok_r(NULL, " ", &saveptr);
                    if (msgLenStr == NULL)
                    {
                        reply(connfd, "ERROR Invalid SEND format\n");
                        goto next;
                    }
                    msgLen = atoi(msgLenStr);
                    message = ret + 1;

                    if (!(validMessageLen(msgLen)))
                    {
                        fprintf(stderr, "ERROR Invalid msglen\n");
                        goto next;
                    }

                    while (message + msgLen > buffer + length)
                    {
                        bytes = recv(connfd, buffer + length, MAXLINE - length, 0);
                        if (bytes < 0)
                        {
                            perror("bytes error");
                        }
                        else if (bytes == 0)
                        {
                            break;
                        }
                        else
                        {
                            length += bytes;
                        }
                    }
                    reply(connfd, "OK!\n");

                    message[msgLen] = '\0';
                    char *newMessage;
                    asprintf(&newMessage, "FROM %s %s %s\n", user->userid, msgLenStr, message);
                    for (Node *receiver = users; receiver != NULL; receiver = receiver->next)
                    {
                        reply(receiver->connfd, newMessage);
                    }
                    free(newMessage);
                    printf("CHILD %ld: Rcvd BROADCAST request\n", pthread_self());
                }
                else
                {
                    printf("Error not a vaild command");
                }
                int lengthFirstMsg;

            next:
                lengthFirstMsg = ret - buffer;

                memcpy(buffer, ret + 1, length - lengthFirstMsg - 1);
                length = length - lengthFirstMsg - 1;
            }
        }
    }
    return NULL;
}

void udp(int connfd)
{
    struct sockaddr addr;
    socklen_t address_len = sizeof(addr);
    char buffer[MAXLINE];
    ssize_t bytes;
    ssize_t length = 0;
    char *saveptr = NULL;

    bytes = recvfrom(connfd, buffer, MAXLINE, 0, &addr, &address_len);
    if (bytes < 0)
    {
        perror("bytes error");
        exit(EXIT_FAILURE);
    }
    length = bytes;
    connect(connfd, &addr, address_len);
    buffer[bytes] = '\0';
    char *token = strtok_r(buffer, " \n", &saveptr);
    printf("MAIN: Rcvd incoming UDP datagram from: %s\n", inet_ntoa((struct in_addr)((struct sockaddr_in *)&addr)->sin_addr));

    if (!strcmp(token, "WHO"))
    {
        printUsers(connfd);
        printf("Main: Rcvd WHO request\n");
    }
    else if (!strcmp(token, "BROADCAST"))
    {
        printf("Main: Rcvd BROADCAST request\n");

        int msgLen;
        char *message;

        char *msgLenStr = strtok_r(NULL, " ", &saveptr);
        if (msgLenStr == NULL)
        {
            reply(connfd, "ERROR Invalid SEND format\n");
            return;
        }
        msgLen = atoi(msgLenStr);
        message = strtok_r(NULL, "\n", &saveptr);

        if (!(validMessageLen(msgLen)))
        {
            fprintf(stderr, "ERROR Invalid msglen\n");
            return;
        }

        while (message + msgLen > buffer + length)
        {
            bytes = recv(connfd, buffer + length, MAXLINE - length, 0);
            if (bytes < 0)
            {
                perror("bytes error");
            }
            else if (bytes == 0)
            {
                break;
            }
            else
            {
                length += bytes;
            }
        }

        message[msgLen] = '\0';
        char *newMessage;
        asprintf(&newMessage, "FROM %s %s %s\n", "UDP-client", msgLenStr, message);
        for (Node *receiver = users; receiver != NULL; receiver = receiver->next)
        {
            reply(receiver->connfd, newMessage);

            reply(connfd, "Ok!\n");
        }
        free(newMessage);
    }

    addr.sa_family = AF_UNSPEC;
    connect(connfd, &addr, address_len);
}

int main(int argc, char *argv[])
{
    // Declare file descriptors
    int listenfd, connfd, udpfd, maxfdp1;

    // declare child thread
    //pid_t childpid;
    fd_set rset;
    //ssize_t n;
    socklen_t len;
    //const int on = 1;
    struct sockaddr_in cliaddr, servaddr;
    int reuse = 1;

    /* create listening TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0)
    {
        perror("Error");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEPORT) failed");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    // binding server addr structure to listenfd
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 10) < 0)
    {
        perror("error listen");
        exit(EXIT_FAILURE);
    }

    /* create UDP socket */
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpfd < 0)
    {
        perror("Error");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEPORT) failed");
        exit(EXIT_FAILURE);
    }
    // binding server addr structure to udp sockfd
    if (bind(udpfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }

    // clear the descriptor set
    FD_ZERO(&rset);

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Main: Started server\n");
    printf("Main: Listening for TCP connections on port: %s\n", argv[1]);
    printf("Main: Listening for UDP datagrams on port: %s\n", argv[1]);
    fflush(stdout);

    // get maxfd
    maxfdp1 = max(listenfd, udpfd) + 1;
    for (;;)
    {

        // set listenfd and udpfd in readset
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        // select the ready descriptor
        select(maxfdp1, &rset, NULL, NULL, NULL);

        // if tcp socket is readable then handle
        // it by accepting the connection
        if (FD_ISSET(listenfd, &rset))
        {
            // mAIN RCVD INCOMING CONNECTION FROM SOCKADDR
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
            printf("MAIN: Rcvd incoming TCP connection from: %s\n", inet_ntoa((struct in_addr)cliaddr.sin_addr));
            pthread_t pid;
            pthread_create(&pid, NULL, tcpProto, (void *)(intptr_t)connfd);
        }
        // if udp socket is readable receive the message.
        if (FD_ISSET(udpfd, &rset))
        {
            udp(udpfd);
        }
    }
}