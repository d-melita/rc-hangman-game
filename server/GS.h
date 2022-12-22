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
#include <sys/file.h>
#include <sys/stat.h>

// DEFINES

// GENERAL
#define DEFAULT_PORT "58090" // must be 58000 + Group Number 
#define GN 90 // group number
#define MAX_ERRORS 8 // Maximum number of errors allowed
#define SIZE_WORDFILE 10
#define DEFAULT_TABLE_SIZE 15625 // max 64 collisions, 125kb
#define MAX_TABLE_SIZE 1000000 // no collisions, 8mb
#define TIMEOUT 10 // seconds

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
#define VERBOSE_ON "Verbose mode enabled"
#define UDP_RECV "New UDP message received"
#define TCP_RECV "New TCP message received"
#define SB_REQUEST "Scoreboard requested"
#define CONN_ACCP "Connection accepted"
#define SERVER_SD "\nServer shutting down..."
#define IG_SIG "\nIGNORING_SIGNAL"

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
#define GAMES_DIR "GAMES"

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
#define ERR_INVALID_OPTION "ERROR: Invalid option"
#define ERR_INVALID_WORDFILE "ERROR: Word's file is invalid, check permissions or if file exists"
#define ERR_FILE_DNE "ERROR: File does not exist"
#define ERR_MALLOC "ERROR: Malloc failed"
#define ERR_FORK "ERROR: Fork failed"
#define ERR_SOCKET "ERROR: Socket failed"
#define ERR_BIND "ERROR: Bind failed"
#define ERR_RECVFROM "ERROR: Recvfrom failed"
#define ERR_SENDTO "ERROR: Sendto failed"
#define ERR_GETADDRINFO "ERROR: Getaddrinfo failed"
#define ERR_LISTEN "ERROR: Listen failed"
#define ERR_ACCEPT "ERROR: Accept failed"
#define ERR_READ "ERROR: Read failed"
#define ERR_WRITE "ERROR: Write failed"
#define ERR_FOPEN "ERROR: Open failed"
#define ERR_FSCANF "ERROR: Fscanf failed"
#define ERR_FREAD "ERROR: Fread failed"
#define ERR_OPENDIR "ERROR: Opendir failed"
#define ERR_TIMEOUT "ERROR: Connection timed out"

// SCOREBOARD HEADER
#define SCOREBOARD_HEADER "   SCORE    PLID    SUCCESSFULL TRIALS    TOTAL TRIALS    WORD  \n"


// prototypes
// structs

struct game_data; // Game state (word progress, errors, etc)
struct game_id; // Player id and associated game data, etc
struct guessed_word; // Array of guessed words

/// Get hash of player id for hash table.
///
/// \param plid player id
/// \return hash of player id
int hash(char* plid);

/// Add game to memory.
///
/// \param plid player id of new game
/// \return 0 if successful, 1 otherwise
int set_game(char* plid);

/// Resize hash table to, by default, double of its size.
///
/// \return 0 if successful, 1 otherwise
int resize_table();

// Parse arguments from argv
void parse_args(int argc, char *argv[]);

// UDP functions /////////////////////////////////////

/// Handle TCP connections.
///
/// \return void
void message_udp();

/// Parse UDP message from client.
///
/// \param message udp message received
/// \return message to be sent to udp client
char* parse_message_udp(char *message);

/// Start new game requested by udp client.
///
/// \param message udp message received
/// \return message to be sent to udp client
char* start_new_game(char *message);

/// Respond to a play letter request from UDP client.
///
/// \param message udp message received
/// \return message to be sent to udp client
char* play_letter(char *message);

/// Respond to a guess from UDP client.
/// 
/// \param message udp message received
/// \return message to be sent to udp client
char* guess_word(char *message);

/// Respond to a quit request from UDP client.
///
/// \param message udp message received
/// \return message to be sent to udp client
char* quit(char *message);

/// Attempt to add a new game record to scoreboard.
/// Calls 'add_scoreboard_line' to add the line to the scoreboard.
///
/// \param game_id game id struct of the game
///
/// \return void
void store_game(struct game_id* game_id);

/// Attempt to add a new game record to scoreboard.
///
/// \param fp scoreboard file
/// \param game_id game id struct of the game
///
/// \return void
void add_scoreboard_line(FILE* fp, struct game_id *curr_game);

// aux functions to UDP functions ////////////////////////////////

/// Setup game data for a new game.
///
/// \param game_id game id struct of the game
///
/// \return void
int set_game_data(struct game_id* game_id);

/// Get game from memory.
///
/// \param plid player id
///
/// \return game_id struct
struct game_id* get_game(char* plid);

/// Verify if given letter is in given word.
/// Used to check if letter was already played.
///
/// \param word word to be checked
/// \param letter letter to be checked
///
/// \return 1 if letter is in word, 0 otherwise
int letter_in_word(char* word, char* letter);

/// Delete game (data) from memory. Keeps the game_id (player id).
///
/// \param game_id game id
///
/// \return void
void delete_game(struct game_id* game_id);

/// Check if word guess attempt was already made.
///
/// \param word word to be checked
/// \param guessed_words array of previous guess attempts
///
/// \return 1 if guess was already attempted, 0 otherwise
int word_played(char* word, struct guessed_word* guessed_words);

/// Update game status after a correct play or guess was made.
///
/// \param game game id
/// \param attempt attempt made by player (letter or word)
/// \param status status of attempt (OK, WIN, OVR)
///
/// \return void
void update_game_status(struct game_id *game, char* attempt, char* status);

// TCP functions //////////////////////////////////////////////////////////

/// Handle TCP connections.
///
/// \return void
void message_tcp();

/// Parse message received from client via TCP.
///
/// \param message message received from client
/// \param fd file descriptor of socket connection to client
///
/// \return void
void parse_message_tcp(char *message, int fd);

/// Send the state of the game to client.
///
/// \param plid player id
/// \param fd file descriptor of socket connection to client
///
/// \return void
char* state(char* plid, int fd);

/// Send a hint to client.
///
/// \param fd file descriptor of socket connection to client
/// \param plid player id
///
/// \return void
void get_hint(int fd, char* plid);

/// Send the scoreboard to client.
///
/// \param fd file descriptor of socket connection to client
///
/// \return void
void get_scoreboard(int fd);

// aux functions to TCP functions

/// Send message to client via TCP.
///
/// \param fd file descriptor of socket connection to client
/// \param message message to be sent
///
/// \return void
void send_message_tcp(int fd, char* message);

/// Send file to client.
///
/// \param fd file descriptor of socket connection to client
/// \param file file to be sent
/// \param fsize size of file to be sent
///
/// \return void
void send_file(int fd, char* file, int fsize);

/// Delete the hash table.
///
/// \return 0 on success, 1 on error
int delete_table();

/// Check existence of GAMES folder and create it if it doesn't exist.
///
/// \return 0 if folder exists or was created, 1 on error
int check_gamesFolder();

/// Verifies if given buffer is only composed of letters.
///
/// \param buf buffer to be verified
/// \return 1 if buffer is only composed of letters, 0 otherwise
int is_word(char* buf);

/// Verifies if given buffer is only composed of digits.
///
/// \param buf buffer to be verified
/// \return 1 if buffer is only composed of digits, 0 otherwise
int is_number(char* buf);

/// Verifies if given message follows the protocol and
/// is valid.
///
/// \param message message to be verified
/// \return 0 if message is valid, 1 otherwise
int clear_input(char* message);

/// Signal handler.
///
/// \param signum signal number
static void handler(int signum);

#endif