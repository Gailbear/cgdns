/*
 *   CS3600 Project 3: A DNS Client
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600dns.h"

/**
 * This function will print a hex dump of the provided packet to the screen
 * to help facilitate debugging.  In your milestone and final submission, you 
 * MUST call dump_packet() with your packet right before calling sendto().  
 * You're welcome to use it at other times to help debug, but please comment those
 * out in your submissions.
 *
 * DO NOT MODIFY THIS FUNCTION
 *
 * data - The pointer to your packet buffer
 * size - The length of your packet
 */
static void dump_packet(unsigned char *data, int size) {
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

int main(int argc, char *argv[]) {
  /**
   * I've included some basic code for opening a socket in C, sending
   * a UDP packet, and then receiving a response (or timeout).  You'll 
   * need to fill in many of the details, but this should be enough to
   * get you started.
   */

  // process the arguments

  // construct the DNS request
  unsigned char buf[65536],*qname; //Where we store our packet and name
  DNS_HEADER *dns = NULL; //Where we store our header
  QUESTION *qinfo = NULL; //Where we store our question type
  unsigned char res[65536];

  //Set the DNS structure to standard queries
  dns=(DNS_HEADER*)&buf;

  //set up the header
  dns->id = (unsigned short)htons(1337); //Query ID
  dns->qr = 0; //This is a query
  dns->opcode = 0; //This is a standard query
  dns->aa = 0; //Not Authoritative
  dns->tc = 0; //This message is not truncated
  dns->rd = 1; //Recursion Desired
  dns->ra = 0; //Recursion not available! hey we dont have it
  dns->z = 0; //Reserved for later use
  dns->ad = 0; //Not authenticated data
  dns->cd = 0; //checking disabled
  dns->rcode = 0; //response code
  dns->q_count = htons(1); //we have only 1 question
  dns->ans_count = 0; //number of answer entries
  dns->auth_count = 0; //number of authority entries
  dns->add_count = 0; //number of resource entries

  //plug in the name we want to get an IP for
  qname =(unsigned char*)&buf[sizeof(DNS_HEADER)];
  ChangetoDnsNameFormat(qname, argv[2]);
//  strncpy(qname, argv[2], strlen(argv[2]+1));

  qinfo = (QUESTION*)&buf[sizeof(DNS_HEADER) + (strlen((const char*)qname) + 1)];

  //fill it
  qinfo->qtype = htons(1); //we are requesting the ipv4 address
  qinfo->qclass = htons(1); //its internet


  // send the DNS request (and call dump_packet with your request)
  dump_packet(buf, sizeof(DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(QUESTION));
  
  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // next, construct the destination address
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(53); //Port
  out.sin_addr.s_addr = inet_addr("129.10.112.152");  //DNS Server

  if (sendto(sock, buf, sizeof(buf), 0, &out, sizeof(out)) < 0) { //Points to packet and packet len
    // an error occurred
  }

  // wait for the DNS reply (timeout: 5 seconds)
  struct sockaddr_in in;
  socklen_t in_len;

  // construct the socket set
  fd_set socks;
  FD_ZERO(&socks);
  FD_SET(sock, &socks);

  // construct the timeout
  struct timeval t;
  t.tv_sec = 5; //timeout in seconds
  t.tv_usec = 0;

  // wait to receive, or for a timeout
  if (select(sock + 1, &socks, NULL, NULL, &t)) {
    if (recvfrom(sock, res, sizeof(res), 0, &in, &in_len) < 0) {
      // an error occured
    }
  } else {
    // a timeout occurred
  }

  // print out the result
  
  return 0;
}

//this will convert www.google.com to 3www6google3com
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host)
{
  int lock=0 , i;

  strcat((char*)host,".");

  for(i=0 ; i<(int)strlen((char*)host) ; i++)
  {
    if(host[i]=='.')
    {
      *dns++=i-lock;
      for(;lock<i;lock++)
      {
        *dns++=host[lock];
      }
      lock++; //or lock=i+1;
    }
  }
  *dns++='\0';
}
