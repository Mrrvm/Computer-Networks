#include "defs.h"

/// Ports
extern int cli_port, sc_port, next_port, prev_port, prev_server_tport;
/// IPs
extern char my_ip[64], sc_ip[64], prev_ip[64], next_ip[64];
/// IDs
extern int my_id, prev_id, next_id;
/// Service number
extern int service;

struct sockaddr_in define_AF_INET_conn(int *sock, int type, int port, char *ip);
void send_msg(int type, int sock, struct sockaddr_in addr);