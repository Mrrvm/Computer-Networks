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
	
	int fd;
	struct hostent *hostptr;   
	struct sockaddr_in serveraddr, clientaddr;
	int addrlen;
	char msg[80], buffer[80];

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(fd==-1)
    	exit(1);


    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL)
    	exit(3);


    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((u_short)PORT);

    if(bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))==-1)
        exit(6);

    while(1){
    	

    	addrlen = sizeof(clientaddr);

    	if(recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientaddr,&addrlen)==-1)
    		exit(5);

        if(sendto(fd, buffer, strlen(buffer)+1, 0, (struct sockaddr*)&clientaddr, addrlen)==-1)
            exit(4);


    	printf("%s", buffer);

    }
    close(fd);
	return 0;
}

