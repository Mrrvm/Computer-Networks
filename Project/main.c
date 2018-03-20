#include "sockets.h"

/// Ports
int cli_port = 0, sc_port = SC_PORT, next_port = 0, 
	prev_port = 0, prev_server_tport = 0, stup_port = 0;
/// IPs
char my_ip[64] = {0}, sc_ip[64] = {0}, prev_ip[64] = {0}, stup_ip[64] = {0};
/// IDs
int my_id = -1, prev_id = -1, stup_id = -1;
/// Service number
int service = 0;

int main(int argc, char *argv[]) {
	
	int opt = 0, maxfd = 0, counter = 0;
	struct hostent *ptr = NULL;
	fd_set rfds;
	char msg[64] = {0};
	int next_msg = NONE, last_msg = NONE;
	unsigned int addrlen = 0;
	/// Sockets
	int sc_sock = 0, cli_sock = 0, next_sock = 0, prev_sock = 0;
	/// sockaddr
	struct sockaddr_in sc_addr, next_addr, prev_addr, cli_addr;
	/// State
	enum {off, joined, accepted} state = off;
	/// Conditions
	int im_alone = -1;

	/// Set SC IP by default
	ptr = gethostbyname(SC_IP);
	sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
 	/// Get the arguments used
    while ((opt = getopt(argc, argv, "n:j:u:t:i:p:")) != -1) {
        switch (opt) {
            case 'n': /// server id
                my_id = atoi(optarg);
                break;
            case 'j': /// server ip
                ptr = gethostbyname(optarg);
                sprintf(my_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
                break;
            case 'u': /// port for clients
                if(isdigit(*optarg)) cli_port = atoi(optarg);
                else spawn_error("-u argument must be an integer");
                break;
            case 't': /// port for previous server
                if(isdigit(*optarg))
                    prev_port = atoi(optarg);
                else 
                    spawn_error("-u argument must be an integer");
                break;
            case 'i': /// SC IP [optional]
                ptr = gethostbyname(optarg);
                sprintf(sc_ip, "%s", inet_ntoa(*(struct in_addr*)ptr->h_addr_list[0]));
                break;
            case 'p': /// SC port [optional]
                if(isdigit(*optarg)) sc_port = atoi(optarg);
                else spawn_error("-u argument must be an integer");
                break;
            default: /// '?' 
                show_usage(argv[0]);
        }
    }

    /// Verify if usage rules are met
    if(my_id == -1) show_usage(argv[0]);
    if(my_ip == NULL) show_usage(argv[0]);
    if(cli_port == -1) show_usage(argv[0]);
    if(prev_port == -1) show_usage(argv[0]);
    if(strcmp(my_ip, LOCALHOST) == 0) {getmyip(my_ip);}

    /// Define connections
    sc_addr = define_AF_INET_conn(&sc_sock, SOCK_DGRAM, sc_port, sc_ip);
	cli_addr = define_AF_INET_conn(&cli_sock, SOCK_DGRAM, cli_port, NULL);
    prev_addr = define_AF_INET_conn(&prev_sock, SOCK_STREAM, prev_port, NULL);
    if(bind(cli_sock, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) == -1) spawn_error("Cannot bind Client socket");
    if(bind(prev_sock, (struct sockaddr*)&prev_addr, sizeof(prev_addr)) == -1) spawn_error("Cannot bind Prev socket");
    if(listen(prev_sock, 5) == -1) spawn_error("Cannot listen()");


    while(1) {

    	/// Define what must be read by select
    	FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;
        if(state == joined || state == accepted){
            FD_SET(sc_sock, &rfds);
            //FD_SET(prev_sock, &rfds);
            //FD_SET(cli_sock, &rfds);
            maxfd = max(0, sc_sock);
            //maxfd = max(maxfd, prev_sock);
            //maxfd = max(maxfd, cli_sock);
        }
        counter = select(maxfd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval*)NULL);
        if(counter <= 0) spawn_error("select() failed");

        /// Clean message
        memset(msg, 0, strlen(msg));

        /////////////////////////////////////////////////
        /// Terminal Interface
        /////////////////////////////////////////////////
        if(FD_ISSET(0, &rfds)){
            if(fgets(msg, 64, stdin) == NULL) spawn_error("Cannot read from terminal");
        	
        	/// Handle $join
            if(strstr(msg, "join") != NULL) {
                sscanf(msg, "%*[^\' '] %d", &service);
                send_msg(GET_START, sc_sock, sc_addr);
                last_msg = GET_START;
                next_msg = SET_START;
                state = joined;
            }
            /// Handle $show_state
            else if(strstr(msg, "show_state") != NULL) {

            }
            /// Handle $leave
            else if(strstr(msg, "leave") != NULL) {
            	
            }
            /// Handle $exit
            else if(strstr(msg, "exit") != NULL) {
                close(sc_sock); 
                close(next_sock);
                //close(new_prev_sock);
                close(prev_sock);
                close(cli_sock);
                exit(EXIT_SUCCESS);
            }
        }

        /////////////////////////////////////////////////
        /// SC Interface
        /////////////////////////////////////////////////
        if(FD_ISSET(sc_sock, &rfds)) {
  			if(recvfrom(sc_sock, msg, sizeof(msg), 0, (struct sockaddr*)&sc_addr, &addrlen) == -1) spawn_error("Cannot receive from SC");
            fprintf(stderr, KYEL"RECV\t"RESET"%s\n", msg);

            if(strstr(msg, "OK") != NULL) {
            	sscanf(msg, "%*[^\' '] %*[^\';'];%d[^\';'];%[^\';'];%d", &stup_id, stup_ip, &stup_port);
            	
            	if(next_msg == SET_START) {

            		if(stup_id == 0) {
            			im_alone = 1;
            			send_msg(SET_START, sc_sock, sc_addr);
            			last_msg = next_msg;
            			next_msg = SET_DS;
            		}
            	}

            }
        }
    }

	exit(EXIT_SUCCESS);
}