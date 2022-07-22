// A simple server in the internet domain using TCP
// The port number is passed as an argument
// implementation code has been taken from sockets Week 7 tutorial

#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <strings.h>
#include <string.h>
#include <signal.h>

#include "server-helper.h"

#define MULTITHREADED
#define IMPLEMENTS_IPV6

// max connections
#define BACKLOG 10
#define MAX_PATH 2048

// struct for passing thread arguments
struct serverPThread{
	int nsockfd;
	char *root_path;
};

// thread function for handling requests on a thread
void *thread_action(void* arg);

// starts server after receiving the protocol, port and root path
void start_server(char protocol, const char *port, char* rootpath);

int main(int argc, char** argv) {

	if (argc < 4) {
		fprintf(stderr, "ERROR, missing info\n");
		exit(EXIT_FAILURE);
	}

    char protocol = *argv[1];
    const char *port = argv[2];
    char rootpath[MAX_PATH];
    
    strcpy(rootpath,argv[3]);
    
    // start the server by sending the protocol, port and rootpath
    start_server(protocol, port, rootpath);

	return 0;
}

// starts server after receiving the protocol, port and root path
void start_server(char protocol, const char *port, char* rootpath){
	int sockfd, enable, s;
	struct addrinfo hints, *res, *p;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
    
	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);

    // IPv4
    if(protocol=='4'){
        hints.ai_family = AF_INET;       
    }
    // IPv6
    else if(protocol=='6'){
        hints.ai_family = AF_INET6;       
    }

    // TCP
	hints.ai_socktype = SOCK_STREAM;

    // for bind, listen, accept
	hints.ai_flags = AI_PASSIVE;     

	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, port, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

    // for ipv4
    if(protocol == '4'){   
        p = res;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    }

    // for ipv6
    if(protocol == '6'){
        for(p = res; p != NULL; p = p->ai_next){
            if (p->ai_family == AF_INET6
            && (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0 ){
                // socket creation was attempted but failed
                perror("socket");
                exit(EXIT_FAILURE);
            } 
        }
        // Create socket
    }

    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // prevent SIGNPIPE signal error
    signal(SIGPIPE, SIG_IGN);
	
    // Reuse port if possible
    enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind address to the socket
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        close(sockfd);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen on socket - means we're ready to accept connections,
    // incoming connection requests will be queued, man 3 listen
    if (listen(sockfd, BACKLOG) < 0) {
        close(sockfd);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    client_addr_size = sizeof client_addr;

    while(1){

        struct serverPThread *serverThread = malloc(sizeof(struct serverPThread));
        if (serverThread == NULL) {
            perror("thread");
            exit(EXIT_FAILURE);
        }

        // Accept a connection - blocks until a connection is ready to be accepted
        // Get back a new file descriptor to communicate on
        // this file descriptor is passed to the server thread arguments
        serverThread->nsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (serverThread->nsockfd< 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // store the root path in the server thread arguments
        serverThread->root_path = rootpath;

        // thread identifier 
        pthread_t tid;

        // creates the thread 
        if(pthread_create(&tid, NULL, thread_action, serverThread)!=0){
            perror("pthread_create() error");
        }
        printf("with thread %ld\n", (long int) tid);
    }
}

// thread function to handle requests on a thread
void *thread_action(void* serverThread){

    // detachs thread  to free its resources/ agruments
    pthread_detach(pthread_self());

	int  newsockfd;
    char root[BUFFER_LEN];
    char filepath[BUFFER_LEN];

	// get the thread arguments
	strcpy(root,((struct serverPThread *) serverThread)->root_path);
	newsockfd = ((struct serverPThread *) serverThread)->nsockfd;
    free(serverThread);

    // parse and serves a valid response to a valid request
    if(parseReq(newsockfd, filepath, root) > 0){
        serveReq(newsockfd,root, filepath);
    } 

    close(newsockfd);
	
    serverThread = NULL;
    pthread_exit(NULL);
    return NULL;
}


