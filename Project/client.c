#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 59000

int main(int argc, char *argv[])
{
    int opt;
    int port = -1;
    int sock;
    int addrlen;
    struct hostent *hostptr = NULL;
    struct sockaddr_in serveraddr;

    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
        switch (opt) {
            case 'i':
                hostptr = gethostbyname(optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i csip] [-p cspt]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(hostptr == NULL) {
        hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");
        if(hostptr==NULL) exit(EXIT_FAILURE);
    }

    if(port == -1) {
        port = DEFAULT_PORT;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL) exit(EXIT_FAILURE);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr;
    serveraddr.sin_port = htons((u_short)port);
    addrlen = sizeof(serveraddr);


    fprintf(stderr, "Arguments %s, %d\n", inet_ntoa((struct in_addr)serveraddr.sin_addr), port);

    close(sock);
    exit(EXIT_SUCCESS);
}
