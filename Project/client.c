#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))

#define DEFAULT_PORT 59000
#define GET_DS_SERVER "GET_DS_SERVER"

int main(int argc, char *argv[])
{
    int opt;
    int port = -1;
    int sock;
    int addrlen;
    int x;
    int maxfd, counter;
    struct hostent *hostptr = NULL;
    struct sockaddr_in serveraddr;
    fd_set rfds;
    enum {idle, busy} state;
    char command[64], reply[64];

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
        if(hostptr==NULL) 
            exit(EXIT_FAILURE);
    }

    if(port == -1) {
        port = DEFAULT_PORT;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock ==-1) 
        exit(EXIT_FAILURE);

    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL) 
        exit(EXIT_FAILURE);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr;
    serveraddr.sin_port = htons((u_short)port);
    addrlen = sizeof(serveraddr);


    fprintf(stderr, "Arguments %s, %d\n", inet_ntoa((struct in_addr)serveraddr.sin_addr), port);

    state = idle;

    while(1){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;

        if(state == busy){
            FD_SET(sock, &rfds);
            maxfd = max(0,sock);
        }

        counter = select(maxfd+1, &rfds,  (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        if(counter <= 0)
            exit(EXIT_FAILURE);


        if(FD_ISSET(0, &rfds)){
            fgets(command, 64, stdin);

            if(strstr(command, "request_service") != NULL){
                memset(reply,0,strlen(reply));
                state = busy;
                sscanf(command, "%*[^\' '] %d", &x);
                sprintf(reply, "%s %d", GET_DS_SERVER, x);

                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
                    exit(EXIT_FAILURE);
                
            }
            else if(strstr(command, "exit") != NULL){
                exit(EXIT_SUCCESS);
            }
            else
                fprintf(stderr, "Invalid Command\n");
            
                
            


        }
        if(FD_ISSET(sock, &rfds)) {
            memset(reply,0,strlen(reply));
            state == idle;

            if(recvfrom(sock, reply, sizeof(reply), 0, (struct sockaddr*)&serveraddr,&addrlen)==-1)
                exit(EXIT_FAILURE);
            fprintf(stderr, "Reply: %s\n", reply);

        }
    }

    close(sock);
    exit(EXIT_SUCCESS);
}
