#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
 
 // the port client will be connecting to
#define PORT 2107
#define MYPORT PORT
#define MAXDATASIZE 300

/* how many pending connections queue will hold */
#define BACKLOG 10

int active_users[BACKLOG];

void init_active_users()
{
	for(int i = 0; i < BACKLOG; ++i)
	{
		active_users[i] = -1;
	}
}

void add_active_user(int user)
{
	for(int i = 0; i < BACKLOG; ++i)
	{
		if(active_users[i] == -1)
		{
			active_users[i] = user;
			return;
		}
	}
}

void remove_active_user(int user)
{
    for(int i = 0; i < BACKLOG; ++i)
    {
        if(active_users[i] == user)
        {
            active_users[i] = -1;
        }
    }
}

int in_active_user(int user)
{
	for(int i = 0; i < BACKLOG; ++i)
	{
		if(active_users[i] == user)
		{
			return 1;
		}
	}
	return 0;
}

void clear_buf(char * buf)
{
	for(int i = 0; i < MAXDATASIZE; ++i)
	{
		buf[i] = '\0';
	}
}

void send_to_all(int sender, fd_set active_fd_set, char * p_message)
{
	for(int i = 0; i < FD_SETSIZE; ++i)
	{
		if(i != sender && in_active_user(i))
		{
			if (write(i, p_message, strlen(p_message)) == -1)
			{
				perror("Failed to send to clients");
			}
		}	
	}
}
 
int main()
{
	init_active_users();
    /* listen on sock_fd, new connection on new_fd */
    int sockfd, new_fd;
    /* my address information, address where I run this program */
    struct sockaddr_in my_addr;
    /* remote address information */
    struct sockaddr_in their_addr;
    int sin_size, read_size;
	fd_set active_fd_set, read_fd_set;
     
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
      perror("socket() error lol!");
      exit(1);
    }
    else
      printf("socket() is OK...\n");
     
    /* host byte order */
    my_addr.sin_family = AF_INET;
    /* short, network byte order */
    my_addr.sin_port = htons(MYPORT);
    /* auto-fill with my IP */
    my_addr.sin_addr.s_addr = INADDR_ANY;
     
    /* zero the rest of the struct */
    memset(&(my_addr.sin_zero), 0, 8);
     
    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
      perror("bind() error lol!");
      exit(1);
    }
    else
      printf("bind() is OK...\n");
     
    if(listen(sockfd, BACKLOG) == -1)
    {
      perror("listen() error lol!");
      exit(1);
    }
    else
      printf("listen() is OK...\n");
     
    /* ...other codes to read the received data... */
     
    sin_size = sizeof(struct sockaddr_in);
    /*
      int accept(int socket, struct sockaddr *restrict address,
            socklen_t *restrict address_len);
    */
	FD_ZERO (&active_fd_set);
  	FD_SET (sockfd, &active_fd_set);
	char buf[MAXDATASIZE];
	while(1)
	{
        read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
          perror ("select");
          exit (EXIT_FAILURE);
        }

		for (int i = 0; i < FD_SETSIZE; ++i)
		{
			if (FD_ISSET (i, &read_fd_set)) //INCOMING
			{
				int new;
				if(i == sockfd)
				{
					new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t *)&sin_size);
					if (new < 0)
                	{
                		perror ("accept");
                    	exit (EXIT_FAILURE);
                	}
					FD_SET (new_fd, &active_fd_set);
					add_active_user(new_fd);
					printf("NEW_FD: %d\n", new_fd);
				}
				else
				{
					clear_buf(buf);
					read_size = recv(i, buf, MAXDATASIZE, 0);
					printf("%s\n", buf);
					send_to_all(i, active_fd_set, buf);
					//write(i, buf, strlen(buf));
					if(strstr(buf, "LEAVE ") != NULL)
        			{
						remove_active_user(i);
            			puts("Client disconnected");
            			//fflush(stdout);
						close(i);
						FD_CLR(i, &active_fd_set);
        			}
					clear_buf(buf);
        			if(read_size == -1)
        			{
           				perror("recv failed");
				    }
				}
			}
		}
	}
    close(sockfd);
    return 0;
}
