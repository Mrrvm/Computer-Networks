#include "defs.h"

int service = 0;

void send_msg(int type, int sock, struct sockaddr_in addr) {

    int addrlen = sizeof(addr);
    char msg[64] = {0};

    if(type == GET_DS_SERVER) {
        sprintf(msg, "GET_DS_SERVER %d", service);
        if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Could not send GET_DS_SERVER\n");
        fprintf(stderr, KCYN"SENT\t"RESET"%s\n", msg);
    }
    else if(type == MY_SERVICE_ON) {
        sprintf(msg, "MY_SERVICE_ON");
        if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Could not send MY_SERVICE_ON\n");
        fprintf(stderr, KCYN"SENT\t"RESET"%s\n", msg);
    }
    else if(type == MY_SERVICE_OFF) {
        sprintf(msg, "MY_SERVICE OFF");
        if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Could not send MY_SERVICE OFF\n");
        fprintf(stderr, KCYN"SENT\t"RESET"%s\n", msg);
    }
}

struct sockaddr_in define_AF_INET_conn(int *sock, int type, int port, char *ip) {

    struct sockaddr_in addr;

    *sock = socket(AF_INET, type, 0);
    if(*sock == -1) spawn_error("Cannot create socket");

    if(memset((void*)&addr, (int)'\0', sizeof(addr)) == NULL) spawn_error("Cannot memset");
    addr.sin_family = AF_INET;
    if(ip == NULL) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        addr.sin_addr.s_addr = inet_addr(ip);
    }
    addr.sin_port = htons((u_short)port);

    return addr;
}

int main(int argc, char *argv[])
{
    int opt;

    
    unsigned int addrlen = 0;
    int maxfd, counter;
    int last_msg = NONE;
    struct timeval tv;
    struct hostent *hostptr = NULL;
    fd_set rfds;
    char msg[64] = {0}, ip[64] = {0}, sc_ip[64] = {0};
    int id;
    /// Ports
    int port = -1;
    int upt;
    /// Socket
    int sock;
    ///sockaddr
    struct sockaddr_in serveraddr;
    /// State
    enum {idle, busy} state = idle;
    /// Conditions
    int im_exiting = 0, in_service = 0;

    /// Set SC IP by default
    hostptr = gethostbyname(SC_IP);
    sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)hostptr->h_addr_list[0]));

    /// Get the arguments used
    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
        switch (opt) {
            case 'i':
                hostptr = gethostbyname(optarg);
                sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)hostptr->h_addr_list[0]));
                break;
            case 'p':
                if(isdigit(*optarg)) port = atoi(optarg);
                else spawn_error("-u argument must be an integer");
                break;
            default: /// ?
                show_usage(argv[0]);
        }
    }

    /// Define the connection to the central server
    if(port == -1) port = SC_PORT;
    serveraddr = define_AF_INET_conn(&sock, SOCK_DGRAM, port, sc_ip);

    fprintf(stderr, "Arguments %s, %d\n", inet_ntoa(serveraddr.sin_addr), port);


    while(1){
        
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;

        if(state == busy){
            FD_SET(sock, &rfds);
            maxfd = max(0,sock);
        }

        if(last_msg == NONE){
        /// Define select with no timer
            printf("NONE\n");
            counter = select(maxfd+1, &rfds,  (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        }
        else {
        /// Define select with a timer of 3 seconds
            printf("SOME\n");
            tv.tv_sec = 3;
            tv.tv_usec = 0;
            counter = select(maxfd+1, &rfds,  (fd_set*)NULL, (fd_set*)NULL, &tv);
        }

        /// Clean message
        memset(msg,0,strlen(msg));

        if(counter < 0) spawn_error("select() failed");
        else if(counter == 0) {
            fprintf(stderr, "Timed out\n");
            send_msg(last_msg, sock, serveraddr);
        }
        else if(counter > 0) {

            /////////////////////////////////////////////////
            /// Terminal Interface
            /////////////////////////////////////////////////            
            if(FD_ISSET(0, &rfds)){
                fgets(msg, 64, stdin);
                state = busy;

                /// Request service
                if(strstr(msg, "rs") != NULL && in_service == 0){
                    
                    sscanf(msg, "%*[^\' '] %d", &service);
                    send_msg(GET_DS_SERVER, sock, serveraddr);
                    last_msg = GET_DS_SERVER;

                }
                else if(strstr(msg, "exit") != NULL){
                    if(in_service == 0){
                    /// Close imediately
                        close(sock);
                        exit(EXIT_SUCCESS);
                    }
                    else {
                    /// Warn the service
                        send_msg(MY_SERVICE_OFF, sock, serveraddr);
                        last_msg = MY_SERVICE_OFF;
                        im_exiting = 1;  
                    }
                }   
                else if(strstr(msg, "ts") != NULL){
                    /// Warn the service
                    send_msg(MY_SERVICE_OFF, sock, serveraddr);
                    last_msg = MY_SERVICE_OFF;
                    im_exiting = 0;
                }
                
                else
                    fprintf(stderr, "Invalid Command\n");
                    
            }

            /////////////////////////////////////////////////
            /// SC/Service Interface
            /////////////////////////////////////////////////
            if(FD_ISSET(sock, &rfds)) {

                if(recvfrom(sock, msg, sizeof(msg), 0, (struct sockaddr*)&serveraddr,&addrlen)==-1)
                    exit(EXIT_FAILURE);
                fprintf(stderr, KYEL"RECV\t"RESET"%s\n", msg);

                /// Received OK
                if(strstr(msg, "OK") != NULL){

                    sscanf(msg, "%*[^\' '] %d;%[^\';'];%d", &id, ip, &upt);
                    if(id == 0){
                    /// No services available
                        fprintf(stderr, "There are no servers available for this service\n");
                    }
                    else {
                        /// Close connection with SC, open it with service
                        close(sock);
                        serveraddr = define_AF_INET_conn(&sock, SOCK_DGRAM, upt, ip);
                        /// Warn service
                        send_msg(MY_SERVICE_ON, sock, serveraddr);
                        last_msg = MY_SERVICE_ON;
                    }
                }
                /// Received message orm service
                else if(strstr(msg, "YOUR_SERVICE") != NULL){

                    if(strstr(msg, "OFF") != NULL){
                        /// Close connection with service
                        close(sock);
                        if(im_exiting) exit(EXIT_SUCCESS);
                        /// Reconnect with SC
                        serveraddr = define_AF_INET_conn(&sock, SOCK_DGRAM, port, sc_ip);
                        in_service = 0;
                    }
                    else if(strstr(msg, "ON") != NULL)
                        in_service = 1; 
                        
                    last_msg = NONE;
                }
            }
            
        }
    }
}
