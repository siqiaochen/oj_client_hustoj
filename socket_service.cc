#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#define MAX_BUF 8192
#define MAX_CMD 20

void process_message(int sock, char* buf, int len)
{
    char input_buf[MAX_BUF];
    char* cmd;
    int n = 0;
    memcpy(input_buf,buf,len);
    input_buf[len] = 0;
    cmd = strtok(input_buf," \r\n");
    if(strcmp(cmd,"echo")==0)
    {

        printf("Client Echo\n");
        n = write(sock,"echo\n\r",6);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(1);
        }
    }
    else if(strcmp(cmd,"exit")==0)
    {

        printf("Client Exit\n");
        n = write(sock,"BYE\n",3);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(1);
        }
        exit(0);
    }
    else if(strcmp(cmd,"solve")==0)
    {
        ;
    }
    else
    {
        printf("ERROR: can not read command:%s\n",cmd);
    }
}
void doprocessing (int sock)
{
    int n;
    char buffer[MAX_BUF];
    int i = 0, index = 0;

    bzero(buffer,MAX_BUF);
    while(1)
    {
        n = read(sock,buffer,MAX_BUF-1-index);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(1);
        }
        index += n;
        if(index >=MAX_BUF-1)
        {
            index = 0;
        }
        for(i=0;i<index;i++)
        {
            if(buffer[i] == '\n')
            {
                process_message(sock,buffer,i);
                //n = write(sock,"\n",1);
                if (n < 0)
                {
                    perror("ERROR writing to socket");
                    exit(1);
                }
                index = 0;
                break;
            }
        }

    }
}
int main( int argc, char *argv[] )
{
    int sockfd, newsockfd, portno;
    unsigned int clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;
    int pid = -1;
    int current_connection = 0;
    pid_t result = 0;
    int status =0;
    struct timeval tv;
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5001;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                          sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
         exit(1);
    }
    /* Now start listening for the clients, here
     * process will go in sleep mode and will wait
     * for the incoming connection
     */
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    while (1)
    {
        newsockfd = accept(sockfd,
                (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }
        if(current_connection > 0)
        {
            result = waitpid(pid, &status, WNOHANG);
            if(result == 0)
            {
                perror("BUSY");
                write(newsockfd,"BUSY\n\r",6);
                close(newsockfd);
                continue;
            }
            else
            {
                current_connection --;
            }
        }
        /* Create child process */
        pid = fork();
        if (pid < 0)
        {
            perror("ERROR on fork");
	    exit(1);
        }
        if (pid == 0)
        {
            /* This is the client process */

            tv.tv_sec = 30;  /* 30 Secs Timeout */
            tv.tv_usec = 0;  // Not init'ing this can cause strange errors

            setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
            close(sockfd);
            doprocessing(newsockfd);
            close(newsockfd);
            exit(0);
        }
        else
        {
            current_connection ++;
            close(newsockfd);
        }
    } /* end of while */
}
