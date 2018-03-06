#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 59000

int main(int argc, char *argv[])
{
    int opt;
    int port = -1;
    char *ip = NULL;
    struct hostent *hostptr;

    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
        switch (opt) {
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i csip] [-p cspt]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(ip == NULL) {
        if(argc > 1) {
            fprintf(stderr, "Usage: %s [-i csip] [-p cspt]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");
        if(hostptr==NULL){exit(EXIT_FAILURE);}
        printf("%s\n", inet_ntoa((hostptr->h_addr_list[0])));
        //strcpy(ip);
    }

    if(port == -1) {
        if(argc > 1) {
            fprintf(stderr, "Usage: %s [-i csip] [-p cspt]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        port = DEFAULT_PORT;
    }

    fprintf(stderr, "Arguments %s %d\n",ip, port );
    exit(EXIT_SUCCESS);
}
