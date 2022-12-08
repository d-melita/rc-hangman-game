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
#include <signal.h>
#include <fcntl.h>
#include <errno.h>


// DEFINES

#define DEFAULT_SERVER "tejo.ist.utl.pt"
#define DEFAULT_PORT "58090" // must be 58000 + Group Number 
#define GN 90 // group number

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


// Buffer sizes

#define TIMEOUT 5

#define SIZE_IP 15 // 3*. + 12*0-9
#define SIZE_PORT 5
#define SIZE_PLID 6
#define MAX_SIZE_HOST 256

#define READ_AMOUNT 512 // Default amount of bytes to read from TCP socket


// Error messages (Consider adding '%s')

#define ERR_INVALID_PLID "ERROR: Invalid player ID\n"
#define ERR_INVALID_HOST "ERROR: Invalid hostname\n"
#define ERR_INVALID_CMD "ERROR: Invalid command\n"
#define ERR_INVALID_ARGS "ERROR: Invalid arguments\n"
#define ERR_PROTOCOL "ERROR: Sprotocol message\n"
#define ERR_UNKNOWN_CODE "ERROR: Unexpected response code\n"
#define ERR_MISSING_ARGUMENT "ERROR: '%s' option requires an argument\n"
#define ERR_INVALID_OPTION "ERROR: Invalid option\n"
#define ERR_ONGOING_GAME "ERROR: There is an ongoing game\n"
#define ERR_NO_GAME "ERROR: There is no active game\n"
#define ERR_NO_GAMES_FOUND "Game server did not find any games\n"
#define ERR_NO_MATCHES_CHAR "ERROR: The given character does not appear in current word\n"
#define ERR_DUP_CHAR "ERROR: The given letter has already been proposed\n"
#define ERR_GETADDRINFO "ERROR: getaddrinfo: %s\n"
#define ERR_SOCKET "ERROR: Socket error\n"
#define ERR_SENDTO "ERROR: Sendto error\n"
#define ERR_RECVFROM "ERROR: Recvfrom error\n"
#define ERR_CONNECT "ERROR: connect error\n"
#define ERR_WRITE "ERROR: write error\n"
#define ERR_READ "ERROR: read error\n"
#define ERR_OPEN_FILE "ERROR: Could not open file\n"
#define ERR_CLOSE_FILE "ERROR: Could not close file\n"
#define ERR_WRITE_FILE "ERROR: Could not write to file\n"
#define ERR_NO_PLID "ERROR: No player ID\n"
#define ERR_WRONG_CHAR_GAMEOVER "ERROR: The given letter does not appear in current word and the game is over\n"
#define ERR_INVALID "ERROR: Trial number not valid or repeating last PLG stored at the GS with different letter\n"
#define ERR_INVALID_2 "ERROR: PLG syntax incorrect, PLID not valid or there's no ongoing game for the specified player PLID\n"
#define ERR_WRONG_GUESS "ERROR: The guess was incorrect!\n"
#define ERR_DUP_GUESS "ERROR: This word has already been rejected\n"
#define ERR_WRONG_GUESS_GAMEOVER "ERROR: The guess was incorrect and the game is over\n"
#define ERR_FILE_NOK "ERROR: There's no file to be sent or some other problem occured\n"
#define ERR_TIMEOUT "ERROR: Socket timeout\n"
#define ERR_SELECT "ERROR: Select error\n"
#define ERR_SCANF "ERROR: scanf error, EOF reached or empty string read.\n"

#define WARN_EMPTY_SB "Scoreboard is empty\n"

// Function prototypes
void parse_args(int argc, char *argv[]);
void get_ip();
void get_ip_known_host(char *host);
void start_function();
void play_function();
void guess_function();
void hint_function();
void state_function();
void scoreboard_function();
void quit_function();

void message_udp(char *buffer);
void parse_response_udp(char *message);
void set_new_game(char *message);
void play_made(char *message);
void parse_message_play(char *message, int pos[]);
void win_function();
void win_word_function();

void message_tcp(char *buffer);
void parse_response_tcp(int fd, char *message);
void scoreboard(int fd, char *message);
void get_hint(int fd, char *message);
void game_status(int fd, char *message);

/// Read unknown length string from socket to buffer,
/// copying it to 'string' buffer as a string.
///
/// \param fd file descriptor to be read (socket)
/// \param code response code
/// \param status response status
/// \param response response message (with file)
///
/// \returns 0 on success
int read_buffer2string(int fd, char *buffer, char *string);

/// Receive file from socket, saving it locally.
///
/// \param fd file descriptor to be read (socket)
/// \param code response code
/// \param status response status
/// \param response response message (with file)
///
/// \returns pointer to response message
char* get_file(int fd, char* code, char* status, char* response);

/// Set a given signal to be handled by handler function.
///
/// \param signum signal number
///
static void handler(int signum);

/// Watch a socket for read/write availability, with a timeout.
///
/// \param fd file descriptor to be watched
/// \param readWrite 1 if the socket is to be read, 0 if it is to be written
/// \param timeout timeout in seconds
///
/// \returns 0 on success
int select_socket(int fd, int readWrite, int timeout);

#endif  