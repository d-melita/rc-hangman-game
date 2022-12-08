#ifndef _SERVER_H
#define _SERVER_H

// INCLUDES

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

// DEFINES

#define PORT "58090" // default port 58000 + GN (90)

#define SNG "SNG"
#define RSG "RSG"
#define QUT "QUT"
#define PLG "PLG"
#define RLG "RLG"
#define PWG "PWG"
#define RWG "RWG"
#define RQT "RQT"
#define REV "REV"
#define RVV "RVV"
#define GSB "GSB\n"
#define RSB "RSB"
#define GHL "GHL"
#define RHL "RHL"
#define STA "STA"
#define RST "RST"

#define OK "OK"
#define NOK "NOK"
#define ERR "ERR"
#define DUP "DUP"
#define WIN "WIN"
#define OVR "OVR"
#define INV "INV"
#define ACT "ACT"
#define FIN "FIN"
#define EMPTY "EMPTY"

// prototypes
void parse_args(int argc, char *argv[]);
void message_udp();
void parse_message_udp(char *message);
void start_new_game(char *message);
void guess_letter(char *message);
void guess_word(char *message);
void quit(char *message);

#endif