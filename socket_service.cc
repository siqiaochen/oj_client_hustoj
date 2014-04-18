#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include "base64.h"
#include "cJSON.h"
#define MAX_BUF 1048576
#define MAX_CMD 20
int solve_progress;
pid_t client_pid;

void init_oj()
{
    solve_progress = 0;
    client_pid = 0;
}
void writefile( char* fname, char* buf, int size)
{
    FILE * fp;
    fp = fopen(fname,"w");
    if(fp!=NULL)
    {
        fwrite(buf,1,size,fp);
        fclose(fp);
    }
    else
    {
        perror("ERROR can not write file");
        exit(1);
    }
}
void parse_cjson(char* str)
{
    cJSON* root = cJSON_Parse(str);
    cJSON* problem = cJSON_GetObjectItem(root,"problem");
    char* solution_64 = cJSON_GetObjectItem(problem,"solution")->valuestring;
    cJSON* unit_tests = cJSON_GetObjectItem(problem,"testcases");
    char* in_str, *out_str,*in_str_64, *out_str_64;
    int i = 0;
    size_t input_len;
    cJSON* unit_test = NULL;
    char filename[256];
    char * solution;
    sprintf(filename,"/home/judger/run/Main.c");
    solution = (char *)base64_decode((const char*)solution_64,
                              strlen(solution_64),
                             &input_len);
    printf("main.c : %s",solution);
    writefile(filename,solution,input_len);
    free(solution);
    for(i =0; i< cJSON_GetArraySize(unit_tests);i++)
    {
        in_str = NULL;
        out_str = NULL;
        unit_test = cJSON_GetArrayItem(unit_tests,i);
        in_str_64 = cJSON_GetObjectItem(unit_test,"data_in")->valuestring;

        in_str = (char *)base64_decode((const char*)in_str_64,
                              strlen(in_str_64),
                             &input_len);
        sprintf(filename,"/home/judger/data/%d.in",i);
        printf("data in : %s",in_str);
        writefile(filename,in_str,input_len);
        out_str_64 = cJSON_GetObjectItem(unit_test,"data_out")->valuestring;
        out_str = (char *)base64_decode((const char*)out_str_64,
                              strlen(out_str_64),
                             &input_len);
        sprintf(filename,"/home/judger/data/%d.out",i);
        printf("data out : %s",out_str);
        writefile(filename,out_str,input_len);
        free(in_str);
        free(out_str);
    }
    // clean memory
    cJSON_Delete(root);

    //parse_object()
}
pid_t solve_problem()
{

    //pid_t parent = getpid();
    pid_t pid = fork();
    int status;

    if (pid < 0)
    {
        // error, failed to fork()
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid,&status,0);
        printf("judged\n");
        return pid;
    }
    else
    {
        // we are the child
        printf("start judge\n");
        execv("/home/judger/bin/oj_client",NULL);
        exit(0);   // exec never returns
    }
}
char* update_solution(size_t* output_size)
{
    char oj_result[255];
    char* buffer;
    char* return_buf;
    int size = 0;
    size_t result;
    strcpy(oj_result, "/home/judger/log/result");
    FILE *fp = fopen(oj_result,"rb");
    if(fp == NULL)
        exit(1);
    printf("read result\n");
    fseek(fp,0,SEEK_END);
    size = ftell(fp);
    fseek(fp,0,SEEK_SET);
    buffer = (char*) malloc(sizeof(char)*size);
    result = fread(buffer,1,size,fp);
     if(result != size)
        exit(1);
    fclose(fp);

    printf("convert result to base 64\n");
    return_buf = base64_encode((unsigned char *)buffer,size,output_size);
    free(buffer);

    printf("return result\n");
    return return_buf;
}
void process_message(int sock, char* buf, int len)
{
    char input_buf[MAX_BUF];
    char* cmd;
    char* param_b64;
    char* input_str;
    int n = 0;
    int status = 0;
    size_t input_len;
    size_t output_len;
    char* output_str;
    pid_t result;
    memcpy(input_buf,buf,len);
    input_buf[len] = 0;
    cmd = strtok(input_buf," \r\n");
    if(strcmp(cmd,"echo")==0)
    {

        printf("Client Echo\n");
        n = write(sock,"echo\n",5);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(1);
        }
    }
    else if(strcmp(cmd,"status")==0)
    {
        if(solve_progress < 1)
        {
            n = write(sock,"starting\n",9);
            if (n < 0)
            {
                perror("ERROR reading from socket");
                exit(1);
            }
        }
        else
        {
            result = waitpid(client_pid, &status, WNOHANG);
            if(result == 0)
                n = write(sock,"processing\n",11);
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
        param_b64 = strtok(NULL," \r\n");
        input_str = (char *)base64_decode((const char*)param_b64,
                              strlen(param_b64),
                             &input_len);
        printf("Client write solve info: %s\n",input_str);
        parse_cjson(input_str);
        //execl("./Main", "./Main", (char *)NULL);
        free(input_str);
        printf("solve problem\n");
        client_pid = solve_problem();
        solve_progress = 1;
        output_str = update_solution(&output_len);
        printf("result: %s\n",output_str);
        solve_progress = 2;
        write(sock,"solution ",9);
        write(sock, output_str, output_len);
        write(sock,"\n",1);
        exit(0);
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
    //char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    //int  n;
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
