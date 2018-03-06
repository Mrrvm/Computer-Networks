#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define N_ARGS 4

int main(int argc, char *argv[])
{
   int opt;
   int ip = 0, port = 0;

   while ((opt = getopt(argc, argv, "i:p:")) != -1) {
       switch (opt) {
	       case 'i':
	           ip = atoi(optarg);
	           break;
	       case 'p':
	           port = atoi(optarg);
	           break;
	       default: /* '?' */
	           fprintf(stderr, "Usage: %s [-i csip] [-p csp]\n", argv[0]);
	           exit(EXIT_FAILURE);
       }
   }

    if (optind != N_ARGS) {
       fprintf(stderr, "Usage: %s [-i csip] [-p csp]\n", argv[0]);
       exit(EXIT_FAILURE);
   	}

   fprintf(stderr, "Arguments %d %d\n",ip, port );
   exit(EXIT_SUCCESS);
}
