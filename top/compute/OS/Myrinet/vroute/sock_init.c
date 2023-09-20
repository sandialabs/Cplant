/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
#include <stdio.h>
#include <string.h>

#include <netdb.h>                  /* gethostbyname() */
#include <netinet/in.h>             /* sockaddr_in     */
#include <sys/socket.h>             /* AF_INET, SOCK_STREAM */

int sock_init(char* hostname, int try_port, int verbosity) 
{
  int sd;
  struct hostent* hostp;
  struct sockaddr_in name;
  struct servent* serv_p;

  if (verbosity) {
   printf("vroute: trying gethostbyname(%s)...\n", hostname);
  }
  if ( (hostp = gethostbyname(hostname)) == NULL ) {
    printf("vroute: gethostbyname() failed for %s\n", hostname);
    perror("vroute");
    return(-1);
  }

  if (verbosity) {
   printf("vroute: trying socket()...\n");
  }
  if ( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    printf("vroute: socket() failed for %s\n", hostname);
    perror("vroute"); 
    return(-1);
  }

  name.sin_family = AF_INET;

  if (try_port != 0) { /* user guess overrides /etc/services */
    name.sin_port = htons(try_port);
    if (verbosity) {
      printf("vroute: trying port from user: %d\n", ntohs(name.sin_port)); 
    }
  }
  else {
    if (verbosity) {
     printf("vroute: trying getservbyname()...\n");
    }
    if ( (serv_p = getservbyname("vroute","tcp")) == NULL ) { 
                                                 /* from /etc/services */
      printf("vroute: sock_init(%s): getservbyname() failed.\n", hostname);
//    perror("vroute"); 
      printf("vroute: could not get service port from /etc/services on local host\n");
      printf("vroute: you can try to fix the problem or if you know the port for\n");
      printf("vroute: vroute service you can enter it on the command line using\n");
      printf("vroute: the -p option. the port for vroute service might be 8011 :)--.\n");
      return(-1);
    }
    else {
      name.sin_port = serv_p->s_port;
      if (verbosity) {
        printf("vroute: port from getservbyname(): %d\n", ntohs(name.sin_port)); 
      }
    }
  }

//memcpy( &name.sin_addr.s_addr, hostp->h_addr, hostp->h_length );
  bcopy( hostp->h_addr, &(name.sin_addr.s_addr), hostp->h_length );

  if (verbosity) {
   printf("vroute: trying connect()...\n");
  }
  if ( connect( sd, (struct sockaddr *) &name, sizeof(name) ) < 0 ) {
    printf("vroute: sock_init(%s): connect() failed.\n", hostname);
    perror("vroute"); 
    return(-1);
  }

  return(sd);
}
