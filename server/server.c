#include "server.h"

// STRUCT FOR MATCH

struct game_data {
  int trial;
  char *word;
  int length;
  int errors;
  int max_errors;
  char last_letter[2];
  char *word_guessed;
};

struct game_id {
  char plid[7];
  struct game_data *game_data;
  struct game_id *next;
  struct game_id *prev;
};

struct game_id *game = NULL;

char port[6];
int verbose = 0; // 0 false, 1 true

int main(int argc, char *argv[]) {

  parse_args(argc, argv);

  printf("Server started on port %s\n", port);
  if (verbose)
    printf("Verbose mode enabled\n");

  message_udp();

  return 0;
}

// Parse the arguments given to the program (host and port)
void parse_args(int argc, char *argv[]) {
  // Two arguments, -v and -p
  if (argc == 1) { // case 1

    strcpy(port, DEFAULT_PORT);

  } else {
    for (int i = 1; i < argc; i++) { // Parse each given option and argument

      if (strcmp(argv[i], "-v") == 0) { // Host option (-n)
        // SET VERBOSE
        verbose = 1;

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
}

/* --------------------------------------------------------------------------------------------------------------


                                                SERVER UDP FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

void message_udp() {
  int fd, errcode;
  int n;
  socklen_t addrlen;
  struct addrinfo hints, *res;
  struct sockaddr_in addr;
  char buf[256];
  int num = 0;

  fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
  if (fd == -1) {
    perror("socket");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  errcode = getaddrinfo(NULL, port, &hints, &res);
  if (errcode != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
    exit(1);
  }

  n = bind(fd, res->ai_addr, res->ai_addrlen);
  if (n == -1) {
    perror("bind");
    exit(1);
  }

  while (1) {
    printf("---DEBUG: %d---\n", ++num);

    addrlen = sizeof(addr);
    n = recvfrom(fd, buf, 256, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
      perror("recvfrom");
      exit(1);
    }

    buf[n] = '\0';

    if (verbose == 1) {
      printf("RECEIVED: '%s'\n", buf);
    }

    parse_message_udp(buf);
    n = sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&addr, addrlen);
    if (n == -1) {
      perror("sendto");
      exit(1);
    }

    if (verbose == 1) {
      printf("SENT: '%s'\n", buf);
    }
  }
  close(fd);
}

void parse_message_udp(char *message) {
  char code[4];
  sscanf(message, "%s", code);

  if (strcmp(code, SNG) == 0) {
    // start new game
    start_new_game(message);
  }

  else if (strcmp(code, PLG) == 0) {
    play_letter(message);
  }

  else if (strcmp(code, PWG) == 0) {
    // guess word
    guess_word(message);
  }

  else if (strcmp(code, QUT) == 0) {
    // Quit game
    quit(message);
  }

  else {
    printf("ERROR: Invalid message code\n");
    // error
  }
}

void start_new_game(char *message) {
  char code[4];
  char plid[7];
  char status[4];
  int word_length;
  char response[256];

  sscanf(message, "%s %s", code, plid);

  // FIND GAME WITH PLID

  struct game_id *curr_game = get_game(plid);

  if (curr_game == NULL) { // NO ACTIVE GAMES FOUND FOR PLAYER
    curr_game = malloc(sizeof(struct game_id));
    curr_game->game_data = malloc(sizeof(struct game_data));
    curr_game->next = NULL;
    strcpy(curr_game->plid, plid);
    if (game == NULL) { // NO ACTIVE GAMES AT ALL
      curr_game->prev = NULL;
      game = curr_game;
    }

    word_length = set_game_word(curr_game->game_data);
    strcpy(status, OK);
  } else // PLAYER HAS AN ONGOING GAME
    strcpy(status, NOK);

  if (strcmp(status, OK) == 0) {
    sprintf(message, "%s %s %d %d", RSG, status, word_length, MAX_ERRORS);
  } else
    sprintf(message, "%s %s", RSG, status);
}

void play_letter(char *message) {
  char code[4];
  char plid[7];
  char letter[2];
  int trial;
  char status[4];

  sscanf(message, "%s %s %s %d", code, plid, letter, trial);

  // if letter is in word, strcpy(status, OK); give number of occurences and
  // positions

  // else if letter is in word and word == guessed, strcpy(status, WIN);

  // else if letter is not in word & errors++ != max_errors, strcpy(status,
  // NOK);

  // else if letter is not in word & errors++ == max_errors, strcpy(status, OVR)
  // and end game;

  // else if letter sent on a previous trial, strcpy(status, DUP);

  // else if trial != trial, strcpy(status, INV);

  // check on going game
  // if no on going game, strcpy(status, ERR);

  // send message to client
}

void guess_word(char *message) {
  char code[4];
  char plid[7];
  char word[31];
  int trial;
  char status[4];

  sscanf(message, "%s %s %s %d", code, plid, word, trial);

  // if word == guessed, strcpy(status, WIN);

  // else if word != guessed & errors++ != max_errors, strcpy(status, NOK);

  // else if word != guessed & errors++ == max_errors, strcpy(status, OVR) and
  // end game;

  // else if word sent on a previous trial, strcpy(status, DUP);

  // else if trial != trial, strcpy(status, INV);

  // check on going game
  // if no on going game, strcpy(status, ERR);

  // send message to client
}

void quit(char *message) {
  char code[4];
  char plid[7];
  char status[4];

  sscanf(message, "%s %s", code, plid);

  struct game_id *curr_game = get_game(plid);
  if (curr_game == NULL) { // NO ACTIVE GAMES FOUND FOR PLAYER
    printf("DEBUG: No active games found for player\n");

    strcpy(status, ERR);
  } else {                       // ACTIVE GAME FOUND
    if (curr_game->prev == NULL) // GAME IS FIRST IN LIST
      game = curr_game->next;
    else
      curr_game->prev->next = curr_game->next;

    if (curr_game->next != NULL) // GAME IS NOT LAST IN LIST
      curr_game->next->prev = curr_game->prev;

    free(curr_game->game_data->word);
    free(curr_game->game_data->word_guessed);
    free(curr_game->game_data);
    free(curr_game);

    strcpy(status, OK);
  }

  sprintf(message, "%s %s", RQT, status);
  // send message to client
}

// Returns the length of the word
int set_game_word(struct game_data *game_data) {
  char word[31];
  int i;
  int word_length;
  int random_number;

  FILE *fp = fopen("words.txt", "r");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  // get random number
  srand(time(0));
  random_number = rand() % WORDFILE_SIZE;

  // get word from file
  for (i = 0; i <= random_number; i++) {
    fscanf(fp, "%s", word);
  }

  word_length = strlen(word);

  // set game data
  game_data->length = word_length;
  game_data->word = malloc((word_length + 1) * sizeof(char));
  strcpy(game_data->word, word);
  game_data->word_guessed = malloc((word_length + 1) * sizeof(char));

  for (i = 0; i < word_length; i++) {
    game_data->word_guessed[i] = '_';
  }
  game_data->errors = 0;
  game_data->trial = 0;

  fclose(fp);
  return word_length;
}

struct game_id *get_game(char *plid) {
  struct game_id *curr_game = game;

  while (curr_game != NULL) {
    if (strcmp(curr_game->plid, plid) == 0)
      return curr_game;
    curr_game = curr_game->next;
  }

  return NULL;
}