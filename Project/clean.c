#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 59000
#ifndef MY_RCI_GROUP
#define MY_RCI_GROUP 26
#endif

int main(void)
{
    int fd;
    unsigned int addrlen;
    struct hostent *hostptr = NULL;
    struct sockaddr_in serveraddr;
    char id[8] = {0}, buffer[80] = {0}, service[8] = {0}, id_stup[8] = {0};

    printf("Service: (press Enter to ignore) ");
    fgets(service, 8, stdin);
    if(strcmp(service, "\n") == 0)
    {
        sprintf(service, "%d", MY_RCI_GROUP);
        sprintf(id, "%s", "1");
    }
    else
    {
        printf("ID: ");
        fgets(id, 8, stdin);
    }

    // create socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1)
    {
        exit(0);
    }

    // get host
    hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");
    if(hostptr == NULL)
    {
        exit(0);
    }

    // configure UDP connection
    if(memset((void *)&serveraddr, (int)'\0', sizeof(serveraddr)) == NULL)
    {
        serveraddr.sin_family = AF_INET;
    }
    serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
    serveraddr.sin_port = htons((u_short)PORT);
    addrlen = sizeof(serveraddr);
    memset(buffer, 0, strlen(buffer));
    sprintf(buffer, "%s %s;%s", "WITHDRAW_START", service, id);

    // Exterminate
    if(sendto(fd, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&serveraddr, addrlen) != -1)
    {

        memset(buffer, 0, strlen(buffer));
        sprintf(buffer, "%s %s;%s", "WITHDRAW_DS", service, id);
        if(sendto(fd, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&serveraddr, addrlen) != -1)
        {

            memset(buffer, 0, strlen(buffer));
            sprintf(buffer, "%s %s;%s", "GET_START", service, id);
            if(sendto(fd, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&serveraddr, addrlen) != -1)
            {

                memset(buffer, 0, strlen(buffer));

                // Verify extermination
                if(recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serveraddr, &addrlen) != -1)
                {

                    sscanf(buffer, "%*[^\' '] %*[^\';'];%[^\';'];", id_stup);
                    if(strcmp(id_stup, "0") == 0)
                    {
                        printf("SUCCESS: SC was wiped out\n");
                        close(fd);
                        exit(0);
                    }
                }
            }
        }

    }

    close(fd);
    printf("WARNING: cleaning operation failed\n");
    exit(0);
}
