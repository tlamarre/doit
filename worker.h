#include "prelude.h"
#include "server.h"

void worker_recvnote(int socket);
void handle_job_order(jobDescriptor *job);
serverId *lookup_manager();
boolean can_contact_server(serverId *server);


