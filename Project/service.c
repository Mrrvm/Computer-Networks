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
#include <net/if.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define SC_PORT 59000
#define SC_IP "193.136.138.142"
#define LOCALHOST "127.0.0.1"
#define service 26
#define GET_START "GET_START"
#define SET_START "SET_START"
#define SET_DS "SET_DS"
#define NOTHING 0
#define GETTING_START 1
#define SETTING_DS 2

void spawn_error(char *error) {
    fprintf(stderr, "Error: %s", error);
    exit(EXIT_FAILURE);
}

void show_usage(char *exe_name) {
    fprintf(stderr, "Usage: %s –n id –j ip -u upt –t tpt [-i csip] [-p cspt]\n", exe_name);
    exit(EXIT_FAILURE);
}

// Get real IP from machine
void getmyip(char my_ip[]) {
    
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[64];

    if(getifaddrs(&ifaddr) == -1) {exit(EXIT_FAILURE);}

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    
        if(ifa->ifa_addr == NULL)
            continue;

        if ((strcmp("lo", ifa->ifa_name) == 0) ||
        !(ifa->ifa_flags & (IFF_RUNNING)))
            continue;

        family = ifa->ifa_addr->sa_family;

        if(family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr,
               sizeof(struct sockaddr_in),
               host, 64,
               NULL, 0, NI_NUMERICHOST);
            if(s != 0) {exit(EXIT_FAILURE);}
       } 
   }

   freeifaddrs(ifaddr);
   strcpy(my_ip, host);
}

int main(int argc, char *argv[]) {

    int opt;
    int counter, maxfd, x;
    int sc_next_message = 0;
    int my_port = -1, cli_port = -1, sc_port = SC_PORT, stup_port, prev_port;
    int sc_sock, next_sock, prev_sock, new_prev_sock, cli_sock;
    int is_ds = 0, is_stp = 0, is_ring_av = 1;
    unsigned int sc_addrlen, next_addrlen, my_addrlen, cli_addrlen;
    struct hostent *ptr = NULL;
    struct sockaddr_in sc_addr, next_addr, my_addr, cli_addr;
    enum {idle, available} state;
    fd_set rfds;
    char *id = NULL;
    char my_ip[64] = {0}, sc_ip[64] = {0}, prev_ip[64] = {0}, stup_ip[64] = {0};
    char command[64] = {0}, reply[64] = {0};
    char stup_id[4] = {0}, prev_id[4];
    char aux[8] = {0};

    sprintf(sc_ip, "%s", SC_IP);

    // Get the right arguments
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
                else 
                    spawn_error("-u argument must me an integer\n");
                
                break;
            case 't':
                if(isdigit(*optarg))
                    my_port = atoi(optarg);
                else 
                    spawn_error("-u argument must me an integer\n");
                break;
            case 'i':
                ptr = gethostbyname(optarg);
                sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
                break;
            case 'p':
                if(isdigit(*optarg))
                    sc_port = atoi(optarg);
                else 
                    spawn_error("-u argument must me an integer\n");
                break;
            default: /* '?' */
                show_usage(argv[0]);
        }
    }
    
    // Verify if usage rules are met
    if(id == NULL) show_usage(argv[0]);
    if(my_ip == NULL) show_usage(argv[0]);
    if(cli_port == -1) show_usage(argv[0]);
    if(my_port == -1) show_usage(argv[0]);
    if(strcmp(my_ip, LOCALHOST) == 0) {getmyip(my_ip);}

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
        
        memset(command,0,strlen(reply));
        memset(reply,0,strlen(reply));

        /****************************************************/
        /* Talk to terminal */
        if(FD_ISSET(0, &rfds)){
            fgets(command, 64, stdin);

            if(strstr(command, "join") != NULL){
                state = available;

                sscanf(command, "%*[^\' '] %d", &x);

                sprintf(reply, "%s %d;%s", GET_START, x, id);
                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen) == -1) spawn_error("Could not send to SC\n");

                sc_next_message = GETTING_START;
            }

 
        }

        /****************************************************/
        /* Talk to SC */
        if(FD_ISSET(sc_sock, &rfds)) {

            if(recvfrom(sc_sock, command, sizeof(command), 0, (struct sockaddr*)&sc_addr, &sc_addrlen) == -1) spawn_error("Could not receive from SC\n");
            fprintf(stderr, "Received: %s\n", command);

            if(strstr(command, "OK") != NULL){

                sscanf(command, "%*[^\' '] %*[^\';'];%[^\';'];%[^\';'];%d", stup_id, stup_ip, stup_port);

                /* Becomes startup server */
                if(sc_next_message == GETTING_START) {
                    if(strcmp(stup_id, "0") == 0){
       
                        sprintf(reply, "%s %d;%s;%s;%d", SET_START, x, id, my_ip, my_port);

                        fprintf(stderr, "Sending message: %s\n", reply);
                        if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1) spawn_error("Could not receive from SC\n");

                        sc_next_message = SETTING_DS;
                    }
                    /* Connect to startup server*/
                    else {
                        next_sock = socket(AF_INET, SOCK_STREAM, 0);
                        if(next_sock == -1) exit(EXIT_FAILURE);
                        if(memset((void*)&next_addr, (int)'\0', sizeof(next_addr))==NULL) exit(EXIT_FAILURE);
                        next_addr.sin_family = AF_INET;
                        next_addr.sin_addr.s_addr = inet_addr(stup_ip);
                        next_addr.sin_port = htons((u_short)stup_port);
                        next_addrlen = sizeof(next_addr);
                        printf("Connecting to %s:%d\n", stup_ip, stup_port);
                        if(connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr)) != -1) spawn_error("Could not connect to startup server\n");
                    }
                }

                /* Acknowledges itself as startup*/
                else if(sc_next_message == SETTING_DS) {

                    sprintf(reply, "%s %d;%s;%s;%d", SET_DS, x, id, my_ip, cli_port);

                    fprintf(stderr, "Sending message: %s\n", reply);
                    if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen) == -1) spawn_error("Could not send to SC\n");

                    sc_next_message = NOTHING;

                }

            }
        }

        /****************************************************/
        /* Talk to client */
        if(FD_ISSET(cli_sock, &rfds)) {

            if(recvfrom(cli_sock, command, sizeof(command), 0, (struct sockaddr*)&cli_addr, &cli_addrlen)==-1) spawn_error("Could not receive from client\n");
            fprintf(stderr, "Received: %s\n", command);

            if(strstr(command, "MY_SERVICE") != NULL){

                sscanf(command, "%*[^\' '] %s", aux);
                sprintf(reply, "YOUR_SERVICE %s", aux);

                fprintf(stderr, "Sending message: %s\n", reply);
                if(sendto(cli_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&cli_addr, cli_addrlen) == -1) spawn_error("Could not send to client\n");
            }

        }

        /****************************************************/
        /* Receive connections from previous servers */
        if(FD_ISSET(prev_sock, &rfds)) {
            new_prev_sock = accept(prev_sock, (struct sockaddr*)&my_addr, &my_addrlen);
            if(new_prev_sock == -1) spawn_error("Could not accept socket\n");
        }

        /****************************************************/
        /* Talk to previous server */
        if(FD_ISSET(new_prev_sock, &rfds)) {
            
            if(read(new_prev_sock, command, strlen(command)+1) == -1) spawn_error("Could not read from previous server\n");
            fprintf(stderr, "Received: %s\n", command);

            if(strstr(command, "NEW") != NULL){
                sscanf(command, "%*[^\' '] %*[^\';'];%d", prev_id, prev_ip, prev_port);
                printf("%d\n", prev_port);
            }
        }

    }  

    exit(EXIT_SUCCESS);

}