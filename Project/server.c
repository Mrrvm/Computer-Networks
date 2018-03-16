#include <unistd.h>
#include <linux/if_link.h>
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
#include <ifaddrs.h>
#include <time.h>
#include <ctype.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define SC_PORT 59000
#define SC_IP "193.136.138.142"
#define service 26
#define GET_START "GET_START"
#define SET_START "SET_START"
#define SET_DS "SET_DS"
#define NOTHING 0
#define GETTING_START 1
#define SETTING_DS 2

void show_usage(char *exe_name) {
    fprintf(stderr, "Usage: %s –n id –j ip -u upt –t tpt [-i csip] [-p cspt]\n", exe_name);
    exit(EXIT_FAILURE);
}

void getmyip(char my_ip[]) {
    
    struct ifaddrs *ifaddr, *ifa;
    int family, s, n = 0;
    char host[64];

    if(getifaddrs(&ifaddr) == -1) {exit(EXIT_FAILURE);}

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if(n == 2) break;
        if(ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if(family == AF_INET) {
            n++;
            s = getnameinfo(ifa->ifa_addr,
               sizeof(struct sockaddr_in),
               host, NI_MAXHOST,
               NULL, 0, NI_NUMERICHOST);
            if(s != 0) {exit(EXIT_FAILURE);}
       } 
   }

   freeifaddrs(ifaddr);
   strcpy(my_ip, host);
}

int main(int argc, char *argv[])
{
    int opt;
    int counter, maxfd, x;
    int sc_next_message = 0;
    int my_port = -1, cli_port = -1, next_port = -1, sc_port = SC_PORT;
    unsigned int sc_addrlen, next_addrlen, my_addrlen, cli_addrlen;
    int sc_sock, next_sock, prev_sock, new_prev_sock, cli_sock;
    int is_ds = 0, is_stp = 0, is_ring_av = 1;
    struct hostent *ptr = NULL;
    enum {idle, available} state;
    fd_set rfds;
    char *id = NULL;
    char my_ip[64] = {0}, sc_ip[64] = {0};
    char command[64] = {0}, pcommand[64] = {0}, reply[64] = {0}, ip_stup[64] = {0}, id_stup[64] = {0}, port_stup[64] = {0};
    struct sockaddr_in sc_addr, next_addr, my_addr, cli_addr;

    sprintf(sc_ip, "%s", SC_IP);

    while ((opt = getopt(argc, argv, "n:j:u:t:i:p:")) != -1) {
        switch (opt) {
            case 'n':
                id = optarg;
                break;
            case 'j':
                ptr = gethostbyname(optarg);
                sprintf(my_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
                break;
            case 'u':
                if(isdigit(*optarg)) {
                    cli_port = atoi(optarg);
                }
                else {
                    printf("Error: -u argument must me an integer\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                if(isdigit(*optarg))
                    my_port = atoi(optarg);
                else {
                    printf("Error: -u argument must me an integer\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                ptr = gethostbyname(optarg);
                sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
                break;
            case 'p':
                if(isdigit(*optarg))
                    sc_port = atoi(optarg);
                else {
                    printf("Error: -u argument must me an integer\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default: /* '?' */
                show_usage(argv[0]);
        }
    }
    
    if(id == NULL) show_usage(argv[0]);
    if(my_ip == NULL) show_usage(argv[0]);
    if(cli_port == -1) show_usage(argv[0]);
    if(my_port == -1) show_usage(argv[0]);
    if(strcmp(my_ip, "127.0.0.1") == 0) {getmyip(my_ip);}

    /* Send to SC */
    sc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sc_sock ==-1) exit(EXIT_FAILURE);
    if(memset((void*)&sc_addr, (int)'\0', sizeof(sc_addr))==NULL) exit(EXIT_FAILURE);
    sc_addr.sin_family = AF_INET;
    sc_addr.sin_addr.s_addr = inet_addr(sc_ip);
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

        if(state == available){
            FD_SET(sc_sock, &rfds);
            FD_SET(prev_sock, &rfds);
            FD_SET(cli_sock, &rfds);
            maxfd = max(0, sc_sock);
            maxfd = max(maxfd, prev_sock);
            maxfd = max(maxfd, cli_sock);
        }

        counter = select(maxfd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        if(counter <= 0) exit(EXIT_FAILURE);

        /****************************************************/
        /* Talk to terminal */
        if(FD_ISSET(0, &rfds)){
            fgets(command, 64, stdin);

            if(strstr(command, "join") != NULL){
                state = available;
                memset(reply,0,strlen(reply));
                sscanf(command, "%*[^\' '] %d", &x);
                sprintf(reply, "%s %d;%s", GET_START, x, id);

                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                    exit(EXIT_FAILURE);

                sc_next_message = GETTING_START;
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

                sscanf(reply, "%*[^\' '] %*[^\';'];%[^\';'];%[^\';']%s", id_stup, ip_stup, port_stup);

                /* Becomes startup server */
                if(sc_next_message == GETTING_START) {
                    if(strcmp(id_stup, "0") == 0){

                        sprintf(pcommand, "%s %d;%s;%s;%d", SET_START, x, id, my_ip, my_port);

                        fprintf(stderr, "Sending message: %s\n", pcommand);
                        if(sendto(sc_sock, pcommand, strlen(pcommand)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                            exit(EXIT_FAILURE);

                        sc_next_message = SETTING_DS;
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
                }

                /* Acknowledges itself as startup*/
                else if(sc_next_message == SETTING_DS) {

                    sprintf(pcommand, "%s %d;%s;%s;%d", SET_DS, x, id, my_ip, cli_port);

                    fprintf(stderr, "Sending message: %s\n", pcommand);
                    if(sendto(sc_sock, pcommand, strlen(pcommand)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1)
                        exit(EXIT_FAILURE);

                    sc_next_message = NOTHING;

                }

            }
        }

        /****************************************************/
        /* Talk to client */
        if(FD_ISSET(cli_sock, &rfds)) {
            memset(reply,0,strlen(reply));
            memset(pcommand,0,strlen(pcommand));

            if(recvfrom(cli_sock, reply, sizeof(reply), 0, (struct sockaddr*)&cli_addr, &cli_addrlen)==-1)
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