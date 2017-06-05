#include <stdio.h> 
	
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* include to see if it as directory or a file */
#include <sys/stat.h>

/* include for the gethostbyname lines of code*/
#include <sys/types.h>
#include <netdb.h>

/* to use the signal */
#include <signal.h>

#define MAXPENDING 5    /* Maximum outstanding connection requests */


FILE *mdbInSock; //global variable to handle the signal function


void controlc (){
	signal(SIGINT,controlc);
	fclose(mdbInSock);
	exit(1);
}

void die(char *s){
	fprintf(stderr,"%s",s);
	exit(1);
}

int main ( int argc, char *argv[]){
	signal(SIGINT, controlc); //control c handler

	int servSock;
	int clntSock;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	unsigned short echoServPort; //port number
	unsigned int clntLen; 
	char clntRequest[1000]; 	
	char webRoot[1000];
	int exit_aux = 0;


/* variables to the tcp connection with ther server for mdb-lookup */	
	int mdbServSock;
	struct sockaddr_in echoMdbServAddr;
	unsigned short echoMdbServPort;
	//unsigned int MdbClntLen;
	char *mdbServIP;
	



	if(argc!= 5){
		fprintf(stderr,
	"./http-server <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>");
		exit(1);
	}
		
	struct hostent *he;
	char *serverName = argv[3];

	if ((mdbServSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf("could not open a socket for the mdb-lookup server connection");
		die("");
	}		


	//get server ipp from server name
	if((he=gethostbyname(serverName)) == NULL){
		printf("Could not make connection with the mdb-lookup server");
		die("");
	}
	mdbServIP = inet_ntoa(*(struct in_addr *)he->h_addr);
	

	echoMdbServPort = atoi(argv[4]);
	memset(&echoMdbServAddr,0,sizeof(echoMdbServAddr));
	echoMdbServAddr.sin_family = AF_INET;
	echoMdbServAddr.sin_addr.s_addr = inet_addr(mdbServIP);
	echoMdbServAddr.sin_port = htons(echoMdbServPort);
	
	if(connect(mdbServSock,(struct sockaddr *)&echoMdbServAddr,sizeof(echoMdbServAddr))<0)
	{
		printf("could not connect to mdb-lookup-server");
		die("");
	}

	mdbInSock = fdopen(mdbServSock,"r");
	if(!mdbInSock){
		die("could not open the mdb server file");
	} 
		


	echoServPort = atoi(argv[1]);
	strcpy(webRoot,argv[2]);	

	if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		die("socket() failed");
	}
	
	memset(&echoServAddr , 0 , sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort);
	
	    /* Bind to the local address */ 
	if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0){
        	close(servSock);
		fclose(mdbInSock);
		die("bind() failed");
	} 
	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAXPENDING) < 0)
		die("listen() failed");

	char *token_separators_files = "\t \r\n", *method, *requestURI, *httpVersion;
	for(;;){
		clntLen = sizeof(echoClntAddr);
		exit_aux = 0;
		if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr,
					&clntLen)) < 0){
			printf("accept() failed");
			exit_aux = 1;
		}	
		FILE *clntInput = fdopen(clntSock,"r");
		fgets(clntRequest,1000,clntInput);
		clntRequest[strlen(clntRequest)-2] = '\0';
		printf("%s :  %s  ",inet_ntoa(echoClntAddr.sin_addr),clntRequest);
        	method = strtok(clntRequest, token_separators_files);
        	requestURI = strtok(NULL, token_separators_files);
       		httpVersion = strtok(NULL, token_separators_files);
		if(strstr(requestURI,"/mdb-lookup") == NULL){	
			if(strstr(method,"GET") == NULL  && exit_aux ==0){
				char *msg = "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>";	
				send(clntSock,msg,strlen(msg),0);	
				printf("501 Not Implemented\n");
				exit_aux =1;
			}
			if(requestURI[0] != '/' && exit_aux == 0){
				char *msg = "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>";
				send(clntSock,msg,strlen(msg),0);
				printf("400 Bad Request\n");
				exit_aux = 1;
			}
			if((strstr(requestURI,"/../")!= NULL || strcmp(&requestURI[strlen(requestURI)-1-3],"/..") == 0) && exit_aux == 0){
				char *msg = "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>";
				send(clntSock,msg,strlen(msg),0);
				printf("400 Bad Request\n");
				exit_aux = 1;
			}
			if(strstr(httpVersion,"HTTP/1.0") == NULL && strstr(httpVersion,"HTTP/1.1") == NULL && exit_aux ==0){
				char *msg = "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>";	
				send(clntSock,msg,strlen(msg),0);
				printf("501 Not Implemented\n");
				exit_aux = 1;
			}
			if(exit_aux == 1){
				fclose(clntInput);
				continue;
			}
			//printf("\nbrowser: %s", clntRequest);
			requestURI = strcat(webRoot,requestURI);
			
			if(requestURI[strlen(requestURI)-1] !='/'){
				struct stat flags;
				if(stat(requestURI,&flags) == 0){
					if (flags.st_mode & S_IFDIR ){
						char *msg = "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>";
						send(clntSock,msg,strlen(msg),0);
						printf("400 Bad Request\n");
						fclose(clntInput);
						strcpy(webRoot,argv[2]);
						continue;
					}
				}
			}
			if(requestURI[strlen(requestURI)-1] =='/'){	
				requestURI = strcat(requestURI,"index.html");
			}

			FILE *fileInput = fopen(requestURI,"r");
			if(fileInput == NULL){
				char *msg = "HTTP/1.0 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
				send(clntSock,msg,strlen(msg),0);
				printf("404 Not Found\n");
				fclose(clntInput);
				strcpy(webRoot,argv[2]);
				continue;
			}
			char *msg = "HTTP/1.0 200 OK\r\n\r\n" ;
			send(clntSock,msg,strlen(msg),0);
			printf("200 OK\n");
			char readFile[4097];
			int bytes_sent = 0;
			int read_aux = 1;
			while (1){
				read_aux = fread(readFile,1,4096,fileInput);
				while(bytes_sent != read_aux){
					bytes_sent += send(clntSock,readFile+bytes_sent,read_aux-bytes_sent,0);
				}
				bytes_sent = 0;
				if(read_aux < 4096)
					break;
			}
			 
			fclose(clntInput);
			fclose(fileInput);
			strcpy(webRoot,argv[2]);
		}
		else{
			if(strcmp("/mdb-lookup",requestURI) == 0){
				char *form =
				"HTTP/1.0 200 OK\r\n\r\n"
				"<html><body><h1>mdb-lookup</h1>\n"
				"<p>\n"
				"<form method=GET action=/mdb-lookup>\n"
				"lookup: <input type=text name=key>\n"
				"<input type=submit>\n"
				"</form>\n"
				"<p></body></html>\n\n";	
				send(clntSock,form,strlen(form),0);
				fclose(clntInput);
				printf("200 OK\n");
			}
			else if(strstr(requestURI,"/mdb-lookup?key=")!= NULL){
				char *form = "HTTP/1.0 200 OK\r\n\r\n"
					"<html><body>"
					"<h1>mdb-lookup</h1>"
					"<p>"
					"<form method=GET action=/mdb-lookup>"
					"lookup: <input type=text name=key>"
					"<input type=submit>"
					"</form>"
					"<p>"
					"<p><table border>";
				send(clntSock,form,strlen(form),0);
				printf("200 OK\n");
				char *token = &requestURI[strlen("/mdb-lookup?key=")];
				char *plus = strstr(token,"+");
				if(plus) *plus = ' ';
				send(mdbServSock,token,strlen(token),0);
			      	send(mdbServSock,"\n",1,0);
				char readFile[1000];
				int bytes_sent = 0;
				int read_aux = 1;
				int count_color = 0;
				while (strlen(fgets(readFile,1000,mdbInSock))>1){
					read_aux = strlen(readFile);
					char *before;
					if(count_color == 1){
						before = "\n\n<tr><td bgcolor=yellow>" ;
						count_color = 0;
					}
					else{
						before = "\n\n<tr><td bgcolor=red>" ;
						count_color =1;	
					}
					//char *after = " ";
					send(clntSock,before,strlen(before),0);
					while(bytes_sent != read_aux){
						bytes_sent += send(clntSock,readFile+bytes_sent,read_aux-bytes_sent,0);
					}
					//send(clntSock,after,strlen(after),0);
					bytes_sent = 0;
				}
				char *after = "\n\n</table>\n</body></html>";
				send(clntSock,after,strlen(after),0);
				fclose(clntInput);
	  		}
			else {
				char *msg = "HTTP/1.0 400 Bad Request\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>\n\n";
				send(clntSock,msg,strlen(msg),0);
				printf("400 Bad Request\n");
				fclose(clntInput);
			}
		}
		
	}
}





