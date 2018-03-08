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
#define service 26
#define GET_START "GET_START"
#define SET_START "SET_START"

void show_usage(char *exe_name) {
    fprintf(stderr, "Usage: %s –n id –j ip -u upt –t tpt [-i csip] [-p cspt]\n", exe_name);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    int opt;
    int id = -1;
    int my_port = -1, client_port = -1, next_port = -1, sc_port = -1;
    int sc_addrlen, next_addrlen;
    int sc_sock;
    int id_stup, port_stup;
    int iam_stup = 0;
    struct hostent *scptr = NULL, *myptr = NULL;
    enum {idle, busy} state;
    fd_set rfds;
    char command[64], pcommand[64];
    char *ip_stup = NULL;
    struct sockaddr_in sc_addr, next_addr;

    while ((opt = getopt(argc, argv, "n:j:u:t:i:p:")) != -1) {
        switch (opt) {
            case 'n':
                id = atoi(optarg);
                break;
            case 'j':
                myptr = gethostbyname(optarg);
                break;
            case 'u':
                client_port = atoi(optarg);
                break;
            case 't':
                next_port = atoi(optarg);
                break;
            case 'i':
                scptr = gethostbyname(optarg);
                break;
            case 'p':
                sc_port = atoi(optarg);
                break;
            default: /* '?' */
                show_usage(argv[0]);
        }
    }

    if(id == -1) show_usage(argv[0]);
    if(myptr == NULL) show_usage(argv[0]);
    if(client_port == -1) show_usage(argv[0]);
    if(next_port == -1) show_usage(argv[0]);

    if(scptr == NULL) {
        scptr = gethostbyname(SC_DOMAIN);
        if(scptr ==NULL) 
            exit(EXIT_FAILURE);
    }
    if(sc_port == -1) {
        sc_port = SC_PORT;
    }

    sc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sc_sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&sc_addr, (int)'\0', sizeof(sc_addr))==NULL) exit(EXIT_FAILURE);
    sc_addr.sin_family = AF_INET;
    sc_addr.sin_addr.s_addr = ((struct in_addr*)(scptr->h_addr_list[0]))->s_addr;
    sc_addr.sin_port = htons((u_short)sc_port);
    sc_addrlen = sizeof(sc_addr);

    next_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(next_sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&next_addr, (int)'\0', sizeof(next_addr))==NULL) exit(EXIT_FAILURE);
    next_addr.sin_family = AF_INET;
    next_addr.sin_addr.s_addr = ((struct in_addr*)(nextptr->h_addr_list[0]))->s_addr;
    next_addr.sin_port = htons((u_short)next_port);
    next_addrlen = sizeof(next_addr);

    state = idle;

    while(1){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;

        if(state == busy){
            FD_SET(sc_sock, &rfds);
            maxfd = max(0, sc_sock);
        }

        counter = select(maxfd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        if(counter <= 0) exit(EXIT_FAILURE);

        if(FD_ISSET(0, &rfds)){
            fgets(command, 64, stdin);
            state = busy;

            if(strstr(command, "join") != NULL){
                memset(reply,0,strlen(reply));
                sscanf(command, "%*[^\' '] %d", &x);
                sprintf(reply, "%s %d;%d", GET_START, x, id);

                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                    exit(EXIT_FAILURE);
            }

 
        }
        if(FD_ISSET(sc_sock, &rfds)) {
            memset(reply,0,strlen(reply));

            if(recvfrom(sc_sock, reply, sizeof(reply), 0, (struct sockaddr*)&serveraddr, &sc_addrlen)==-1)
                exit(EXIT_FAILURE);

            fprintf(stderr, "Received: %s\n", reply);

            if(strstr(reply, "OK") != NULL){

                sscanf(reply, "%*[^\' '] %[^\';'];%[^\';'];%[^\';']%d", id, id_stup, ip_stup, port_stup);

                if(id_stup == 0) {

                    sprintf(pcommand, "%s %d;%d;%s;%d", SET_START, service, id, inet_ntoa((struct in_addr)next_addr.sin_addr), next_port);

                    if(sendto(sc_sock, pcommand, strlen(pcommand)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                        exit(EXIT_FAILURE);

                    iam_stup = 1;
                }
                else {



                }

            }
        }
    }  

    close(sock);
    exit(EXIT_SUCCESS);

}