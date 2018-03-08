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

#define SC_PORT 59000
#define SC_DOMAIN "tejo.tecnico.ulisboa.pt"

void show_usage(char *exe_name) {
    fprintf(stderr, "Usage: %s –n id –j ip -u upt –t tpt [-i csip] [-p cspt]\n", exe_name);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    int opt;
    int id = -1;
    int my_port = -1, client_port = -1, bro_port = -1, sc_port = -1;
    struct hostent *scptr = NULL, *myptr = NULL;

    while ((opt = getopt(argc, argv, "n:j:u:t:i:p:")) != -1) {
        switch (opt) {
            case 'n':
                id = atoi(optarg);
                break;
            case 'j':
                myptr = gethostbyname(optarg);
                break;
            case 'u':
                client_port = atoi(optarg);
                break;
            case 't':
                bro_port = atoi(optarg);
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

    if(id == -1) show_usage(argv[0]);
    if(myptr == NULL) show_usage(argv[0]);
    if(client_port == -1) show_usage(argv[0]);
    if(bro_port == -1) show_usage(argv[0]);
    
    if(scptr == NULL) {
        scptr = gethostbyname(SC_DOMAIN);
        if(scptr ==NULL) 
            exit(EXIT_FAILURE);
    }
    if(sc_port == -1) {
        sc_port = SC_PORT;
    }

    printf("%d %d %d [%d]\n", id, client_port, bro_port, sc_port);

}