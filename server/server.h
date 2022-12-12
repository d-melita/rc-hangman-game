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
#include <time.h>

// DEFINES

#define DEFAULT_PORT "58090" // must be 58000 + Group Number 
#define GN 90 // group number
#define MAX_ERRORS 8 // Maximum number of errors allowed

#define MAX_GAMES 256 // Maximum number of games/matches at once
#define WORDFILE_SIZE 5 // Conservative number of files in word file

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


// ERRORS

#define ERR_MISSING_ARGUMENT "ERROR: '%s' option requires an argument\n"
#define ERR_INVALID_OPTION "ERROR: Invalid option\n"


// prototypes
struct game_data;
struct game_id;
struct guessed_word;

void parse_args(int argc, char *argv[]);
void message_udp();
char* parse_message_udp(char *message);
char* start_new_game(char *message);
char* guess_letter(char *message);
char* guess_word(char *message);
char* quit(char *message);

char* play_letter(char *message);

struct game_id* get_game(char* plid);
int set_game_word(struct game_data* game_data);
int letter_in_word(char* word, char* letter);
void delete_game(struct game_id* game_id);
int word_played(char* word, struct guessed_word* guessed_words);


#endif