#include <unistd.h>
#include <linux/if_link.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include <ifaddrs.h>
#include <ctype.h>
#include <net/if.h>
#include <errno.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

#define max(x, y) (((x) > (y)) ? (x) : (y))

#define SC_PORT 59000
#define SC_IP "tejo.tecnico.ulisboa.pt"
#define LOCALHOST "127.0.0.1"
#define TIMEUS 0
#define TIMES 3

#define NONE 0
#define GET_START 1
#define SET_START 2
#define SET_DS 3
#define NEW 4
#define TOKEN_N 5
#define TOKEN_S 6
#define TOKEN_D 7
#define TOKEN_I 8 
#define TOKEN_O 9
#define TOKEN_T 10
#define NEW_START 11
#define WITHDRAW_START 12
#define WITHDRAW_DS 13
#define YOUR_SERVICE_ON 14
#define YOUR_SERVICE_OFF 15

#define GET_DS_SERVER 16
#define MY_SERVICE_ON 17
#define MY_SERVICE_OFF 18

#define EXIT 1
#define LEAVE 2

void spawn_error(char *error);
void show_usage(char *exe_name);
void getmyip(char my_ip[]);
