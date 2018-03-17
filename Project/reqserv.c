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

#define SC_PORT 59000
#define SC_DOMAIN "tejo.tecnico.ulisboa.pt"
#define GET_DS_SERVER "GET_DS_SERVER"
#define MY_SERVICE_ON "MY_SERVICE ON"
#define MY_SERVICE_OFF "MY_SERVICE OFF"


int main(int argc, char *argv[])
{
    int opt;
    int port = -1;
    int sock;
    unsigned int addrlen;
    int in_service = 0;
    int x;
    int maxfd, counter;
    struct hostent *hostptr = NULL;
    struct sockaddr_in serveraddr;
    fd_set rfds;
    enum {idle, busy} state;
    char command[64] = {0}, reply[64] = {0}, id[64] = {0}, ip[64] = {0}, upt[64] = {0};

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
        hostptr = gethostbyname(SC_DOMAIN);
        if(hostptr==NULL) exit(EXIT_FAILURE);
    }
    if(port == -1) port = SC_PORT;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL) exit(EXIT_FAILURE);
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
            state = busy;

            if(strstr(command, "rs") != NULL && in_service == 0){
                memset(reply,0,strlen(reply));
                sscanf(command, "%*[^\' '] %d", &x);
                sprintf(reply, "%s %d", GET_DS_SERVER, x);

                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
                    exit(EXIT_FAILURE);

            }
            else if(strstr(command, "exit") != NULL){
                fprintf(stderr, "Sending message: %s\n", MY_SERVICE_OFF);

                if(sendto(sock, MY_SERVICE_OFF, strlen(MY_SERVICE_OFF)+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
                        exit(EXIT_FAILURE);
                close(sock);
                exit(EXIT_SUCCESS);
            }   
            else if(strstr(command, "ts") != NULL){
                fprintf(stderr, "Sending message: %s\n", MY_SERVICE_OFF);

                if(sendto(sock, MY_SERVICE_OFF, strlen(MY_SERVICE_OFF)+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
                        exit(EXIT_FAILURE);
            }
            
            else
                fprintf(stderr, "Invalid Command\n");
                
        }

        if(FD_ISSET(sock, &rfds)) {
            memset(reply,0,strlen(reply));

            if(recvfrom(sock, reply, sizeof(reply), 0, (struct sockaddr*)&serveraddr,&addrlen)==-1)
                exit(EXIT_FAILURE);
            fprintf(stderr, "Received: %s\n", reply);

            if(strstr(reply, "OK") != NULL){

                sscanf(reply, "%*[^\' '] %[^\';'];%[^\';'];%s", id, ip, upt);

                if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL) exit(EXIT_FAILURE);
                serveraddr.sin_family = AF_INET;
                serveraddr.sin_addr.s_addr = inet_addr(ip);
                serveraddr.sin_port = htons((u_short)atoi(upt));
                addrlen = sizeof(serveraddr);

                fprintf(stderr, "Sending message: %s\n", MY_SERVICE_ON);
                if(sendto(sock, MY_SERVICE_ON, strlen(MY_SERVICE_ON)+1, 0, (struct sockaddr*)&serveraddr, addrlen)==-1)
                        exit(EXIT_FAILURE);

            }
            else if(strstr(reply, "YOUR_SERVICE") != NULL){

                if(strstr(reply, "OFF") != NULL){
                    if(memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr))==NULL) exit(EXIT_FAILURE);
                    serveraddr.sin_family = AF_INET;
                    serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr;
                    serveraddr.sin_port = htons((u_short)port);
                    addrlen = sizeof(serveraddr);

                    in_service = 0;
                }
                else if(strstr(reply, "ON") != NULL)
                    in_service = 1; 
            }
        }
    }

    close(sock);
    exit(EXIT_SUCCESS);
}
