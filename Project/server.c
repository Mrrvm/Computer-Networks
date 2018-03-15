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
#define SET_DS "SET_DS"

void show_usage(char *exe_name) {
    fprintf(stderr, "Usage: %s –n id –j ip -u upt –t tpt [-i csip] [-p cspt]\n", exe_name);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    int opt;
    char *id = NULL;
    int counter, maxfd, x;
    int my_port = -1, cli_port = -1, next_port = -1, sc_port = -1;
    int sc_addrlen, next_addrlen, my_addrlen, cli_addrlen;
    int sc_sock, next_sock, prev_sock, new_prev_sock, cli_sock;
    char *id_stup = NULL; 
    char *port_stup = NULL;
    int iam_stup = 0, iam_disp = 0;
    struct hostent *scptr = NULL, *myptr = NULL;
    enum {idle, busy, busy2} state;
    fd_set rfds;
    char command[64], pcommand[64], reply[64];
    char *ip_stup = NULL;
    char *sc_last_message = NULL;
    struct sockaddr_in sc_addr, next_addr, my_addr, cli_addr;

    while ((opt = getopt(argc, argv, "n:j:u:t:i:p:")) != -1) {
        switch (opt) {
            case 'n':
                id = optarg;
                break;
            case 'j':
                myptr = gethostbyname(optarg);
                break;
            case 'u':
                cli_port = atoi(optarg);
                break;
            case 't':
                my_port = atoi(optarg);
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

    if(id == NULL) show_usage(argv[0]);
    if(myptr == NULL) show_usage(argv[0]);
    if(cli_port == -1) show_usage(argv[0]);
    if(my_port == -1) show_usage(argv[0]);

    if(scptr == NULL) {
        scptr = gethostbyname(SC_DOMAIN);
        if(scptr ==NULL) 
            exit(EXIT_FAILURE);
    }
    if(sc_port == -1) {
        sc_port = SC_PORT;
    }

    /* Send to SC */
    sc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sc_sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&sc_addr, (int)'\0', sizeof(sc_addr))==NULL) exit(EXIT_FAILURE);
    sc_addr.sin_family = AF_INET;
    sc_addr.sin_addr.s_addr = ((struct in_addr*)(scptr->h_addr_list[0]))->s_addr;
    sc_addr.sin_port = htons((u_short)sc_port);
    sc_addrlen = sizeof(sc_addr);
    /* Receive from Client*/
    cli_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(cli_sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&cli_addr, (int)'\0', sizeof(cli_addr))==NULL) exit(EXIT_FAILURE);
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons((u_short)cli_port);
    cli_addrlen = sizeof(cli_addr);
    bind(cli_sock, (struct sockaddr*)&cli_addr, cli_addrlen);
    /* Receive from prev*/
    prev_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(prev_sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&my_addr, (int)'\0', sizeof(my_addr))==NULL) exit(EXIT_FAILURE);
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons((u_short)my_port);
    my_addrlen = sizeof(my_addr);
    bind(prev_sock, (struct sockaddr*)&my_addr, my_addrlen);
    listen(prev_sock, 5);
    
    state = idle;

    while(1){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;

        if(state == busy){FD_SET(sc_sock, &rfds); maxfd = max(0, sc_sock);}
        if(state == busy2){FD_SET(cli_sock, &rfds); maxfd = max(0, cli_sock);}

        counter = select(maxfd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        if(counter <= 0) exit(EXIT_FAILURE);

        /****************************************************/
        /* Talk to terminal */
        if(FD_ISSET(0, &rfds)){
            fgets(command, 64, stdin);

            if(strstr(command, "join") != NULL){
                state = busy;
                memset(reply,0,strlen(reply));
                sscanf(command, "%*[^\' '] %d", &x);
                sprintf(reply, "%s %d;%s", GET_START, x, id);

                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                    exit(EXIT_FAILURE);

                sc_last_message = GET_START;
            }

 
        }

        /****************************************************/
        /* Talk to SC */
        if(FD_ISSET(sc_sock, &rfds)) {
            memset(reply,0,strlen(reply));
            memset(pcommand,0,strlen(pcommand));

            if(recvfrom(sc_sock, reply, sizeof(reply), 0, (struct sockaddr*)&sc_addr, &sc_addrlen)==-1)
                exit(EXIT_FAILURE);
            fprintf(stderr, "Received: %s\n", reply);

            if(strstr(reply, "OK") != NULL){
                state = busy2;

                sscanf(reply, "%*[^\' '] %[^\';'];%[^\';'];%[^\';']%s", id, id_stup, ip_stup, port_stup);

                /* Becomes startup server */
                if(id_stup == "0" && strcmp(sc_last_message, GET_START) == 0) {

                    sprintf(pcommand, "%s %d;%s;%s;%d", SET_START, x, id, inet_ntoa((struct in_addr)my_addr.sin_addr), my_port);

                    fprintf(stderr, "Sending message: %s\n", pcommand);
                    if(sendto(sc_sock, pcommand, strlen(pcommand)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                        exit(EXIT_FAILURE);

                    sc_last_message = SET_START;
                }
                /* Connect to startup server*/
               /* else {
                    next_sock = socket(AF_INET, SOCK_STREAM, 0);
                    if(next_sock == -1) exit(EXIT_FAILURE);
                    if(memset((void*)&next_addr, (int)'\0', sizeof(next_addr))==NULL) exit(EXIT_FAILURE);
                    next_addr.sin_family = AF_INET;
                    next_addr.sin_addr.s_addr = ((struct in_addr*)(nextptr->h_addr_list[0]))->s_addr;
                    next_addr.sin_port = htons((u_short)next_port);
                    next_addrlen = sizeof(next_addr);
                    if(-1 != connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr))
                        exit(EXIT_FAILURE);
                }*/

                /* Acknowledges itself as startup*/
                if(id_stup == "0"&& strcmp(sc_last_message, SET_START)) {
                    iam_stup = 1;

                    sprintf(pcommand, "%s %d;%s;%s;%d", SET_DS, x, id, inet_ntoa((struct in_addr)my_addr.sin_addr), my_port);

                    fprintf(stderr, "Sending message: %s\n", pcommand);
                    if(sendto(sc_sock, pcommand, strlen(pcommand)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                        exit(EXIT_FAILURE);

                }

            }
        }
        if(FD_ISSET(cli_sock, &rfds)) {
            memset(reply,0,strlen(reply));
            memset(pcommand,0,strlen(pcommand));

            if(recvfrom(cli_sock, reply, sizeof(reply), 0, (struct sockaddr*)&cli_sock, &cli_addrlen)==-1)
                exit(EXIT_FAILURE);

            fprintf(stderr, "Received: %s\n", reply);

            if(strstr(reply, "MY_SERVICE") != NULL){
                sscanf(reply, "%*[^\' '] %s", pcommand);
                sprintf(pcommand, "YOUR_SERVICE %s", pcommand);

                fprintf(stderr, "Sending message: %s\n", pcommand);

                if(sendto(cli_sock, pcommand, strlen(pcommand)+1, 0, (struct sockaddr*)&cli_addr, cli_addrlen)==-1)
                    exit(EXIT_FAILURE);
            }

        }

    }  

    exit(EXIT_SUCCESS);

}