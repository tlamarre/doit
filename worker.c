#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include "server.h"
#include "worker.h"
#include "jobDictionary.h"

#define TCP_PROTOCOL 6

#define BULLETIN_TERMINATE             ("STOP")

#define BULLETIN_OK                      0
#define BULLETIN_HOSTNAME_LOOKUP_ERROR (-1)
#define BULLETIN_PORT_IN_USE_ERROR     (-2)
#define BULLETIN_CONNECT_ERROR         (-3)
#define BULLETIN_TALK_ERROR            (-4)

serverId *workerId = serverId(gethostname(), atoi(argv[1]));

void worker_recvnote(int socket) {
  char buffer[512];
  int length;
  char *note;
  char *messageType;
  int t = 0;

  note = (char *)malloc(sizeof(buffer));
  messageType = (char *)malloc(sizeof(char)*64);
  pthread_t *thread;
  length = recv_string(bulletin_socket,buffer,511);
  close(socket);
  strcpy(note,buffer);

  messageType = strtok(note, "$$");
  
  if(strcmp(messageType, "jobOrder")) {
    jobDescriptor *job;
    job = (job *)malloc(sizeof(jobDescriptor);
    job->sender->hostname = getVal("hostname", note);
    job->sender->port = getVal("port", note);
    job->sourceFilename = getVal("src", note);
    job->outputFilename = getVal("out", note);
    pthread_create(&thread, NULL, handleJobOrder, (void *)job);
  }
}

void handleJobOrder(jobDescriptor *job, serverId *sender) {
  char *srcFile = job->sourceFilename;
  char *outFile = job->outputFilename;
  char *cmd = strcat("/usr/bin/python ", srcFile, " >> ", outFile);
  int result = system(cmd);
  char *reply = strcat("jobResult$$", keyValue("job", job->id), keyValue("result", itoa(result)));
  bulletin_sendservernote(sender->hostname, sender->port, reply);
}

int main(int argc, char **argv) {
  int connection, listener;
  int connect_result;
  job_dict_t *jobD;

  while (1) {
    connect_result = bulletin_wait_for_connection(listener,&connection);
    if (connect_result < 0) bulletin_exit(connect_result);
    worker_recvnote(int socket);
    close(connection);
  }
}
