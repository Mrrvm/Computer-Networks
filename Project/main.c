#include "sockets.h"

/// Ports
int cli_port = 0, sc_port = SC_PORT, next_port = 0, 
	prev_port = 0, prev_server_tport = 0;
/// IPs
char my_ip[64] = {0}, sc_ip[64] = {0}, prev_ip[64] = {0}, next_ip[64] = {0};
/// IDs
int my_id = -1, prev_id = -1, next_id = -1;
/// Service number
int service = 0;

int main(int argc, char *argv[]) {
	
	int opt = 0, maxfd = 0, counter = 0, dummy = 0;
	int last_msg = NONE;
	struct hostent *ptr = NULL;
	fd_set rfds;
	char msg[64] = {0};
	char *dummy_s = NULL;
	unsigned int addrlen = 0;
	/// Sockets
	int sc_sock = 0, cli_sock = 0, next_sock = 0, 
		prev_sock = 0, new_prev_sock = 0;
	/// sockaddr
	struct sockaddr_in sc_addr, next_addr, prev_addr, cli_addr;
	/// State
	enum {off, joined, accepted} state = off;
	/// Conditions
	int im_alone = 1, is_ring_av = 1, im_ds = 0, im_stup = 0, im_av = 1;

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
    if(bind(prev_sock, (struct sockaddr*)&prev_addr, sizeof(prev_addr)) == -1) spawn_error("Cannot bind Previous socket");
    if(listen(prev_sock, 5) == -1) spawn_error("Cannot listen()");


    while(1) {

    	/// Define select conditions
    	FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;
        if(state == joined || state == accepted){
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
                state = joined;
            }
            /// Handle $show_state
            else if(strstr(msg, "show_state") != NULL) {
            	fprintf(stderr, 
            		KMAG"CURRENT STATE\n\tAVAILABLE"RESET" %d\n"KMAG"\tRING AVAILABLE"RESET" %d\n"KMAG"\tNEXT ID"RESET" %d\n"RESET, 
            		im_av, is_ring_av, next_id);
            }
            /// Handle $leave
            else if(strstr(msg, "leave") != NULL) {
            	
            }
            /// Handle $exit
            else if(strstr(msg, "exit") != NULL) {
                close(sc_sock); 
                close(next_sock);
                close(new_prev_sock);
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
            	sscanf(msg, "%*[^\' '] %d;", &dummy);
            	if(dummy != my_id) spawn_error("Received bad OK message");

            	if(last_msg == GET_START) {
            		sscanf(msg, "%*[^\' '] %*[^\';'];%d;%[^\';'];%d", &next_id, next_ip, &next_port);
            		/// If no servers exist yet
            		if(next_id == 0) {
            			im_alone = 1;
            			send_msg(SET_START, sc_sock, sc_addr);
            			last_msg = SET_START;
            		}
            		/// If startup server already exists
            		else {
            			im_alone = 0;
            			next_addr = define_AF_INET_conn(&next_sock, SOCK_STREAM, next_port, next_ip);
            			if(connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr)) == -1) spawn_error("Cannot connect to next server");
            			send_msg(NEW, next_sock, next_addr);
            			last_msg = NONE;
            		}
            	}
            	else if(last_msg == SET_START) {
            		/// Confirms previously asked startup status
					im_stup = 1;
					/// Set myself as dispatch
            		send_msg(SET_DS, sc_sock, sc_addr);
            		last_msg = SET_DS;
            	}
            	else if(last_msg == SET_DS) {
            		/// Confirms previously asked dispatch status
            		im_ds = 1;
            		last_msg = NONE;
            	}
            	else if(last_msg == WITHDRAW_DS) {
            		/// After confirming off duty status
            		/// Inform other servers (Token S)
            		if(!im_alone) send_msg(TOKEN_S, next_sock, next_addr);
            		else is_ring_av = 0;
            		/// Inform client the service is ON
            		send_msg(YOUR_SERVICE_ON, cli_sock, cli_addr);
            		/// Update my conditions
            		im_av = 0; im_ds = 0;
            		last_msg = NONE;
            	}
            }
        }

        /////////////////////////////////////////////////
        /// Previous Server Acceptor
        /////////////////////////////////////////////////
        if(FD_ISSET(prev_sock, &rfds)) {
            new_prev_sock = accept(prev_sock, (struct sockaddr*)&prev_addr, &addrlen);
            if(new_prev_sock == -1) spawn_error("Cannot accept previous server");
            fprintf(stderr, KGRN"NEW CONNECTION\n"RESET);
            state = accepted;
        }
        /////////////////////////////////////////////////
        /// Previous Server Interface
        /////////////////////////////////////////////////
        if(state == accepted && FD_ISSET(new_prev_sock, &rfds)) {
        	dummy = read(new_prev_sock, msg, sizeof(msg));
        	if(dummy == -1) spawn_error("Cannot read from previous server");
            else if(dummy == 0) {close(new_prev_sock); state = joined;}
            else{
            	fprintf(stderr, KGRN"RECV\t"RESET"%s", msg);
           
            	/// Handle NEW
            	if(strstr(msg, "NEW") != NULL) {
            		sscanf(msg, "%*[^\' '] %d;%[^\';'];%d", &prev_id, prev_ip, &prev_server_tport);
            		/// When only 2 servers
            		if(im_alone) {
            			/// Define previous server as next 
            			next_id = prev_id;
            			strncpy(next_ip,  prev_ip, sizeof(prev_ip));
            			next_port = prev_server_tport;
            			prev_port = prev_server_tport;
            			next_addr = define_AF_INET_conn(&next_sock, SOCK_STREAM, next_port, next_ip);
            			if(connect(next_sock,(struct sockaddr*)&next_addr, sizeof(next_addr)) == -1) spawn_error("Cannot connect to next server");
            			im_alone = 0;

            			/// NOTE: Handle case where ring is unavailable and a new server enters
            		}
            		else send_msg(TOKEN_N, next_sock, next_addr);
            	}
            	/// Handle TOKEN
            	else if(strstr(msg, "TOKEN") != NULL) {
            		dummy_s = strstr(msg, ";");
            		/// TOKEN N
            		if(strstr(dummy_s, "N") != NULL) {
            			/// Get the id of who sent the token
            			sscanf(msg, "%*[^\' '] %d;", &dummy);
            			/// If the one who sent was the next, connect to the new server
            			if(dummy == next_id) {
        					memset(next_ip, 0, strlen(next_ip));
            				sscanf(msg, "%*[^\' '] %*[^\';'];%*[^\';'];%d;%[^\';'];%d", &next_id, next_ip, &next_port);
            				next_addr = define_AF_INET_conn(&next_sock, SOCK_STREAM, next_port, next_ip);
            				if(connect(next_sock, (struct sockaddr*)&next_addr, sizeof(next_addr)) == -1) spawn_error("Cannot connect to next server");
            			}
            			else {
            				/// Resend token
                            if(write(next_sock, msg, strlen(msg)) == -1) spawn_error("Cannot write to next server");
            				fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
            			}
            		}
            		/// TOKEN S
            		else if(strstr(dummy_s, "S") != NULL) {
            			/// Get the id of who sent the token
            			sscanf(msg, "%*[^\' '] %d;", &dummy);
            			/// If I'm the one who sent
            			if(my_id == dummy) {
            				/// Send token I
            				send_msg(TOKEN_I, next_sock, next_addr);
            			}
            			else {
            				if(im_av) {
            					send_msg(SET_DS, sc_sock, sc_addr);
            					send_msg(TOKEN_T, next_sock, next_addr);
            					last_msg = SET_DS;
            				}
            				else {
            					/// Resend token
                            	if(write(next_sock, msg, strlen(msg)) == -1) spawn_error("Cannot write to next server");
            					fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
            				}
            			}
            		}
            		/// TOKEN T
            		else if(strstr(dummy_s, "T") != NULL) {
			            sscanf(msg, "%*[^\' '] %d;", &dummy);
			            if(my_id != dummy) {
			                /// Resend token
                        	if(write(next_sock, msg, strlen(msg)) == -1) spawn_error("Cannot write to next server");
        					fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
			            }
			        }
            		/// TOKEN I
            		else if(strstr(dummy_s, "I") != NULL) {
            			/// This condition garantees there are no sudden availability changes
            			if(!im_av) {
            				/// Update ring status
            				is_ring_av = 0;
			            	sscanf(msg, "%*[^\' '] %d;", &dummy);
				            if(my_id != dummy) {
				                /// Resend token
	                        	if(write(next_sock, msg, strlen(msg)) == -1) spawn_error("Cannot write to next server");
	        					fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
				            }
            			}
			        }
			        /// TOKEN D
			        else if(strstr(dummy_s, "D") != NULL) {
			        	if(im_av) {
			        		sscanf(msg, "%*[^\' '] %d;", &dummy);
			        		if(my_id > dummy) {
			        			/// Resend token
	                        	if(write(next_sock, msg, strlen(msg)) == -1) spawn_error("Cannot write to next server");
	        					fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
			        		}
			        		else if(my_id == dummy){
			        			/// Inform SC server I am dispatch now
            					send_msg(SET_DS, sc_sock, sc_addr);
			        		}
			        	}
			        	else {
			        		/// Resend token
                        	if(write(next_sock, msg, strlen(msg)) == -1) spawn_error("Cannot write to next server");
        					fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
			        	}
			        	is_ring_av = 1;
			        }
			        /// TOKEN O
			        else if(strstr(dummy_s, "O") != NULL) {

			        }
            	}
            }

        }

        /////////////////////////////////////////////////
        /// Client Interface
        /////////////////////////////////////////////////
        if(FD_ISSET(cli_sock, &rfds)) {
        	if(recvfrom(cli_sock, msg, sizeof(msg), 0, (struct sockaddr*)&cli_addr, &addrlen)==-1) spawn_error("Error receiving from Client");
            fprintf(stderr, KCYN"RECV\t"RESET"%s\n", msg);

            if(strstr(msg, "MY_SERVICE") != NULL) {	
            	if(strstr(msg, "ON") != NULL) {
            		/// Inform SC I am not dispatch anymore
            		send_msg(WITHDRAW_DS, sc_sock, sc_addr);
            		last_msg = WITHDRAW_DS;
            	}
            	else if(strstr(msg, "OFF") != NULL) {
            		send_msg(YOUR_SERVICE_OFF, cli_sock, cli_addr);
            		im_av = 1;
            		if(!is_ring_av) {
            			/// Send token D
            			send_msg(TOKEN_D, next_sock, next_addr);
            		}
            	}
            }
        }
    }

	exit(EXIT_SUCCESS);
}