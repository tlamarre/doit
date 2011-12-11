#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include "userDictionary.h"
#include "lazerChatServer.h"


#define TCP_PROTOCOL 6

#define BULLETIN_TERMINATE             ("STOP")

#define BULLETIN_OK                      0
#define BULLETIN_HOSTNAME_LOOKUP_ERROR (-1)
#define BULLETIN_PORT_IN_USE_ERROR     (-2)
#define BULLETIN_CONNECT_ERROR         (-3)
#define BULLETIN_TALK_ERROR            (-4)

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

void bulletin_sendservernote(char *server,char *message) {
  int connection;
  int connect_result;
    connect_result = bulletin_make_connection_with(server,20000,&connection);
    if (connect_result < 0) {
      printf("Could not establish connection with client\n");
//bulletin_exit(connect_result);
    }
    else {
      send_string(connection,message);
    }
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

void *handle_note(pstruct_t *pstruct) {
  char *user;
  char *destination;
  char *message;
  char *note;
  char *sender;
  dict_t *userD;
  int connection;
  int connect_result;

  userD = pstruct->D;
  user = (char *)malloc(sizeof(char)*32);
  message = (char *)malloc(sizeof(char)*256);
  sender = (char *)malloc(sizeof(char)*32);
  note = pstruct->note;
  printf("Thread has been created with note, %s\n", note);
  //trent = "trent.reed.edu";
  strcpy(user,strtok(note," "));
  strcpy(message,strtok(NULL,"^"));
  printf("message is: %s\n",message);
  strcpy(sender,strtok(NULL,"~~"));
  printf("sender is: %s\n",sender);  
  printf("message is: %s\n",message);
  strcat(sender," ");
  strcat(sender,message);
  printf("sender is: %s\n",sender);
  printf("user is: %s\n",user);
  if(containsD(user,userD)) {
    //printf("Here's where we would get location.\n");
    //pthread_exit(NULL);
    destination = getLocation(user,userD);
  }
  else {
    printf("Could not locate user");
    pthread_exit(NULL);
  }
  //destination = "trent.reed.edu";
  printf("So destination is: %s\n",destination);
  //bulletin_sendservernote(trent,note);
  do {
    connect_result = bulletin_make_connection_with(destination,20000,&connection);
    }while (connect_result < 0);
  send_string(connection,sender);
  free(sender);
  free(message);
  free(user);
  pthread_exit(NULL);
}

boolean shouldAdd(char *message) {
  char *head;
  char *mutable;
  mutable = (char *)malloc(sizeof(char[256]));
  mutable = strcpy(mutable,message);
  head = strtok(mutable,"$$");
  printf("original is: %s\n",message);
  printf("head is: %s\n",head);
  return !strcmp(message,head);
}

void addNewUser(pstruct_t *pstruct) {
  char *user;
  char *location;
  char *portString;
  int port;
  char *message;
  dict_t *userD;
  
  user = (char *)malloc(sizeof(char)*32);
  location = (char *)malloc(sizeof(char)*128);
  portString = (char *)malloc(sizeof(char)*8);

  message = pstruct->note;
  userD = pstruct->D;
  strcpy(user,strtok(message,"$$"));
  printf("user is: %s\n",user);
  strcpy(location,strtok(NULL,"$$"));
  printf("location is: %s\n",location);
  strcpy(portString,strtok(message,"##"));
  port = atoi(portString);
  printf("port is: %d\n",port);
  insertD(user,userD,location,port);
  printf("user has been succesfully added to dictionary.\n");
  pthread_exit(NULL);
}

void bulletin_recvnote(int bulletin_socket,dict_t *userD) {
  char buffer[256];
  int length;
  char *shouldParse;
  char *note;
  pstruct_t *pstruct;
  pthread_t *thread;
  int t = 0;

  note = (char *)malloc(sizeof(buffer));
  pstruct = (pstruct_t *)malloc(sizeof(pstruct_t));
  length = recv_string(bulletin_socket,buffer,255);
  close(bulletin_socket);
  strcpy(note,buffer);
  printf("buffer is: %s\n",buffer);
  pstruct->note = note;
  pstruct->D = userD;
  if(!shouldAdd(buffer)) {
    pthread_create(&thread,NULL,addNewUser,(void *)pstruct);
  }
  else {
    pthread_create(&thread,NULL,handle_note,(void *)pstruct);
  } 
  for(t = 0;t < 256;t++) {
    buffer[t] = 0;
  }
}

int main(int argc, char **argv) {
  int connection, listener;
  int connect_result;
  dict_t *userD = newD(10);
  char *testLocation;

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
      if(connect_result < 0) bulletin_exit(connect_result);
    fprintf(stderr,"You have successfully chosen to be a server.\n");
    while (1) {
      connect_result = bulletin_wait_for_connection(listener,&connection);
      if (connect_result < 0) bulletin_exit(connect_result);
      bulletin_recvnote(connection,userD);
    }
  } else {
    printf("You have successfully chosen to be a client.");
    connect_result = bulletin_make_connection_with(argv[1],atoi(argv[2]),&connection);
    if (connect_result < 0) bulletin_exit(connect_result);
    printf("Connected to that server!\n");
    bulletin_send_post(connection);
    printf("Disconnecting from that server...\n");
    close(connection);
  }
}
