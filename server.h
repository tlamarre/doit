/*
 * server.h
 *
 * declares structs intended to be serialized and sent as messages
 * between components of the doit system, and the methods for 
 * accomplishing this.
 */

#ifndef _SERVER_H
#define _SERVER_H
#include "prelude.h"

typedef struct _serverId {
    char *hostname;
    int port;
} serverId;

typedef struct _clientAnnounceMessage {
    serverId sender;
} clientAnnounceMessage;

typedef struct _masterAnnounceMessage {
    serverId sender;
} candidateAnnounceMessage;

typedef struct _successorAnnounceMessage {
    serverId sender;
} successorAnnounceMessage;

typedef struct _callElectionMessage {
    serverId sender;
} callElectionMessage;

typedef struct _sendNumberMessage {
    int n;
} sendNumberMessage;

typedef struct _jobRequestMessage {
    serverId sender;
} jobRequestMessage

/*
 * jobOrderMessage: contains a reference to some
 * file containing the source code to be executed
 * for a job, and fields for information associated
 * with a job
 */

typedef struct _jobOrderMessage {
    char *jobid;

    /*
     * neededBy: array of ID references to other pending jobs
     */
    char **neededBy;
} jobOrderMessage;

char *keyValue(char *key, char *value);

char *getVal(char *key, char *info);

ssize_t recv_string(int socket, char *buffer, size_t maxlen);

ssize_t send_string(int socket, char *buffer);

int build_address(char *hostname, int port, struct sockaddr_in *inet_address);

int bulletin_set_up_listener(int port, int *listener);

int bulletin_wait_for_connection(int listen_socket, int *connection);

int bulletin_make_connection_with(char *hostname, int port, int *connection);

void bulletin_exit(int errcode);

void bulletin_send_post(int bulletin_socket);

void bulletin_sendservernote(char *server,char *message);

void bulletin_recv_post(int bulletin_socket);

void bulletin_recvnote(int bulletin_socket);


#endif
