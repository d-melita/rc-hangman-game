#ifndef _CLIENT_H
#define _CLIENT_H


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


// DEFINES

#define DEFAULT_SERVER "tejo.ist.utl.pt"
#define DEFAULT_PORT "58011" // must be 58000 + Group Number but it isnt defined yet so using default for now
#define GN "GN" // group number

#define START "start"
#define SG "sg"
#define PLAY "play"
#define PL "pl"
#define GUESS "guess"
#define GW "gw"
#define SCOREBOARD "scoreboard"
#define SB "sb"
#define HINT "hint"
#define H "h"
#define STATE "state"
#define ST "st"
#define QUIT "quit"
#define EXIT "exit"

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
#define GSB "GSB"
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


// Buffer sizes

#define COUNT_CMD 8
#define MAX_SIZE_CMD 10
#define MAX_SIZE_ALTCMD 3

#define SIZE_IP 15 // 3*. + 12*0-9
#define SIZE_PORT 5
#define SIZE_PLID 6
#define MAX_SIZE_HOST 256


// Error messages (Consider adding '%s')

#define ERR_INVALID_PLID "ERROR: Invalid player ID\n"
#define ERR_INVALID_HOST "ERROR: Invalid hostname\n"
#define ERR_INVALID_CMD "ERROR: Invalid command\n"
#define ERR_GETADDRINFO "ERROR: getaddrinfo: %s\n"
#define ERR_MISSING_ARGUMENT "ERROR: '%s' option requires an argument\n"
#define ERR_INVALID_OPTION "ERROR: Invalid option\n"
#define ERR_ONGOING_GAME "ERROR: There is an ongoing game\n"
#define ERR_NO_MATCHES_CHAR "ERROR: The given character does not appear in current word\n"
#define ERR_SOCKET "ERROR: Socket error\n"
#define ERR_SENDTO "ERROR: Sendto error\n"
#define ERR_RECVFROM "ERROR: Recvfrom error\n"


// Function prototypes

void message_udp(char *buffer);
void set_new_game(char *message);
void play_made(char *message);
void get_ip();
void get_ip_known_host(char *host);
void parse_args(int argc, char *argv[]);
void parse_response_udp(char *message);
void parse_response_tcp(int fd, char *message);
void set_new_game(char *message);
void parse_message_play(char *message, char pos[]);
void play_made(char *message);
void start_function();
void play_function();
void guess_function();
void scoreboard_function();
void quit_function();
void message_udp(char *buffer);
void message_tcp(char *buffer);
void win_function();
void win_word_function();
void scoreboard(char *message);
void get_hint(int fd, char *message);
void game_status(int fd, char *message);
void hint_function();
void state_function();



#endif