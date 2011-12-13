#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include "server.h"


#define TCP_PROTOCOL 6

#define BULLETIN_TERMINATE             ("STOP")

#define BULLETIN_OK                      0
#define BULLETIN_HOSTNAME_LOOKUP_ERROR (-1)
#define BULLETIN_PORT_IN_USE_ERROR     (-2)
#define BULLETIN_CONNECT_ERROR         (-3)
#define BULLETIN_TALK_ERROR            (-4)

char *keyValue(char *key, char *value) {

  char *tokenizedString;
  strcat(tokenizedString,"$$");
  strcat(tokenizedString,key);
  strcat(tokenizedString,"$$");
  strcat(tokenizedString,value);
  return tokenizedString;
}

char *getVal(char *key, char *info) {
  int j = 0;
  char *value = malloc(128);
  char *str;
  char *src = malloc(sizeof(key));

  for(int i = 0; info[i] != NULL; i++) {
    if (info[i] == key[0]) {
      str = info[i];
      strncpy(src, str, sizeof(key));
      if (strcmp(src,key)) {
        str = info[i + strlen(key) + 1];
        strncpy(value,str,128);
        value = strtok(value,"$$");
        return value;  
      }
    }
  }

  return NULL;
}

//
// recv_string
//
// Receives a series of bytes from a socket, of at most 
// maxlen, and terminated by a NULL character.  It 
// packages up the bytes and returns them in the memory 
// referenced by "buffer".
//
// It assumes that there is space for maxlen bytes in 
// the memory referenced at buffer.
//
ssize_t recv_string(int socket, char *buffer, size_t maxlen) {
  ssize_t rc;
  char c;
  int pos;

  for (pos = 0; pos <= maxlen; pos++) {
    if ((rc = read(socket, &c, 1))==1) {
      buffer[pos] = c;
      if (c==0) break;
    } else if (rc==0) {
      break;
    } else {
      if (errno != EINTR ) continue;
      return -1;
    }
  }
  buffer[pos] = 0;
  return pos;
}


//
// send_string
//
// Sends a series of messages over a socket in order
// to transmit the characters referenced by "buffer".
// The last character it sends is the NULL character
// terminating that string.
//
ssize_t send_string(int socket, char *buffer) {
  ssize_t     nwritten;
  int pos = 0;
  int len = strlen(buffer)+1;
  while (pos < len) {
    if ((nwritten = write(socket, &(buffer[pos]), len-pos)) <= 0) {
      if (errno == EINTR) {
	nwritten = 0;
      } else {
	return -1;
      }
    }
    pos += nwritten;
  }
  return pos;
}

int build_address(char *hostname, int port, struct sockaddr_in *inet_address) {
  struct hostent *host_entries;
  host_entries = gethostbyname(hostname);
  if (host_entries == NULL || host_entries->h_addrtype != AF_INET || host_entries->h_length != 4) {
    return BULLETIN_HOSTNAME_LOOKUP_ERROR;
  } else {
    // success!
    inet_address->sin_family = AF_INET;
    inet_address->sin_addr.s_addr = *((uint32_t *)host_entries->h_addr_list[0]);
    inet_address->sin_port = port;
    return BULLETIN_OK;
  }
}

int bulletin_set_up_listener(int port, int *listener) {
  int listen_socket;
  char hostname[128];
  struct sockaddr_in address;
  int lookup_result, bind_result;
  printf("Trying to set up listener on port %d.\n", port);
  // get a socket
  listen_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL); 
  if (listen_socket == -1) return BULLETIN_CONNECT_ERROR;

  if (gethostname(hostname,sizeof(hostname)) < 0) return BULLETIN_CONNECT_ERROR;
  lookup_result = build_address(hostname,port,&address);
  if (lookup_result < 0) return lookup_result;
  
  // bind it to a port on this machine
  bind_result = bind(listen_socket,(struct sockaddr *)&address,sizeof(address)); 
  if (bind_result == EADDRINUSE) return BULLETIN_PORT_IN_USE_ERROR;
  if (bind_result < 0) return BULLETIN_CONNECT_ERROR;

  // return the listener's socket descriptor
  *listener = listen_socket;
  return BULLETIN_OK;
}


int bulletin_wait_for_connection(int listen_socket, int *connection) {
  int bulletin_socket;

  // listen for a client's "connect" request
  if (listen(listen_socket, 1) < 0) return BULLETIN_CONNECT_ERROR;

  // accept it, get a dedicated socket for connection with this client
  bulletin_socket = accept(listen_socket,NULL,NULL);
  if (bulletin_socket < 0) return BULLETIN_CONNECT_ERROR;

  // return that client connection's socket descriptor
  *connection = bulletin_socket;
  return BULLETIN_OK;
}
  
int bulletin_make_connection_with(char *hostname, int port, int *connection) {
  int bulletin_socket;
  struct sockaddr_in address;
  int lookup_result, connect_result;

  // grab a new socket to connect with the server
  bulletin_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL);
  if (bulletin_socket == -1) return BULLETIN_CONNECT_ERROR;

  lookup_result = build_address(hostname,port,&address);
  if (lookup_result < 0) return lookup_result;

  // connect with that server
  connect_result = connect(bulletin_socket,(struct sockaddr *)&address,sizeof(address));
  if (connect_result < 0) return BULLETIN_CONNECT_ERROR;

  // return the connection's socket descriptor
  *connection = bulletin_socket;
  return BULLETIN_OK;
}

void bulletin_exit(int errcode) {
  fprintf(stderr,"Exiting with error code %d (%d).\n",errcode, errno);
  exit(errcode);
}

void bulletin_send_post(int bulletin_socket) {
  char buffer[256];
  // repeatedly post lines typed by the user
  do {
    // read a string as a line from the console
    gets(buffer);
    // send that string on the socket to the server
    send_string(bulletin_socket,buffer);
  } while (strcmp(buffer,BULLETIN_TERMINATE));
}

void bulletin_sendservernote(char *server,int port,char *message) {
  int connection;
  int connect_result;
    connect_result = bulletin_make_connection_with(server,port,&connection);
    if (connect_result < 0) {
      printf("Could not establish connection.");
    }
    else {
      send_string(connection,message);
    }
}

void bulletin_replicate(char *server,int port,char *message,dict_t *serverD,dict_t *candidateD,int candidateNum) {
  int connection;
  int connect_result;
    connect_result = bulletin_make_connection_with(server,port,&connection);
    if (connect_result < 0) {
      findNewSuccessor(serverD,candidateD,candidateNum);
    }
    else {
      send_string(connection,message);
    }
}

void findNewSuccessor(dict_t *serverD,dict_t *candidateD,int candidateNum) {
  char *serverName;
  char *updateMessage;
  char *updateNumber;
  serverId *successor;
  serverId *master;

  updateMessage = (char *)malloc(256);
  updateNumber = (char *)malloc(128);

  serverName = getUniqueEntry(serverD,candidateD);
  changeNameOfEntry(server,"Successor",serverD);
  successor = getServerId("Successor",serverD);
  master = getServerId("Master",serverD);
  updateMessage = "candidateAnnounce$$hostname:%s$$port:%d$$";
  bulletin_sendservernote(master->hostname,master->port,updateMessage);
  updateNumber = ("becomeCandidate$$candidateNumber:%d",candidateNum + 1);
  bulletin_sendservernote(successor->hostname,successor->port,updateNumber);
}
  

void bulletin_recv_post(int bulletin_socket) {
  char buffer[256];
  int length;
  // repeatedly post lines sent by the client
    // receive a string from this client's connection socket
  do {
    length = recv_string(bulletin_socket,buffer,255);
    printf("Client says \"%s\".\n",buffer);
  }while (length >= 0 && strcmp(buffer,BULLETIN_TERMINATE)); 
}

void addNewServer(pstruct_t *pstruct) {
  char *newHost;
  serverId *newServer;
  int port;
  dict_t *serverD;

  newServer = (serverId *)malloc(sizeof(serverId));

  newHost = getVal("hostname",pstruct->note);
  port = atoi(getVal("port",pstruct->note);
  newServer->hostName = newHost;
  newServer->port = port;

  serverD = pstruct->D;
  insertServerD(newHost,serverD,newServer);
  pthread_exit(NULL);
}
  
void addNewClient(pstruct_t *pstruct) {
  char *newHost;
  serverId *newServer;
  int port;
  dict_t *serverD;

  newServer = (serverId *)malloc(sizeof(serverId));

  newHost = getVal("hostname",pstruct->note);
  port = atoi(getVal("port",pstruct->note);
  newServer->newHost = newHost;
  newServer->port = port;

  serverD = pstruct->D;
  insertServerD(newHost,serverD,newServer);
  pthread_exit(NULL);
}

void addNewSuccessor(pstruct_t *pstruct) {
  char *newHost;
  serverId *newServer;
  int port;
  dict_t *serverD;

  newServer = (serverId *)malloc(sizeof(serverId));

  newHost = getVal("hostname",pstruct->note);
  port = atoi(getVal("port",pstruct->note);
  newServer->newHost = newHost;
  newServer->port = port;

  serverD = pstruct->D;
  insertServerD("Successor",serverD,newServer);
  pthread_exit(NULL);
}

void addNewMaster(pstruct_t *pstruct) {
  char *newHost;
  serverId *newServer;
  int port;
  dict_t *serverD;

  newServer = (serverId *)malloc(sizeof(serverId));

  newHost = getVal("hostname",pstruct->note);
  port = atoi(getVal("port",pstruct->note);
  newServer->newHost = newHost;
  newServer->port = port;

  serverD = pstruct->D;
  insertServerD("Master",serverD,newServer);
  pthread_exit(NULL);
}

void handleJobRequestMessage (serverId *runner, dict_t jobD) {
  char *jobAnswer;
  jobAnswer = (char *)malloc(sizeof(char)*256);

  if (jobD->head != NULL) {
  jobAnswer = keyValue("jobId",(char *)getJobId(jobD->head->nick,jobD));
  }
  else {
  jobAnswer = "NoJob";
  }
  bulletin_sendservernote(runner->hostname,runner->port,jobAnswer);
}

void sendCandidateNumber(pstruct_t *pstruct) {
  char *hostname;
  int port;
  char *candidateMessage = (char *)malloc(64);
  
  hostname = getVal("hostname",pstruct->note);
  port = atoi(getVal("port",pstruct->note);
  candidateMessage = ("candidateNumber:%s",getVal(candidateNumber,pstruct->note));
  bulletin_sendservernote(hostname,port,candidateMessage);
  pthread_exit(NULL);
}

void propogateMessage(pstruct_t *pstruct) {
  char *hostname;
  int port;
  serverId *successor;

  successor = getServerId("Successor",pstruct->D);
  bulletin_replicate(successor->hostname,successor->port,pstruct->note,pstruct->serverD,pstruct->candidateD,pstruct->candidateNum);
}

void bulletin_recvnote(int bulletin_socket,dict_t *serverD,dict_t *candidateD,int candidateNum) {
  char buffer[512];
  int length;
  char *note;
  char *messageType;
  serverId *master;
  pstruct_t *pstruct;
  pthread_t *thread;
  int t = 0;

  if (candidateNum > 0) {
  char *propogationNote;
  propogateStruct_t *propogationStruct;
  pthread_t *propogationThread;
  }

  note = (char *)malloc(sizeof(buffer));
  messageType = (char *)malloc(sizeof(char)*64);
  pstruct = (pstruct_t *)malloc(sizeof(pstruct_t));
  length = recv_string(bulletin_socket,buffer,511);
  close(bulletin_socket);
  strcpy(note,buffer);
  if(candidateNum > 0) {
    propogationNote = (char *)malloc(sizeof(buffer));
    propogationThread = (pstruct_t *)malloc(sizeof(propogationStruct_t);
    strcpy(propogationNote,buffer);
    propogationStruct->note = propogationNote;
    propogationStruct->serverD = serverD;
    propogationStruct->candidateD = candidateD;
    propogationStruct->candidateNum = candidateNum;
  }
  printf("buffer is: %s\n",buffer);
  messageType = strtok(note,"$$");
  pstruct->note = note;
  pstruct->D = userD;
  if(strcmp(messageType,"clientAnnounce") {
    pthread_create(&thread,NULL,addNewClient,(void *)pstruct);
    if(candidateNum > 0) {
      pthread_create(&propogationThread,NULL,propogateMessage,(void *)propogationStruct);
    }
  }
  else if(strcmp(messageType,"masterAnnounce") {
    pthread_create(&thread,NULL,addNewMaster,(void *)pstruct);
    if(candidateNum > 0) {
      pthread_create(&propogationThread,NULL,propogateMessage,(void *) propogationStruct);
    }
  } 
  else if(strcmp(messageType,"successorAnnounce") {
    pthread_create(&thread,NULL,addNewSuccessor,(void *)pstruct);
    if(candidateNum > 0) {
      pthread_create(&propogationThread,NULL,propogateMessage,(void *) propogationStruct);
    }
  }
  else if(strcmp(messageType,"election") {
    if(candidateNum != 0) {
      note = strcat(note,"$$candidateNumber:%d",candidateNum);
      pthread_create(&thread,NULL,sendCandidateNumber,(void *)pstruct);
    }
  }
  else if(strcmp(messageType,"jobRequest") {
    if (candidateNum == 1) {
      pthread_create(&thread,NULL,handleJobRequestMessage,(void *)pstruct);
    }
    else {
      note = strcat(messageType,note);
      master = getServerId(userD,"Master");
      bulletin_sendservernote(master->hostname,master->port,note);
    }
  }
  else if(strcmp(messageType,"becomeCandidate") {
    candidateNum = getVal("candidateNumber",note);
    learnDictionaries();
  }
  for(t = 0;t < 256;t++) {
    buffer[t] = 0;
  }
}

void learnDictionaries () {
//Here will be the code to read dictionaries from files or otherwise learn about the 
//dictionaries in the system that the candidates are responsible for.
}

int main(int argc, char **argv) {
  int connection, listener;
  int connect_result;
  int candidateNumber = 0;
  dict_t *serverD = newD(10);
  dict_t *candidateD = newD(10);

  if (argc < 2 && argc > 3) {
    fprintf(stderr,"bulletin: two many arguments.\n");
    fprintf(stderr,"usage(1): bulletin <port>\n");
    fprintf(stderr,"\tBrings up a conversation task waiting for another to join it.\n");
    fprintf(stderr,"usage(2): bulletin <address> <port>\n");
    fprintf(stderr,"\tJoins another conversation task already waiting on the specified address.\n");
    exit(-1);
  }
  fprintf(stderr,"You have run bulletin with %d argc's\n", argc);

  if (argc == 2) {
      connect_result = bulletin_set_up_listener(atoi(argv[1]),&listener);
      if (connect_result < 0) bulletin_exit(connect_result);
    fprintf(stderr,"You have successfully chosen to be a server.\n");
    while (1) {
      connect_result = bulletin_wait_for_connection(listener,&connection);
      if (connect_result < 0) bulletin_exit(connect_result);
      bulletin_recvnote(connection, serverD, candidateD, candidateNumber);
      close(connection);
    }
  } else {
    // if we are a client

    // set up a connection with a server
    printf("You have successfully chosen to be a client.");
    connect_result = bulletin_make_connection_with(argv[1],atoi(argv[2]),&connection);
    if (connect_result < 0) bulletin_exit(connect_result);
    printf("Connected to that server!\n");
    // post to that bulletin server
    bulletin_send_post(connection);
    printf("Disconnecting from that server...\n");
    // disconnect
    close(connection);
  }
}
