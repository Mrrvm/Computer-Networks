#include "sockets.h"

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

void send_msg(int type, int sock, struct sockaddr_in addr) {

	int addrlen = sizeof(addr);
	char msg[64] = {0};

	if(type == GET_START) {
		sprintf(msg, "%s %d;%d", "GET_START", service, my_id);
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Cannot send GET_START");      
		fprintf(stderr, KYEL"SENT\t"RESET"%s\n", msg);
	}
	else if(type == SET_START) {
		sprintf(msg, "%s %d;%d;%s;%d", "SET_START", service, my_id, my_ip, prev_port);
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Cannot send SET_START");  
		fprintf(stderr, KYEL"SENT\t"RESET"%s\n", msg);
	}
	else if(type == SET_DS) {
		sprintf(msg, "%s %d;%d;%s;%d", "SET_DS", service, my_id, my_ip, cli_port);
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Cannot send SET_DS");  
		fprintf(stderr, KYEL"SENT\t"RESET"%s\n", msg);
	}
	else if(type == WITHDRAW_DS) {
		sprintf(msg, "%s %d;%d", "WITHDRAW_DS", service, my_id);
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Cannot send WITHDRAW_DS");  
		fprintf(stderr, KYEL"SENT\t"RESET"%s\n", msg);
	}
	else if(type == WITHDRAW_START) {
		sprintf(msg, "%s %d;%d", "WITHDRAW_START", service, my_id);
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Cannot send WITHDRAW_START");  
		fprintf(stderr, KYEL"SENT\t"RESET"%s\n", msg);
	}
	else if(type == NEW) {
		sprintf(msg, "NEW %d;%s;%d\n", my_id, my_ip, prev_port);
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_N");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == TOKEN_N) {
		sprintf(msg, "TOKEN %d;N;%d;%s;%d\n", my_id, prev_id, prev_ip, prev_server_tport);
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_N");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == TOKEN_S) {
		sprintf(msg, "TOKEN %d;%s\n", my_id, "S"); 
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_S");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == TOKEN_T) {
		sprintf(msg, "TOKEN %d;%s\n", S_sender_id, "T");
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_T");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == TOKEN_I) {
		sprintf(msg, "TOKEN %d;%s\n", my_id, "I");
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_I");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == TOKEN_D) {
		sprintf(msg, "TOKEN %d;%s\n", my_id, "D");
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_D");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == TOKEN_O) {
		sprintf(msg, "TOKEN %d;O;%d;%s;%d\n", my_id, next_id, next_ip, next_port);
		if(write(sock, msg, strlen(msg)) == -1) spawn_error("Cannot send TOKEN_O");      
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == NEW_START) {
		sprintf(msg, "NEW_START\n");
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Could not send NEW_START\n");
		fprintf(stderr, KGRN"SENT\t"RESET"%s", msg);
	}
	else if(type == YOUR_SERVICE_ON) {
		sprintf(msg, "YOUR_SERVICE ON");
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Could not send YOUR_SERVICE ON\n");
		fprintf(stderr, KCYN"SENT\t"RESET"%s\n", msg);
	}
	else if(type == YOUR_SERVICE_OFF) {
		sprintf(msg, "YOUR_SERVICE OFF");
		if(sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, addrlen) == -1) spawn_error("Could not send YOUR_SERVICE OFF\n");
		fprintf(stderr, KCYN"SENT\t"RESET"%s\n", msg);
	}
	else if(type == GET_DS_SERVER) {
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

int readTCP(int sock, char *msg) {

	char *a = NULL;
	char ch;
	int i= 0;

	a = &msg[0];

	while((i = read(sock, &ch, 1)) != 0) {

		if(i == -1) return -1;
		else {
			*a++ = ch;
			if(ch == '\n') {
				return 1;
			}
		}
		
	}
	return 0;
}