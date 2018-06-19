#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

/*the port client will be connecting to*/
 #define PORT 2107
/* max number of bytes we can get at once*/
 #define MAXDATASIZE 300

char * remove_newline(char * s)
{
    int i = strlen(s)-1;
    if (i > -1)
    {
        s[i] = '\0';
    }
	return s;
}

void clearBuf(char *buf)
{
	for(int i=0; i<MAXDATASIZE; ++i)
	{
		buf[i] = '\0'; 
	}
}

void check_chars(char * s)
{
	for(int i = 0; i < MAXDATASIZE; ++i)
	{
		printf("%d ", s[i]);
	}
}

char * send_enter(int sockfd, char * username)
{
    char enter[MAXDATASIZE] = "ENTER ";
    printf("Username: ");
    fgets(username, 194, stdin);
    remove_newline(username);
    strcat(enter, username);
	
	/*check_chars(enter);
     printf("%s\n", enter);*/
	printf("%d\n", strlen(enter));
	if(send(sockfd, enter, strlen(enter),0)<0)
    {
        perror("send failed");
    	exit(1);
    }
	return username;
}

void say_message(int sockfd, char * username, char * message)
{
	char complete_message[MAXDATASIZE];
	strcpy(complete_message, "SAY ");
	strcat(complete_message, username);
	strcat(complete_message, " ");
	strcat(complete_message, message);
    if(send(sockfd, complete_message, strlen(complete_message),0)<0)        
	{
        perror("send failed");
	    exit(1);
	}
}

void * send_leave(int sockfd, char * username)
{
	char leave[MAXDATASIZE] = "LEAVE ";
	strcat(leave, username);
    if(send(sockfd, leave, strlen(leave),0)<0)
    {
        perror("send failed");
        exit(1);
    }	
}

void to_server_loop(int sockfd, char * username)
{
	char buf[MAXDATASIZE];
	while(1)
	{
	fgets( buf, MAXDATASIZE, stdin);
    	if(strcmp(buf, "!LEAVE\n")==0)
        {
            send_leave(sockfd, username);
            printf("Client-Closing sockfd\n");
            close(sockfd);
            return;
        }
        remove_newline(buf);
        say_message(sockfd, username, buf);
        clearBuf(buf);
	}
}

void parse_input(char * input)
{
	char * tok;
	if(strncmp(input, "ENTER", 5) == 0)
	{
		tok = strtok(input, " ");
		tok = strtok(NULL, " ");
		printf("%s has joined the chatroom\n", tok);
	}
	else if(strncmp(input, "SAY", 3) == 0)
	{
		char temp[MAXDATASIZE];
		strcpy(temp, input);
		tok = strtok(temp, " ");
        tok = strtok(NULL, " ");
		printf("%s: %s\n", tok, input+strlen(tok)+5);
	}
	else if(strncmp(input, "LEAVE", 5) == 0)
	{
        tok = strtok(input, " ");
        tok = strtok(NULL, " ");
        printf("%s has left the chatroom\n", tok);
	}
	else
	{
		perror("Recieved input that doesn't follow protocol");
	}
}

void from_server_loop(int sockfd)
{
	char buf[MAXDATASIZE];
	while(1)
	{
		recv(sockfd, buf, MAXDATASIZE-1, 0);
		if(strlen(buf) > 0)
		{
			parse_input(buf);
		}
		clearBuf(buf);
		if(waitpid(0, NULL, WNOHANG)>0)
		{
			break;
		}
	}
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    /* connector’s address informationi*/
     struct sockaddr_in their_addr;
     
    /* if no command line argument supplied*/
     if(argc != 2)
    {
        fprintf(stderr, "Client-Usage: %s the_client_hostname\n", argv[0]);
        /* just exit*/
        exit(1);
    }
     
    /* get the host info*/
   	 if((he=gethostbyname(argv[1])) == NULL)
    {
        perror("gethostbyname()");
        exit(1);
    }
    else
        printf("Client-The remote host is: %s\n", argv[1]);
     
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket()");
        exit(1);
    }
    else
        printf("Client-The socket() sockfd is OK...\n");
     
    /* host byte order*/
     their_addr.sin_family = AF_INET;
    /* short, network byte order*/
     printf("Server-Using %s and port %d...\n", argv[1], PORT);
    their_addr.sin_port = htons(PORT);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    /* zero the rest of the struct*/
     memset(&(their_addr.sin_zero), '\0', 8);
     
    if(connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect()");
        exit(1);
    }
    else
        printf("Client-The connect() is OK...\n");
	char username[194];
	strcpy(username, send_enter(sockfd, username));
	int pid = fork();
	switch(pid)
	{
		case -1:
			perror("Failed to fork");
			exit(1);
		case 0:
			to_server_loop(sockfd, username);
			exit(0);
		default:
			from_server_loop(sockfd);
			
	}
	/*while(1)
	{ 
		clearBuf(buf);
		fgets( buf, MAXDATASIZE, stdin);
		if(strcmp(buf, "!LEAVE\n")==0)
		{
			send_leave(sockfd, username);	
			printf("Client-Closing sockfd\n");
	 		close(sockfd);
			break;
		}
		remove_newline(buf);
		say_message(sockfd, username, buf);
		clearBuf(buf);
    	if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
   		 {
    	    perror("recv()");
       		 exit(1);
   	 	}
	printf("%s\n", buf);
	
  }	*/
   
    return 0;
}
