#define MAXPATH 256*256
#define MAXNAME 256
#define MAXARGV MAXPATH+2*MAXNAME
#define MAXDEPENDENCY 32
#define PORT 20000
#define	HOSTNAME "eve.reed.edu"	
#include "fileDictionary.h"
#include "jobDictionary.h"

int main(int argc, char *argv) {
	printf("Usage: \n");
	printf("To submit a file: file <path> <nickname>\n");
	printf("To submit a job to run: job <nickname> <source> <dependencies ... >\n");
	printf("To check output of a job: result <nickname>\n");
	while(1){
		parseInput();
	}
}

int parseInput() {
	char argv[MAXARGV];
	//FILE *source;
	char action[MAXNAME];
	char message[MAXARGV];

	struct _file_dict_t *FileD;
	job_dict_t *JobD;

	//get input
	gets(argv);

	//sscanf(argv," %s",action);
	//get the first token; should designate the type of action
	action = strtok(argv, " ");

	if (0 == strcmp(action,"file")) {
		char path[MAXPATH];
		char truepath[MAXPATH];
		char nick[MAXNAME];
		int newID;
		
		//sscanf(argv,"%s %s %s",action,path,nick);
		//eat the next two tokens
		path = strtok(NULL," ");
		nick = strtok(NULL," ");

		realpath(path,truepath); //gives an absolute path

		message = strcat(action,"$$");
		message = strcat(message,truepath);
		
		newID = sendData(message);	
		insertFileD(nick,FileD,newID);

	} else if (0 == strcmp(action,"job")) {
		char fileNicks[MAXDEPENDENCY][MAXNAME];
		int fileIDs[MAXDEPENDENCY];
		char nick[MAXNAME];
		int newID;
		int i = 0;
		
		//eat the next token from argv
		nick = strtok(NULL," ");

		while(0 != strcmp(argv,"")) {
			fileNicks[i]=strtok(NULL," ");
			fileIDs[i]=getFileID(fileNicks[i],FileD);
			i++;
		}

		//building the message through the output file handle
		message = strcat(action,"$$");
		message = strcat(message,fileIDs[0]);
		while(i>0) {
			message = strcat(message,"##");
			message = strcat(message,fileIDs[i]);
			i--;
		}

		newID = sendData(message);	
		insertJobD(nick,JobD,newID);		
	} else if (0 == strcmp(action,"result")) {
		char nick[MAXNAME];
		int jobID;
		int connect_result,listener,connection;
		char outputPath[MAXPATH];
		char receivedType[MAXNAME];
		//FILE *source;
		//FILE *dest;
		int cpExit;
		pid_t pid;

		nick = strtok(NULL," ");

		jobID = getJobID(nick,JobD);		
		message = strcat(action,"$$");
		message = strcat(message,jobID);


		connect_result = bulletin_make_connection_with(HOSTNAME,PORT,&connection);
	  	if (connect_result < 0) bulletin_exit(connect_result);
	  	send_string(server,message);
		
		//listen
		connect_result = bulletin_set_up_listener(PORT,&listener);
		if(connect_result < 0) bulletin_exit(connect_result);
		recv_string(connection, outputPath, MAXARGV);

		receivedType = strtok(outputPath,"$$");
		if(strcmp(receivedType,"path"){ 
			printf("Job %s incomplete.\n",nick);
 		} //if receivedType is not "path"
		else {
			outputPath = realpath(outputPath,NULL);
			//source = fopen(outputPath,"r");
			//dest = fopen(nick,"w");
			pid = fork();

			if (pid == 0) {
        			execl("/bin/cp", "/bin/cp", outputPath, nick, (char *)0);
    			}
    			else if (pid < 0) {
    				printf("Job complete, but file not found.\n")
    			}
    			else {
			        pid_t ws = wait(cpExit);
				printf("Job complete. Output file at \"%s\" in this directory.\n",nick);
			}
		}
	}
}

//sendData sends a message to the servers detailing a file or job to be included in the system, waits for an ID for that file or job, and returns it.
int sendData(char *message){
	int connect_result,listener,connection;
	char buffer[MAXARGV];
//	int port = 20000;
//	char *hostname="eve.reed.edu";
	
	//speak
	connect_result = bulletin_make_connection_with(HOSTNAME,PORT,&connection);
  	if (connect_result < 0) bulletin_exit(connect_result);
  	send_string(server,message);
	
	//listen
	connect_result = bulletin_set_up_listener(PORT,&listener);
	if(connect_result < 0) bulletin_exit(connect_result);
	recv_string(connection, buffer, MAXARGV);

	connect_result = atoi(buffer);
	return connect_result;
}

