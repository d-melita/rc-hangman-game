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
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

// DEFINES

// GENERAL
#define DEFAULT_PORT "58090" // must be 58000 + Group Number 
#define GN 90 // group number
#define MAX_ERRORS 8 // Maximum number of errors allowed
#define SIZE_WORDFILE 10
#define DEFAULT_TABLE_SIZE 15625 // max 64 collisions, 125kb
#define MAX_TABLE_SIZE 1000000 // 1mb

// CODES
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
#define ERR_CODE "ERROR: Invalid message code\n"

// STATUS
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
#define FAIL "FAIL"

// MESSAGES
#define LETTER_TRIAL "Letter trial: "
#define WORD_GUESS "Word guess: "
#define VERBOSE "Verbose mode enabled\n"
#define UDP "New UDP message received\n"
#define SB_REQUESTED "Scoreboard requested\n"

// SCOREBOARD HEADER
#define SCOREBOARD_HEADER "   SCORE    PLID    SUCCESSFULL TRIALS    TOTAL TRIALS    WORD  \n"

// FILES 
#define SCOREBOARD "SCOREBOARD.txt"
#define TEMP "temp.txt"
#define GAMES "GAMES/GAME_%s.txt"
#define GAMES_STATUS "GAMES/STATUS_%s.txt"
#define STATE "STATE_%s.txt"

// DIRS
#define WORDS "WORDS/"

// STATUS MESSAGES
#define LAST_FIN_GAME "Last finalized game for player %s\n"
#define WORD_HINT "Word: %s Hint file: %s\n"
#define TRANSACTIONS "%d transactions found:\n"
#define ACTIVE_GAME "Active game found for player %s\n"
#define GAME_STARTED "Game started - no transactions found\n"
#define TRANSACTIONS_FOUND "--- Transactions found: %d---\n"
#define TERMINATION "TERMINATION: %s\n"
#define SOLVED_SO_FAR "Solved so far: %s\n"

// ERRORS

#define ERR_MISSING_ARGUMENT "ERROR: '%s' option requires an argument\n"
#define ERR_INVALID_OPTION "ERROR: Invalid option\n"
#define FILE_DNE "ERROR: File does not exist\n"

// SCOREBOARD HEADER
#define SCOREBOARD_HEADER "   SCORE    PLID    SUCCESSFULL TRIALS    TOTAL TRIALS    WORD  \n"


// prototypes
// structs
struct game_data;
struct game_id;
struct guessed_word;

int hash(char* plid);
int set_game(char* plid);
int resize_table();

void parse_args(int argc, char *argv[]);

// UDP functions
void message_udp();
char* parse_message_udp(char *message);
char* start_new_game(char *message);
char* play_letter(char *message);
char* guess_word(char *message);
char* quit(char *message);

// add game to scoreboard
void store_game(struct game_id* game_id);
void add_scoreboard_line(FILE* fp, struct game_id *curr_game);

// aux functions to UDP functions
int set_game_data(struct game_id* game_id);
struct game_id* get_game(char* plid);
int letter_in_word(char* word, char* letter);
void delete_game(struct game_id* game_id);
int word_played(char* word, struct guessed_word* guessed_words);

// udpdate game status
void update_game_status(struct game_id *game, char* attempt, char* status);

// TCP functions
void message_tcp();
void parse_message_tcp(char *message, int fd);
char* state(char* plid, int fd);
void get_hint(int fd, char* plid);
void get_scoreboard(int fd);

// aux functions to TCP functions
void send_message_tcp(int fd, char* message);
void send_file(int fd, char* file, int fsize);

int delete_table();

static void handler(int signum);

#endif