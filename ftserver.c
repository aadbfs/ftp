/* 
* Name: Alexandre Silva
* inspiration from: http://beej.us/guide/bgnet/output/html/multipage/clientserver.html & http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
*/

/****
Library definitions
****/
#include <arpa/inet.h>
#include <dirent.h> 
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/****
Constant definitions
****/
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 4194304 //max size of data

/****
Function: get_in_addr
Description: get sockaddr, IPv4 or IPv6
****/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/****
Function: getData
Description: gets data
****/
void getData(int sockfd, char* buf)
{
    int numBytes;
    memset(buf, 0, MAXDATASIZE);

    if ((numBytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) 
    {
        perror("recv");
        exit(1);
    }
    
    buf[numBytes] = 0;
    return;
}

/****
Function: getCommand
Description: gets commands
****/
void getCommand(int sockfd, char** commandp, char** hostp, char** data_portp, char** filenamep)
{
    //declare  vars
    static char buf[MAXDATASIZE];    
    char* command;
    char* host;
    char* data_port;
    char* filename;
    char* temp;

	//open to receive 
	getData(sockfd, buf);

    //parse recvd command
    command = buf;
    temp = strchr(buf, ' ');
    *temp = 0;
    host = temp+1;
    temp = strchr(host, ' ');
    *temp = 0;
    data_port = temp+1;
    temp = strchr(data_port, ' ');
    *temp = 0;
    filename = temp+1;
    temp = strchr(filename, ' ');
    close(sockfd);
	
    *commandp = command;
    *hostp = host;
    *data_portp = data_port;
    *filenamep = filename;

    return;

}

/****
Function: setConnection
Description: set up connection to receive data
****/
int setConnection(char* portno)
{
    int sockfd, new_fd;  //listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; //connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char* const hostname = "localhost";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //use my IP

    if ((rv = getaddrinfo(NULL, portno, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    printf("Server open on %s\n", portno);
    
    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) 
    {
            fprintf(stderr, "server: failed to find address\n");
            return 2;
    }
    
    if (listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen");
        exit(1);
    }

    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    close(sockfd);

    if (new_fd == -1) 
    {
        perror("accept");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    getnameinfo(get_in_addr((struct sockaddr *)p->ai_addr), sizeof(sa), hostname, sizeof hostname, NULL, 0, 0);
    freeaddrinfo(servinfo); // all done with this structure
    
	return new_fd;
}

/****
Function: setDataConnection
Description: sets up connection to receive commands, get data
****/
int setDataConnection(char* portno, char* hostn) 
{
    int sockfd;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN]; 
    printf("Connection from %s\n", hostn);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostn, portno, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("Server data connection: error opening socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("Server data connection: error connecting");
            continue;
        }
        break;
    }

    if (p == NULL) 
    {
        fprintf(stderr, "Server data connect: failed to find address\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    freeaddrinfo(servinfo); // all done with this structure
    return sockfd;
}

/****
Function: sendFile
Description: sends file over
****/
void sendFile(int sock, char* filename)
{
    char buf[MAXDATASIZE];
    int numBytes, sentBytes, offset;
    FILE *file = fopen(filename, "rb");
    printf("File %s requested\n", filename);
    //file error
    if (!file) 
    {
        printf("File %s not found.\n", filename);
        return;
    }
    
    //else read file
    while (!feof(file))
    {
        numBytes = fread(buf, 1, sizeof(buf), file);
        if (numBytes < 1) 
        {
            printf("Can't read from file %s\n", filename);
            fclose(file);
            exit(1);
        }
        offset = 0;
        do 
        {
            sentBytes = send(sock, &buf[offset], numBytes - offset, 0); //send bytes
            if (sentBytes < 1) 
            {
                printf("Can't write to socket\n");
                fclose(file);
                exit(1);
            }

            offset += sentBytes;

        } while (offset < numBytes);
    }

    fclose(file);
    
    printf("File %s sent.\n", filename);
    
    return;
}

/****
Function: sendDir
Description: send directory
****/
void sendDir(int sockfd) 
{
    DIR *pdir;
    struct dirent *pent;
    pdir=opendir("."); //current dir
    printf("List directory requested\n");
    if (!pdir)
    {
        printf ("opendir() failure; terminating");
        exit(1);
    }
    
    errno = 0;
    
    while((pent=readdir(pdir)))
    {
        send(sockfd, pent->d_name, strlen(pent->d_name), 0);
        send(sockfd, "\n", 1, 0);
    }
    
    if (errno)
    {
        printf ("readdir() failure; terminating");
        exit(1);
    }
    closedir(pdir);
    printf("List directory sent\n");
}

/****
Function: main
Description: main function
****/
int main(int argc, char *argv[])
{
    int datafd, controlfd;  
    char* command = NULL;
    char* host = NULL;
    char* data_port = NULL;
    char* filename = NULL;
    char* control_portno;
    

    //if invalid num of args  
    if (argc != 2) 
    {
        fprintf(stderr,"usage: server <portnumber>\n");
        exit(1);
    }
    
    control_portno = argv[1];

    //connect to client
    controlfd = setConnection(control_portno);

    //get command
    getCommand(controlfd, &command, &host, &data_port, &filename);

    //connect to client for sending data
    datafd = setDataConnection(data_port, host);

    //if command was -l
    if (strcmp(command, "-l") == 0) 
        sendDir(datafd);
    
    //elif command was -g, send file
    else if (strcmp(command, "-g") == 0) 
        sendFile(datafd, filename);
    
    else //error!
    {
        const char* msg = "Invalid command";
        send(datafd, msg, strlen(msg), 0);
    }

    return 0;
}

