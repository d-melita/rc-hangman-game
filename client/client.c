#include "client.h"

char ip[16];
char port[6];
char plid[7] = "";
int game_ongoing = 0;

struct current_game {
  int trial;
  char word[31];
  int length;
  int errors;
  int max_errors;
  char last_letter[2];
  char word_guessed[31];
};

struct current_game current_game;

int main(int argc, char *argv[]) {
  signal(SIGPIPE, handler); // Ignore SIGPIPE (broken pipe)
  signal(SIGINT, handler);
  

  int n;
  char command[20];

  parse_args(argc, argv);

  n = scanf("%s", command);
  if (n == EOF || n == 0) {
    exit(1); // EOF or no input
  }

  while (strcmp(command, EXIT) != 0) {

    if (strcmp(command, START) == 0 || strcmp(command, SG) == 0) {
      start_function();
    }

    // TODO check if game_ongoing == 1??
    else if (strcmp(command, PLAY) == 0 || strcmp(command, PL) == 0) {
      play_function();
    }

    else if (strcmp(command, QUIT) == 0) {
      quit_function();
    }

    else if (strcmp(command, GUESS) == 0 || strcmp(command, GW) == 0) {
      guess_function();
    }

    else if (strcmp(command, SCOREBOARD) == 0 || strcmp(command, SB) == 0) {
      scoreboard_function();
    }

    else if (strcmp(command, HINT) == 0 || strcmp(command, H) == 0) {
      hint_function();
    }

    else if (strcmp(command, STATE) == 0 || strcmp(command, ST) == 0) {
      state_function();
    }

    else {
      printf(ERR_INVALID_CMD);
      // Clear stdin until newline using something other than getchar
      while (getchar() != '\n');
    }

    n = scanf("%s", command);
    if (n == EOF || n == 0) {
      exit(1); // EOF or no input
    }
  }

  if (game_ongoing == 1) {
    quit_function();
  }
  return 0;
}

// Parse the arguments given to the program (host and port)
void parse_args(int argc, char *argv[]) {
  // Two arguments, -n and -p
  strcpy(port, DEFAULT_PORT);
  get_ip(); // get ip of current machine

  for (int i = 1; i < argc; i++) { // Parse each given option and argument

    if (strcmp(argv[i], "-n") == 0) { // Host option (-n)
      if (i - 1 < argc) {
        get_ip_known_host(argv[++i]);
        strcpy(port, DEFAULT_PORT);
      } else {
        printf(ERR_MISSING_ARGUMENT, argv[i]);
        exit(1);
      }

    } else if (strcmp(argv[i], "-p") == 0) { // Port option (-p)
      if (i - 1 < argc)
        strcpy(port, argv[++i]);
      else {
        printf(ERR_MISSING_ARGUMENT, argv[i]);
        exit(1);
      }

    } else { // Invalid option
      printf(ERR_INVALID_OPTION);
      exit(1);
    }
  }
}

// Get the ip of current machine when '-n' not specified
void get_ip() {
  char host[256];
  int hostname;

  hostname = gethostname(host, sizeof(host));
  if (hostname == -1) {
    perror(ERR_INVALID_HOST);
    exit(1);
  }

  get_ip_known_host(host);
}

// Get the ip of host when '-n' specified.
void get_ip_known_host(char *host) {
  struct addrinfo hints, *res, *p;
  int errcode;
  char buffer[INET_ADDRSTRLEN];
  struct in_addr *addr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;

  if ((errcode = getaddrinfo(host, NULL, &hints, &res)) != 0) {
    fprintf(stderr, ERR_GETADDRINFO, gai_strerror(errcode));
    exit(1);
  } else {
    for (p = res; p != NULL; p = p->ai_next) {
      addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
      inet_ntop(p->ai_family, addr, buffer, sizeof(buffer));
      strcpy(ip, buffer);
    }
    freeaddrinfo(res);
  }
}

// Send start message to server.
void start_function() {
  int n;
  char t_plid[7];

  char message[12];

  n = scanf("%6s", t_plid);
  if (n == EOF || n == 0) {
    perror(ERR_SCANF);
    exit(1); // EOF or no input
  }

  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    return;
  }

  if (game_ongoing == 1) {
    printf(ERR_ONGOING_GAME);
    return;
  }

  if (strlen(t_plid) != 6) {
    printf(ERR_INVALID_PLID);
    return;
  }

  strcpy(plid, t_plid);
  sprintf(message, "%s %s\n", SNG, plid);

  message_udp(message);
}

// Send play message to server.
void play_function() {
  int n;
  char *message;
  char trial_str[10];

  sprintf(trial_str, "%d", current_game.trial);
  message = malloc(13 + strlen(trial_str) + 2);
  // 13 (PLG + plid + space) + strlen(trial_str) + 2 (\0 + \n)

  n = scanf("%s", current_game.last_letter);
  if (n == EOF || n == 0) {
    exit(1); // EOF or no input
  }
  
  current_game.last_letter[0] = toupper(current_game.last_letter[0]);
  sprintf(message, "%s %s %s %d\n", PLG, plid, current_game.last_letter,
          current_game.trial);
  printf("%s", message);

  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    free(message);
    return;
  }

  if (game_ongoing == 0) {
    printf(ERR_NO_GAME);
    return;
  }

  message_udp(message);
  free(message);
}

// Send guess message to server.
void guess_function() {
  int n;
  char *message;
  char trial_str[10];

  sprintf(trial_str, "%d", current_game.trial);

  n = scanf("%s", current_game.word_guessed);
  if (n == EOF || n == 0) {
    exit(1); // EOF or no input
  }
  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    free(message);
    return;
  }

  if (game_ongoing == 0) {
    printf(ERR_NO_GAME);
    return;
  }

  for (int i = 0; i < strlen(current_game.word_guessed); i++) {
    current_game.word_guessed[i] = toupper(current_game.word_guessed[i]);
  }

  message = malloc(12 + strlen(current_game.word_guessed) + strlen(trial_str));

  sprintf(message, "%s %s %s %d\n", PWG, plid, current_game.word_guessed,
          current_game.trial);
  message_udp(message);

  free(message);
}

void scoreboard_function() { 
  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    return;
  }

  message_tcp(GSB);
}

void hint_function() {
  char message[12];

  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    return;
  }

  if (game_ongoing == 0) {
    printf(ERR_NO_GAME);
    return;
  }

  sprintf(message, "%s %s\n", GHL, plid);
  message_tcp(message);
}

void state_function() {
  char message[12];

  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    return;
  }

  if (strcmp(plid, "") == 0) {
    printf(ERR_NO_PLID);
    return;
  }

  sprintf(message, "%s %s\n", STA, plid);
  message_tcp(message);
}

// Notify the server to quit the current game.
void quit_function() {
  char message[12];

  if (clear_input() == 1) {
    puts(ERR_INVALID_ARGS);
    return;
  }

  if (game_ongoing == 0) {
    printf(ERR_NO_GAME);
    return;
  }

  sprintf(message, "%s %s\n", QUT, plid);
  message_udp(message);
  game_ongoing = 0;
}

/* --------------------------------------------------------------------------------------------------------------


                                                CLIENT UDP FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

void message_udp(char *buffer) {
  int fd, errcode;
  ssize_t n;
  socklen_t addrlen;
  struct addrinfo hints, *res;
  struct sockaddr_in addr;
  char response[128];


  fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
  if (fd == -1) {
    perror(ERR_SOCKET);
    exit(1);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  printf("IP: %s, Port: %s\n", ip, port);
  errcode = getaddrinfo(ip, port, &hints, &res);
  if (errcode != 0) {
    fprintf(stderr, ERR_GETADDRINFO, gai_strerror(errcode));
    exit(1);
  }

  n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
  if (n == -1) {  // CHECK IF CONNECTION DROPPED LOCALLY OR ETC
    perror(ERR_SENDTO);
    puts(ERR_CONNECTION);
    return;
  }

  struct timeval tv = {TIMEOUT, 0};
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0) {
    if (errno == ETIMEDOUT) {
      printf(ERR_TIMEOUT);
      return;
    } else {
      perror("setsockopt");
      exit(1);
    }
  }

  addrlen = sizeof(addr);
  n = recvfrom(fd, response, 128, 0, (struct sockaddr *)&addr, &addrlen);
  if (n == -1) {  // CHECK IF CONNECTION DROPPED LOCALLY OR ETC
    perror(ERR_RECVFROM);
    puts(ERR_CONNECTION);
    return;
  }

  response[n] = '\0';
  parse_response_udp(response);

  freeaddrinfo(res);
  close(fd);
}

// function to parse server response
void parse_response_udp(char *message) {
  char code[4];
  char status[4];
  char *word;

  printf("%s", message);

  // scan the message and get the code and status
  sscanf(message, "%s %s", code, status);

  if (strcmp(code, RSG) == 0) {
    if (strcmp(status, OK) == 0) {
      set_new_game(message);
    } else if (strcmp(status, NOK) == 0) {
      printf(ERR_ONGOING_GAME);
    }
  }

  else if (strcmp(code, RLG) == 0) {
    if (strcmp(status, OK) == 0) {
      play_made(message);
      // increment trial
      current_game.trial++;
    } else if (strcmp(status, NOK) == 0) {
      current_game.errors++;
      current_game.trial++;
      printf(ERR_NO_MATCHES_CHAR);
    } else if (strcmp(status, DUP) == 0) {
      printf(ERR_DUP_CHAR);
    }

    else if (strcmp(status, WIN) == 0) {
      win_function();
      game_ongoing = 0;
    }

    else if (strcmp(status, OVR) == 0) {
      current_game.errors++;
      game_ongoing = 0;
      printf(ERR_WRONG_CHAR_GAMEOVER);
    }

    else if (strcmp(status, INV) == 0) {
      printf(ERR_INVALID);
    }

    else if (strcmp(status, ERR) == 0) {
      printf(ERR_INVALID_2);
    }
  }

  else if (strcmp(code, RWG) == 0) {
    if (strcmp(status, NOK) == 0) {
      current_game.errors++;
      current_game.trial++;
      printf(ERR_WRONG_GUESS);
    } else if (strcmp(status, DUP) == 0) {
      printf(ERR_DUP_GUESS);
    } else if (strcmp(status, WIN) == 0) {
      win_word_function();
      game_ongoing = 0;
    }

    else if (strcmp(status, OVR) == 0) {
      current_game.errors++;
      game_ongoing = 0;
      printf(ERR_WRONG_GUESS_GAMEOVER);
    }

    else if (strcmp(status, INV) == 0) {
      printf(ERR_INVALID);
    }

    else if (strcmp(status, ERR) == 0) {
      printf(ERR_INVALID_2);
    }
  }

  else if (strcmp(code, RQT) == 0) {
    if (strcmp(status, OK) == 0) {
      printf(GAME_OVER);
    } else if (strcmp(status, ERR) == 0) {
      printf(ERR_NO_GAME);
    }
  }

  else if (strcmp(code, RVV) == 0) {
    printf("%s\n", status); // only using during devolopment to display the word
                            // to bue guessed

    // only when server finished
    /*
    if (strcmp(status, OK) == 0){
        quit_function();
    }

    else if (strcmp(status, ERR) == 0){
        printf("Error: There's no ongoing game for the specified player
    PLID\n");
    }
    */
  } else if (strcmp(code, ERR) == 0) {

    printf(ERR_PROTOCOL);
  } else {

    printf(ERR_UNKNOWN_CODE);
  }
}

// function to set a new game
void set_new_game(char *message) {
  char code[4];
  char status[4];
  char *word; // word only containing _ _ _ _ _ (because we dont know the length
              // of the word)
  sscanf(message, "%s %s %d %d", code, status, &current_game.length,
         &current_game.max_errors);
  word = (char *)malloc((current_game.length + 1) * sizeof(char));
  for (int i = 0; i < current_game.length; i++) {
    word[i] = '-';
  }
  word[current_game.length] = '\0';
  // set new game components
  strcpy(current_game.word, word);
  free(word);

  current_game.errors = 0;
  current_game.trial = 1;
  strcpy(current_game.last_letter, "");
  game_ongoing = 1;
  printf("New game started (max %d errors): %s (word length: %d)\n",
         current_game.max_errors, current_game.word, current_game.length);
}

// function to know which positions to change in the word
void parse_message_play(char *message, int pos[]) {
  char c;
  int ne = 0; // nb of white spaces
  int i = 0;
  int j, index = 0;
  for (i; i < strlen(message); i++) {
    if (message[i] == ' ') {
      ne++;
    }
    if (ne == 4) { // i know i will have exactly 4 spaces before the positions
      break;
    }
  }
  for (j = i; j < strlen(message); j++) {
    if (message[j] != ' ') {
      if (message[j + 1] == ' ' || message[j + 1] == '\n') {
        pos[index] = message[j] - '0';

      } else {
        pos[index] = (message[j] - '0') * 10 + (message[j + 1] - '0');
      }
      index++;
    }
  }
}

// function to show the play made
void play_made(char *message) {
  int n; // number of positions where the letter appears
  char code[4];
  char status[4];

  sscanf(message, "%s %s %d %d", code, status, &current_game.trial, &n);
  int pos[n];

  parse_message_play(message, pos);

  for (int i = 0; i < n; i++) {
    current_game.word[pos[i] - 1] = current_game.last_letter[0];
  }
  printf("Correct Letter: %s\n", current_game.word);
}

void win_function() {
  for (int i = 0; i < current_game.length; i++) {
    if (current_game.word[i] == '-') {
      current_game.word[i] = current_game.last_letter[0];
    }
  }
  printf(WELL_DONE, current_game.word);
}

void win_word_function() {
  strcpy(current_game.word, current_game.word_guessed);
  printf(WELL_DONE, current_game.word);
}

/* --------------------------------------------------------------------------------------------------------------


                                                CLIENT TCP FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

int read_buffer2string(int fd, char *string) {
  int errcode;
  int n;
  char *buffer = (char *)malloc(256 * sizeof(char));

  int bytes = 0;
  n = read(fd, buffer + bytes, 1);
  if (n <= 0) {
    puts(ERR_READ);
    exit(1);
  }
  while ((buffer[bytes] != ' ') && (buffer[bytes] != '\n')) {
    bytes++;
    n = read(fd, buffer + bytes, 1);
    if (n <= 0) {
      puts(ERR_READ);
      exit(1);
    }
  }
  buffer[bytes++] = '\0';
  strcpy(string, buffer);
  return bytes;
}

void get_file(int fd) {
  int n, errcode;
  char filename[128];
  char filesize_str[128];
  char *response;
  int filesize;
  int bytes = 0;
  int to_read;

  // READ FILENAME
  bytes += read_buffer2string(fd, filename);

  // READ FILESIZE
  bytes += read_buffer2string(fd, filesize_str);
  filesize = atoi(filesize_str);

  // READ AND WRITE IMAGE
  FILE *fp = fopen(filename, "w");
  if (fp == NULL) {
    perror(ERR_OPEN_FILE);
    exit(1);
  }

  response = (char*)malloc(filesize*sizeof(char));

  bytes = 0;
  while (bytes < filesize) {
    if (READ_AMOUNT > filesize - bytes)
      to_read = filesize - bytes;
    else
      to_read = READ_AMOUNT;
    n = read(fd, response + bytes, to_read);
    if (n <= 0) {
      perror(ERR_READ);
      exit(1);
    }
    n = fwrite(response + bytes, 1, n, fp);
    if (n <= 0) {
      perror(ERR_WRITE_FILE);
      exit(1);
    }
    bytes += n;
  }
  fclose(fp);

  printf(FILE_RECEIVED, filename, filesize);
  free(response);
}

void message_tcp(char *buffer) {
  int fd, errcode, errno;
  ssize_t n;
  socklen_t addrlen;
  struct addrinfo hints, *res;
  struct sockaddr_in addr;
  int bytes;

  fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
  if (fd == -1) {
    perror(ERR_SOCKET);
    exit(1);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  // Timeout for TCP socket
  struct timeval tv = {TIMEOUT, 0};
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0) {
    if (errno == ETIMEDOUT) {
      printf(ERR_TIMEOUT);
      return;
    } else {
      perror("setsockopt");
      exit(1);
    }
  }

  errcode = getaddrinfo(ip, port, &hints, &res);
  if (errcode != 0) {
    fprintf(stderr, ERR_GETADDRINFO, gai_strerror(errcode));
    exit(1);
  }

  n = connect(fd, res->ai_addr, res->ai_addrlen);

  // Send message to server
  bytes = 0;
  while (bytes < strlen(buffer)) {
    n = write(fd, buffer + bytes, strlen(buffer) - bytes);
    if (n <= 0) {
      perror(ERR_WRITE);
      exit(1);
    }
    bytes += n;
  }

  parse_response_tcp(fd);

  freeaddrinfo(res);
  close(fd);
}

void parse_response_tcp(int fd) {
  char code[4];
  char status[7];

  // READ CODE
  read_buffer2string(fd, code);
  // READ STATUS
  read_buffer2string(fd, status);

  if (strcmp(code, RSB) == 0) { // SCOREBOARD
    if (strcmp(status, OK) == 0) {
      get_file(fd);
    }

    else if (strcmp(status, EMPTY) == 0) {
      printf(WARN_EMPTY_SB);
    }
  }

  else if (strcmp(code, RHL) == 0) { // HINT
    if (strcmp(status, OK) == 0) {
      get_file(fd);
    }

    else if ((strcmp, NOK) == 0) {
      printf(ERR_FILE_NOK);
    }
  }

  else if (strcmp(code, RST) == 0) { // STATE
    if (strcmp(status, ACT) == 0 || strcmp(status, FIN) == 0) {
      get_file(fd); // active or last game finished
    }

    else if (strcmp(status, NOK) == 0) {
      printf(ERR_NO_GAMES_FOUND);
    }
  }

  else if (strcmp(code, ERR) == 0) { //ERROR
    printf(ERR_PROTOCOL);
  }

  else {
    printf(ERR_UNKNOWN_CODE);
  }
}

static void handler(int signum) {
  switch (signum) {
  case SIGINT:
    printf(CLOSING_SIGNAL);
    exit(1);
  case SIGPIPE:
    printf(BROKEN_PIPE);
    exit(1);
  default:
    printf(IGNORING_SIGNAL);
  }
}

int clear_input() {
  int c;
  int ret = 0;

  while ((c = getchar()) != '\n') {
    if (c != ' ' && c != '\t')
      ret = 1;
  }
  return ret;
}