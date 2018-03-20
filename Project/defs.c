#include "defs.h"

void spawn_error(char *error) {
    fprintf(stderr, "Error: %s. %s\n", strerror(errno), error);
    exit(EXIT_FAILURE);
}

void show_usage(char *exe_name) {
    fprintf(stderr, "Usage: %s –n id –j ip -u upt –t tpt [-i csip] [-p cspt]\n", exe_name);
    exit(EXIT_FAILURE);
}

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

