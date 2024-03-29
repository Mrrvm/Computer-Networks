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
#include <errno.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define SC_PORT 59000
#define SC_IP "193.136.138.142"
#define LOCALHOST "127.0.0.1"
#define service 26
#define GET_START "GET_START"
#define SET_START "SET_START"
#define SET_DS "SET_DS"
#define NONE 0
#define GETTING_START 1
#define SETTING_DS 2

//./service -n 1 -j localhost -u 59002 -t 59001


void spawn_error(char *error) {
    fprintf(stderr, "Error: %s\n", error);
    fprintf(stderr, "Error: %s\n", strerror(errno));
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

    int counter, maxfd, x, im_alone = 0, res = 0, opt = 0;
    int sc_next_message = NONE, sc_last_message = NONE;
    int my_port = -1, cli_port = -1, stup_port = -1, sc_port = SC_PORT, prev_port = -1;
    int sc_sock, next_sock, prev_sock, new_prev_sock, cli_sock;
    int is_ds = 0, is_stp = 0, is_ring_av = 1, is_av = 1;
    unsigned int sc_addrlen, next_addrlen, my_addrlen, cli_addrlen;
    struct hostent *ptr = NULL;
    struct sockaddr_in sc_addr, next_addr, my_addr, cli_addr;
    enum {idle, available, accepted} state;
    fd_set rfds;
    char *tmp = NULL;
    char *id = NULL;
    char my_ip[64] = {0}, sc_ip[64] = {0}, prev_ip[64] = {0}, stup_ip[64] = {0};
    char command[64] = {0}, reply[64] = {0};
    char stup_id[4] = {0}, prev_id[4] = {0}, origin_id[4] = {0};
    char aux[8] = {0}, type[4] = {0};

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
                    spawn_error("-u argument must me an integer");
                
                break;
            case 't':
                if(isdigit(*optarg))
                    my_port = atoi(optarg);
                else 
                    spawn_error("-u argument must me an integer");
                break;
            case 'i':
                ptr = gethostbyname(optarg);
                sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
                break;
            case 'p':
                if(isdigit(*optarg))
                    sc_port = atoi(optarg);
                else 
                    spawn_error("-u argument must me an integer");
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
    if(sc_sock ==-1) spawn_error("Error creating SC socket");
    if(memset((void*)&sc_addr, (int)'\0', sizeof(sc_addr))==NULL) spawn_error("Error memsetting");
    sc_addr.sin_family = AF_INET;
    sc_addr.sin_addr.s_addr = inet_addr(sc_ip);
    sc_addr.sin_port = htons((u_short)sc_port);
    sc_addrlen = sizeof(sc_addr);

    /* Receive from Client*/
    cli_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(cli_sock ==-1) spawn_error("Error creating Client Socket");
    if(memset((void*)&cli_addr, (int)'\0', sizeof(cli_addr))==NULL) spawn_error("Error memsetting");
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons((u_short)cli_port);
    cli_addrlen = sizeof(cli_addr);
    if(bind(cli_sock, (struct sockaddr*)&cli_addr, cli_addrlen) == -1) spawn_error("Error binding Client Socket");

    /* Receive from prev*/
    prev_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(prev_sock ==-1) spawn_error("Error creating prev socket");
    if(memset((void*)&my_addr, (int)'\0', sizeof(my_addr))==NULL) spawn_error("Error memsetting");
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons((u_short)my_port);
    my_addrlen = sizeof(my_addr);
    if(bind(prev_sock, (struct sockaddr*)&my_addr, my_addrlen) == -1) spawn_error("Error binding Prev Socket");

    listen(prev_sock, 5);
    
    state = idle;
    while(1){

        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;

        if(state == available || state == accepted){
            FD_SET(sc_sock, &rfds);
            FD_SET(prev_sock, &rfds);
            FD_SET(cli_sock, &rfds);
            maxfd = max(0, sc_sock);
            maxfd = max(maxfd, prev_sock);
            maxfd = max(maxfd, cli_sock);
        }
        if(state == accepted) {
            FD_SET(new_prev_sock, &rfds);
            maxfd = max(maxfd, new_prev_sock);
        }

        counter = select(maxfd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        if(counter <= 0) spawn_error("Error in select");
        
        memset(command, 0, strlen(reply));
        memset(reply, 0, strlen(reply));

        /****************************************************/
        /* Talk to terminal */
        if(FD_ISSET(0, &rfds)){
            fgets(command, 64, stdin);

            if(strstr(command, "join") != NULL){
                state = available;

                sscanf(command, "%*[^\' '] %d", &x);

                sprintf(reply, "%s %d;%s", GET_START, x, id);
                fprintf(stderr, "Sending message: %s\n", reply);

                if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen) == -1) spawn_error("Error sending to SC");
                sc_next_message = GETTING_START;
            }
            else if(strstr(command, "exit") != NULL){
                close(sc_sock); 
                close(next_sock);
                close(new_prev_sock);
                close(prev_sock);
                close(cli_sock);
                exit(EXIT_SUCCESS);
            }
        }

        /****************************************************/
        /* Talk to SC */
        if(FD_ISSET(sc_sock, &rfds)) {
            if(recvfrom(sc_sock, command, sizeof(command), 0, (struct sockaddr*)&sc_addr, &sc_addrlen) == -1) spawn_error("Error receiving from SC");
            fprintf(stderr, "Received: %s\n", command);

            if(strstr(command, "OK") != NULL){

                sscanf(command, "%*[^\' '] %*[^\';'];%[^\';'];%[^\';'];%d", stup_id, stup_ip, &stup_port);

                /* Becomes startup server */
                if(sc_next_message == GETTING_START) {
                    if(strcmp(stup_id, "0") == 0){
                        im_alone = 1;
       
                        sprintf(reply, "%s %d;%s;%s;%d", SET_START, x, id, my_ip, my_port);

                        fprintf(stderr, "Sending message: %s\n", reply);
                        if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen)==-1) spawn_error("Error sending to SC");

                        sc_last_message = sc_next_message;
                        sc_next_message = SETTING_DS;
                    }
                    /* Connect to startup server*/
                    else {
                        sprintf(reply, "NEW %s;%s;%d\n", id, my_ip, my_port);
                        next_sock = socket(AF_INET, SOCK_STREAM, 0);
                        if(next_sock == -1) spawn_error("Error creating next socket");
                        if(memset((void*)&next_addr, (int)'\0', sizeof(next_addr))==NULL) spawn_error("Error memsetting");
                        next_addr.sin_family = AF_INET;
                        next_addr.sin_addr.s_addr = inet_addr(stup_ip);
                        next_addr.sin_port = htons((u_short)stup_port);
                        next_addrlen = sizeof(next_addr);
                        fprintf(stderr, "Connecting to %s:%d\n", stup_ip, stup_port);
                        if(connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr)) == -1) spawn_error("Error connecting to next");
                        if(write(next_sock, reply, strlen(reply)) == -1) spawn_error("Error writing to next");
                        fprintf(stderr, "Sending message: %s\n", reply);          

                    }
                }

                /* Acknowledges itself as startup*/
                else if(sc_next_message == SETTING_DS) {

                    if(sc_last_message == GETTING_START) {
                        is_stp = 1;
                    }

                    sprintf(reply, "%s %d;%s;%s;%d", SET_DS, x, id, my_ip, cli_port);

                    fprintf(stderr, "Sending message: %s\n", reply);
                    if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen) == -1) spawn_error("Error sending to SC");

                    sc_last_message = sc_next_message;
                    sc_next_message = NONE;

                }

                // Confirms status as DS server
                else if(sc_last_message == SETTING_DS) {
                    is_ds = 1;
                    sc_last_message = NONE;
                }

            }
        }

        /****************************************************/
        /* Talk to client */
        if(FD_ISSET(cli_sock, &rfds)) {

            if(recvfrom(cli_sock, command, sizeof(command), 0, (struct sockaddr*)&cli_addr, &cli_addrlen)==-1) spawn_error("Error receiving from Client");
            fprintf(stderr, "Received: %s\n", command);

            if(strstr(command, "MY_SERVICE") != NULL){

                if(strstr(command, "ON") != NULL){   
                    // Tells SC that I am not dispatch anymore
                    sprintf(reply, "WITHDRAW_DS %d;%s", service, id);
                    fprintf(stderr, "Sending message: %s\n", reply);
                    if(sendto(sc_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&sc_addr, sc_addrlen) == -1) spawn_error("Could not send to SC\n");

                    // Sends token S to next server
                    if(!im_alone) {
                        memset(reply, 0, strlen(reply));
                        sprintf(reply, "TOKEN %s;%s\n", id, "S");
                        fprintf(stderr, "Sending message: %s\n", reply);
                        if(sendto(next_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&next_addr, next_addrlen) == -1) spawn_error("Could not send to next server\n");
                    }

                    // Tells client service is on
                    memset(reply, 0, strlen(reply));
                    sscanf(command, "%*[^\' '] %s", aux);
                    sprintf(reply, "YOUR_SERVICE %s", aux);
                    fprintf(stderr, "Sending message: %s\n", reply);
                    if(sendto(cli_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&cli_addr, cli_addrlen) == -1) spawn_error("Could not send to client\n");
                
                    is_av = 0;
                    is_ds = 0;
                }
                else {
                    // Tells client service is on
                    memset(reply, 0, strlen(reply));
                    sscanf(command, "%*[^\' '] %s", aux);
                    sprintf(reply, "YOUR_SERVICE %s", aux);
                    fprintf(stderr, "Sending message: %s\n", reply);
                    if(sendto(cli_sock, reply, strlen(reply)+1, 0, (struct sockaddr*)&cli_addr, cli_addrlen) == -1) spawn_error("Could not send to client\n");
                
                    is_av = 1;
                }             
            }

        }

        /****************************************************/
        /* Receive connections from previous servers */
        if(FD_ISSET(prev_sock, &rfds)) {
            new_prev_sock = accept(prev_sock, (struct sockaddr*)&my_addr, &my_addrlen);
            if(new_prev_sock == -1) spawn_error("Error accepting prev");
            fprintf(stderr, "Previous Server connected\n");
            state = accepted;
        }

        /****************************************************/
        /* Talk to previous server */
        if(state == accepted && FD_ISSET(new_prev_sock, &rfds)) {
            
            res = read(new_prev_sock, command, sizeof(command));
            if(res == -1) spawn_error("Error reading from prev");
            else if(res == 0) {close(new_prev_sock); state = available;}
            else{

                fprintf(stderr, "Received: %s\n", command);

                if(strstr(command, "NEW") != NULL){
                    sscanf(command, "%*[^\' '] %[^\';'];%[^\';'];%d", prev_id, prev_ip, &prev_port);

                    if(im_alone){
                        strncpy(stup_id, prev_id, sizeof(prev_id));
                        strncpy(stup_ip,  prev_ip, sizeof(prev_ip));
                        stup_port = prev_port;
                        next_sock = socket(AF_INET, SOCK_STREAM, 0);
                        if(next_sock == -1) spawn_error("Error creating next socket");
                        if(memset((void*)&next_addr, (int)'\0', sizeof(next_addr))==NULL) spawn_error("Error memsetting");
                        next_addr.sin_family = AF_INET;
                        next_addr.sin_addr.s_addr = inet_addr(stup_ip);
                        next_addr.sin_port = htons((u_short)stup_port);
                        next_addrlen = sizeof(next_addr);
                        fprintf(stderr, "Connecting to %s:%d\n", stup_ip, stup_port);
                        if(connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr)) == -1) spawn_error("Could not connect to startup server");
                        im_alone = 0;
                    }
                    else{
                        sprintf(reply, "TOKEN %s;N;%s;%s;%d\n", id, prev_id, prev_ip, prev_port);
                        fprintf(stderr, "Sending message: %s\n", reply);
                        if(write(next_sock, reply, strlen(reply)) == -1) spawn_error("Could not write to next server");
                    }
                    
                }
                else if(strstr(command, "TOKEN") != NULL){
                    tmp = strstr(command, ";");
                    if(strstr(tmp, "N")){
                        sscanf(command, "%*[^\' '] %[^\';'];%[^\';'];%[^\';'];%[^\';'];%d", origin_id, type, prev_id, prev_ip, &prev_port);
                        //sscanf(command, "%*[^\' '] %[^\';'];%[^\';'];", origin_id, type);

                        if(strcmp(origin_id, stup_id) == 0){
                            strncpy(stup_id, prev_id, sizeof(prev_id));
                            strncpy(stup_ip,  prev_ip, sizeof(prev_ip));
                            stup_port = prev_port;
                            next_sock = socket(AF_INET, SOCK_STREAM, 0);
                            if(next_sock == -1) spawn_error("Error creating next socket");
                            if(memset((void*)&next_addr, (int)'\0', sizeof(next_addr))==NULL) spawn_error("Error memsetting");
                            next_addr.sin_family = AF_INET;
                            next_addr.sin_addr.s_addr = inet_addr(stup_ip);
                            next_addr.sin_port = htons((u_short)stup_port);
                            next_addrlen = sizeof(next_addr);
                            fprintf(stderr, "Connecting to %s:%d\n", stup_ip, stup_port);
                            if(connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr)) == -1) spawn_error("Could not connect to startup server");

                        }
                        else{
                            if(write(next_sock, command, strlen(command)) == -1) spawn_error("Could not write to next server");

                        }
                        
                    }
                    else if(strstr(tmp, "S")) {
                        
                    }

                }
            }

        }


    }  

    exit(EXIT_SUCCESS);

}