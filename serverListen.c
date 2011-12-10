// Math 442 DS: Distributed and Networked Systems
// Reed College
// Fall 2011
//
// bulletin.c
//
// This code implements a Unix executable that 
// can act either as a client or a server of a
// remote bulletin posting service.
//
// Run as "bulletin 20000", it will set up a
// bulletin server that receives a posts from
// a series of clients that connect to it, 
// listening on port 20000.
//
// Run as "bulletin bob.reed.edu", it will connect
// to a bulletin server listening on port 20000
// on the machine named bob.reed.edu, and send
// a single post on the machine.  The post can
// be entered by the program's user as a series.
// line with the four letters "STOP".
//
// It is a simple example of the Berkeley socket
// API for performing networked communications via
// TCP/IP.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

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

void bulletin_recv_post(int bulletin_socket) {
  char buffer[256];
  int length;
  // repeatedly post lines sent by the client
  do {
    // receive a string from this client's connection socket
    length = recv_string(bulletin_socket,buffer,255);
    printf("Client says \"%s\".\n",buffer);
  } while (length >= 0 && strcmp(buffer,BULLETIN_TERMINATE)); 
}

void *handle_message(char *message) {
  char *sender;
  char *note;

  sender = (char *)malloc(sizeof(char)*32);
  note = (char *)malloc(sizeof(char)*256);
  strcpy(sender,strtok(message," "));
  strcpy(note,strtok(NULL,"~~"));
  printf("%s says ",sender); 
  printf("\"%s\".\n",note);
  pthread_exit(NULL);
}

void bulletin_recvnote(int bulletin_socket) {
  char buffer[256];
  int length;
  pthread_t *thread;
  int t;
  // repeatedly post lines sent by the client
  //do {
    // receive a string from this client's connection socket
    length = recv_string(bulletin_socket,buffer,255);
    //printf("Client says \"%s\".\n",buffer);
    close(bulletin_socket);
    pthread_create(&thread,NULL,handle_message,(void *)buffer);
    
  //} while (length >= 0 && strcmp(buffer,BULLETIN_TERMINATE)); 
}

void tell_server_who_i_am(char *name,char *location,int listenPort) {
  char *serverInfo;
  char *eve;
  char *portString;
  //char *portNumber;
  int server;
  int connect_result;
  int i = 0;

  eve = "eve.reed.edu";
  //serverInfo = "Hi Eve I am Trent.";
  //portNumber = sprintf(portNumber,"%d",listenPort);
  //serverInfo = strcat("#$",name);
  //serverInfo = strcat(serverInfo,"_");
  //serverInfo = strcat(serverInfo,portNumber);
  
  serverInfo = (char *)malloc(sizeof(char) * 128);

  serverInfo = strcat(name,"$$");
  strcat(serverInfo,location);
  strcat(serverInfo,"$$");
  strcat(serverInfo,(char *)&listenPort);
  strcat(serverInfo,"##");
  while(i < 1000) {
    connect_result = bulletin_make_connection_with(eve,20000 + i,&server);
    if(connect_result > 0) break;
    i++;
  }
  send_string(server,serverInfo);
}  
  
  
  
  

int main(int argc, char **argv) {
  int connection, listener;
  int connect_result;
  int listenPort;
  char host[128];
  char *hostname;
  char *name;

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
    // if we are a server

    // set up a listener for client connections
    fprintf(stderr,"You have successfully chosen to listen.\n");
    listenPort = 20000;
    connect_result = bulletin_set_up_listener(listenPort,&listener);
    if (connect_result < 0) bulletin_exit(connect_result);
    if(gethostname(host,sizeof(hostname)) > -1) {
      printf("host is: %s\n",host);
      hostname = strcat(host,".reed.edu");
      printf("hostname is: %s\n", hostname);
      name = argv[1];
      printf("name is: %s\n", name);
      tell_server_who_i_am(name,hostname,listenPort);
    }
    //tell_server_who_i_am(trent,listenPort);
    // receive and handle a series of client posts, one connection at a time
    while (1) {
      // get the next client connection
      connect_result = bulletin_wait_for_connection(listener,&connection);
      if (connect_result < 0) bulletin_exit(connect_result);
      // receive that client's post
      bulletin_recvnote(connection);
      close(connection);
    }
  }
}
