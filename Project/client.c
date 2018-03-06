#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  int opt;
  int port = -1;
  char *ip = NULL;

  while ((opt = getopt(argc, argv, "i:p:")) != -1) {
      switch (opt) {
        case 'i':
            ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-i csip] [-p csp]\n", argv[0]);
            exit(EXIT_FAILURE);
      }
  }

  if(ip == NULL || port == -1) {
      fprintf(stderr, "Usage: %s [-i csip] [-p csp]\n", argv[0]);
      exit(EXIT_FAILURE);
  }
  
  if (argc != 5) {
      fprintf(stderr, "Usage: %s [-i csip] [-p csp]\n", argv[0]);
      exit(EXIT_FAILURE);
  }

   fprintf(stderr, "Arguments %s %d\n",ip, port );
   exit(EXIT_SUCCESS);
}
