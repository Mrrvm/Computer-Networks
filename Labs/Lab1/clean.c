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

#define PORT 59000

int main(int argc, char const *argv[])
{
	
	int fd;
	struct hostent *hostptr;
	struct sockaddr_in serveraddr, clientaddr;
	int addrlen;
	char msg[80], buffer[80], buffer2[128];

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(fd==-1)
    	exit(1);

    hostptr=gethostbyname("tejo.tecnico.ulisboa.pt");

    if(hostptr==NULL){
    	exit(2);
    }

    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL)
    	exit(3);


    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr;
    serveraddr.sin_port = htons((u_short)PORT);

    addrlen = sizeof(serveraddr);

	if(sendto(fd, "WITHDRAW_START 26;1", strlen("WITHDRAW_START 26;1")+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
		exit(4);

    if(sendto(fd, "WITHDRAW_DS 26;1", strlen("WITHDRAW_DS 26;1")+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
        exit(4);

    if(sendto(fd, "GET_START 26;1", strlen("WITHDRAW_DS 26;1")+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
        exit(4);


    while(1){
        memset(buffer,0,strlen(buffer));
        fgets (msg, 80, stdin);


    	addrlen = sizeof(serveraddr);

    	if(recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&serveraddr,&addrlen)==-1)
    		exit(5);

    	

    }
    close(fd);
	return 0;
}

