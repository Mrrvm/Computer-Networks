#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>

#define PORT 58000

int main(int argc, char const *argv[])
{
	
	int fd, newfd;
	struct hostent *hostptr;   
	struct sockaddr_in serveraddr, clientaddr;
	int addrlen;
	char msg[80], buffer[80];

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd==-1)
    	exit(1);


    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL)
    	exit(3);


    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((u_short)PORT);

    if(bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))==-1)
        exit(6);

    if(listen(fd, 5)==-1)
        exit(7);

    while(1){
    	

    	addrlen = sizeof(clientaddr);
        newfd = accept(fd, (struct sockaddr*)&clientaddr, &addrlen);

        if(newfd==-1)
            exit(8);

    	if(read(newfd, buffer, sizeof(buffer))==-1)
    		exit(5);

        printf("%s", buffer);

        if(write(newfd, buffer, strlen(buffer)+1)==-1)
            exit(4);


    	

    }
    close(fd);
	return 0;
}

