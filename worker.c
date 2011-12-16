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

serverId *worker_id = serverId(gethostname(), atoi(argv[1]));
boolean waiting_for_job = TRUE;

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
    waiting_for_job = FALSE;
    jobDescriptor *job;
    job = (job *)malloc(sizeof(jobDescriptor);
    job->sender->hostname = getVal("hostname", note);
    job->sender->port = getVal("port", note);
    job->sourceFilename = getVal("src", note);
    job->outputFilename = getVal("out", note);
    pthread_create(&thread, NULL, handle_job_order, (void *)job);
  } else if (strcmp(messageType, "noJob") {
    waiting_for_job = TRUE;
  }
}

void handle_job_order(jobDescriptor *job, serverId *sender) {
  char *src = job->sourceFilename;
  char *out = job->outputFilename;
  char *cmd = strcat("/usr/bin/python ", src, " >> ", out);
  int result = system(cmd);
  char *reply = strcat("jobResult$$", keyValue("job", job->id), keyValue("result", itoa(result)));
  bulletin_sendservernote(sender->hostname, sender->port, reply);
}

serverId *lookup_manager() {
  FILE *server_directory;
  server_directory = fopen("server_directory.txt", "r");
  char server_dir_str[1028];
  fgets(server_dir_str, 1028, server_directory);

  char *server_id_str[64];

  int i = 0;
  while (server_dir_str[i] != EOF && server_dir_str[i] != EOL){
    server_id_str[i] = server_dir_str[i];
    // read lines, test connections to server with can_contact_server
  }
}

boolean can_contact_server(serverId *server) {
    int connection;
    int connect_result;
    connect_result = bulletin_make_connection_with(server->hostname, server->port, &connection);
    if (connect_result < 0) {
      return FALSE;
    } else {
      close(connection);
      return TRUE;
    }
}

int main(int argc, char **argv) {
  int connection, listener;
  int connect_result;
  job_dict_t *jobD;

  connect_result = bulletin_set_up_listener(atoi(argv[1]),&listener);
  if (connect_result < 0) bulletin_exit(connect_result);

  while (1) {
    if (waiting_for_work) {
      serverId *worker_manager = lookup_manager();
      char *message = strcat("jobRequest$$", keyValue("hostname", worker_id->hostname), keyValue("port", itoa(worker_id->port)));
      bulletin_sendservernote(worker_manager->hostname, worker_manager->port, message);
    }

    connect_result = bulletin_wait_for_connection(listener,&connection);
    if (connect_result < 0) bulletin_exit(connect_result);
    worker_recvnote(int socket);
    close(connection);
  }
}
