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
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host);

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
  if (argc != 3) {
    printf("ERROR\tnot implemented.\n");
    return 0;
  }
  int port = 53; // default
  char *ip_address = ++argv[1];
  char *colon = strchr(ip_address, ':');
  if(colon){
    *colon = '\0';
    port = atoi(++colon);
  }


  // construct the DNS request
  unsigned char buf[65536],*qname; //Where we store our packet and name
  DNS_HEADER *dns = NULL; //Where we store our header
  QUESTION *qinfo = NULL; //Where we store our question type
  unsigned char res[65536]; //Where we store the result

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
  int packet_len = sizeof(DNS_HEADER) + strlen(qname) + 1 + sizeof(QUESTION);
  dump_packet(buf, packet_len);
  
  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // next, construct the destination address
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(port); //Port
  out.sin_addr.s_addr = inet_addr(ip_address);  //DNS Server

  if (sendto(sock, buf, packet_len, 0, &out, sizeof(out)) < 0) { //Points to packet and packet len
    // an error occurred
    printf("ERROR\tdid not send packet.\n");
    return 0;
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
      printf("ERROR\tno data received.\n");
      return 0;
    }
  } else {
    // a timeout occurred
    printf("NORESPONSE\n");
    return 0;
  }

  // put results into structures
  memcpy(dns, res, sizeof(DNS_HEADER));
  dns->id = ntohs(dns->id);
  dns->q_count = ntohs(dns->q_count);
  dns->ans_count = ntohs(dns->ans_count);
  dns->auth_count = ntohs(dns->auth_count);
  dns->add_count = ntohs(dns->add_count);

  if(dns->rcode == 3) {
    printf("NOTFOUND\n");
    return 0;
  }

  int index = 12;
  QUERY queries[dns->q_count];
  for(int i = 0; i < dns->q_count; i++){
    char *qname = malloc(255);
    int qindex = 0;
    char count = 0;
    while(res[index] != '\0'){
      if(count == 0){
        count = res[index];
        qname[qindex] = '.';
      }else {
        qname[qindex] = res[index];
        count --;
      }
      qindex ++;
      index ++;
    }qname[qindex] = res[index];
    // malloc the non-temp string to length
    queries[i].name = (char *) malloc(qindex + 1);
    strncpy(queries[i].name, qname, qindex);
    free(qname);
    // get rest of query information
    QUESTION q;
    q.qtype = (res[index+1] << 8) + res[index+2];
    q.qclass = (res[index+3] << 8) + res[index +4];
    queries[i].ques = &q;
    index += 5;
  }

  RES_RECORD answers[dns->ans_count];
  for(int i = 0; i < dns->ans_count; i++){
    unsigned char *tmp_name = (unsigned char *) malloc(255);
    int nindex = 0;
    unsigned char count = 0;
    int index_restore = 0;
    while(res[index] != '\0'){
      // find label length if count == 0
      if(count == 0){
        // check for a pointer
        if(res[index] & 0x10000000){
          unsigned short ptr = (res[index] << 8) + res[index+1];
          ptr &= 0x0011111111111111;
          if (!index_restore) index_restore = index + 1;
          index = ptr;
          continue;
        }
        count = res[index];
        tmp_name[nindex] = ".";
      } else {
        tmp_name[nindex] = res[index];
        count --;
      }
      nindex ++;
      index ++;
    }
    tmp_name[nindex] = res[index]; // null terminator
    // malloc the name string, free the tmp string, restore index
    answers[i].name = malloc(nindex + 1);
    strncpy(answers[i].name, tmp_name, nindex);
    free(tmp_name);
    if(index_restore) index = index_restore;
    R_DATA *rd = (R_DATA *) malloc(sizeof(R_DATA));
    rd->type = (res[index] << 8) + res[index + 1];
    rd->_class = (res[index + 2] << 8) + res[index + 3];
    rd->ttl = (res[index +4] << 24) + (res[index + 5] << 16) + (res[index + 6] << 8) + res[index + 7];
    rd->data_len = (res[index + 8] << 8) + res[index + 9];
    answers[i].resource = rd;
    index += 10;

    // parse rdata
    if (rd->type == 1){
     unsigned char rdata[4];
     for(int j = 0; j < 4; j++){
       rdata[j] = res[index + j];
     }
     answers[i].rdata = rdata;
     index += 4;
   }
   if (rd->type == 5) {
     tmp_name = malloc(255);
     nindex = 0;
     count = 0;
     index_restore = 0;
      while(res[index] != '\0'){
        // find label length if count == 0
        if(count == 0){
          // check for a pointer
          if(res[index] & 0xc0){
            unsigned short ptr = (res[index] << 8) + res[index+1];
            ptr &= 0x3fff;
            if(!index_restore) index_restore = index + 1;
            index = ptr;
            continue;
          }
          count = res[index];
          tmp_name[nindex] = '.';
        } else {
          tmp_name[nindex] = res[index];
          count --;
        }
        nindex ++;
        index ++;
      }
      tmp_name[nindex] = res[index]; // null terminator
      tmp_name += 1;
    // malloc the name string, free the tmp string, restore index
    answers[i].rdata = malloc(nindex);
    strncpy(answers[i].rdata, tmp_name, nindex - 1);
    free(--tmp_name);
    if(index_restore) index = index_restore;
   }
  }

  // ignore rest of packet
  
  // print out answers
  for(int i = 0; i < dns->ans_count; i++){
    if (answers[i].resource->type == 1){
      printf("IP\t%d.%d.%d.%d\t", answers[i].rdata[0], answers[i].rdata[1], answers[i].rdata[2], answers[i].rdata[3]);
    }
    if (answers[i].resource->type == 5){
      printf("CNAME\t%s\t", answers[i].rdata);
    }
    if(dns->aa) printf("auth\n");
    else printf("nonauth\n");
  }

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
