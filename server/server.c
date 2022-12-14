#include "server.h"

// STRUCT FOR MATCH

typedef struct game_data {
  int trial;
  char *word;
  char *file;
  int errors;
  int max_errors;
  int letters_guessed; // number of letters correctly guessed
  char letters_played[31]; // 30 max word size + \0
  struct guessed_word *guesses;

  char last_letter[2];
  char word_status[31];
} game_data;

typedef struct game_id {
  char plid[7];
  struct game_data *game_data;
  // struct game_data *last_game;
  struct game_id *next;
  struct game_id *prev;
} game_id;

typedef struct guessed_word {
  char *word;
  struct guessed_word *prev;
} guessed_word;

typedef struct sb_entry {
  char *plid;
  char *word;
  int score;
  int good_trials; // total_trials - errors
  int total_trials; // trials
  struct sb_entry *next;
} sb_entry;

game_id *games = NULL;

char port[6];
int verbose = 0; // 0 false, 1 true

int main(int argc, char *argv[]) {

  parse_args(argc, argv);

  printf("Server started on port %s\n", port);
  if (verbose)
    printf("Verbose mode enabled\n");

  fork();
  message_udp();

  return 0;
}

// Parse the arguments given to the program (host and port)
void parse_args(int argc, char *argv[]) {
  // Two arguments, -v and -p
  strcpy(port, DEFAULT_PORT);

  for (int i = 1; i < argc; i++) { // Parse each given option and argument

    if (strcmp(argv[i], "-v") == 0) { // Host option (-n)
      // SET VERBOSE
      verbose = 1;

    } else if (strcmp(argv[i], "-p") == 0) { // Port option (-p)
      if (i - 1 < argc) {
        strcpy(port, argv[++i]);
      } else {
        printf(ERR_MISSING_ARGUMENT, argv[i]);
        exit(1);
      }

    } else { // Invalid option
      printf(ERR_INVALID_OPTION);
      exit(1);
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
  char *response;
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
      puts("New UDP message received");
      printf("CLIENT IP: '%s';PORT: '%d'\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    }

    response = parse_message_udp(buf);
    n = sendto(fd, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);
    if (n == -1) {
      perror("sendto");
      exit(1);
    }

    if (verbose == 1) {
      printf("SENT: '%s'\n", response);
    }
  }
  free(response);
  close(fd);
}

char* parse_message_udp(char *message) {
  char code[4];
  sscanf(message, "%s", code);

  if (strcmp(code, SNG) == 0) {
    // start new game
    return start_new_game(message);
  }

  else if (strcmp(code, PLG) == 0) {
    return play_letter(message);
  }

  else if (strcmp(code, PWG) == 0) {
    // guess word
    return guess_word(message);
  }

  else if (strcmp(code, QUT) == 0) {
    // Quit game
    return quit(message);
  }

  else {
    printf("ERROR: Invalid message code\n");
    return NULL;
    // error
  }
}

char* start_new_game(char *message) {
  char code[4];
  char plid[7];
  char status[4];
  int word_length;
  char* response;
  response = (char*) malloc(256 * sizeof(char));

  sscanf(message, "%s %s", code, plid);

  if (verbose == 1)
    printf("Player %s requested a new game.\n", plid);

  // FIND GAME WITH PLID
  game_id *curr_game = get_game(plid);

  if (curr_game == NULL || curr_game->game_data == NULL) { // PLAYER HAS NO ONGOING GAME
    curr_game = malloc(sizeof(game_id));
    curr_game->game_data = malloc(sizeof(game_data));
    // curr_game->last_game = NULL;
    strcpy(curr_game->plid, plid);

    if (games != NULL)
      games->prev = curr_game;

    curr_game->next = games;
    games = curr_game;

    word_length = set_game_word(curr_game->game_data);
    strcpy(status, OK);

    sprintf(response, "%s %s %d %d\n", RSG, status, word_length, MAX_ERRORS);

    printf("New game started for %s with word: '%s'\n", plid, curr_game->game_data->word);
  } else { // PLAYER HAS AN ONGOING GAME
    strcpy(status, NOK);
    sprintf(response, "%s %s\n", RSG, status);
  }
  
  return response;
}

char* play_letter(char *message) {
  char status[4];
  char code[4];
  char plid[7];
  char letter[2];
  int trial;
  char* buffer;
  buffer = (char*) malloc(256 * sizeof(char));

  sscanf(message, "%s %s %s %d\n", code, plid, letter, &trial);

  if (verbose == 1)
    printf("Player %s requested to play letter %s.\n", plid, letter);

  game_id *game_id = get_game(plid);
  if (game_id != NULL) {          // 1: THERES AN ACTIVE GAME FOR PLAYER
    game_data *game = game_id->game_data;

    if (trial != game->trial) {   // 2: TRIAL NUMBER DOESNT MATCH -> INV

      if (trial == game->trial-1 &&
       strcmp(game->last_letter, letter) == 0) // TODO
        return NULL; // RESEND LAST PLAY RESPONSE TODO
      else {
        strcpy(status, INV);
        sprintf(buffer, "%s %s %d\n", RLG, status, game->trial);
      }

                                  // 3: LETTER ALREADY PLAYED -> DUP
    } else if (letter_in_word(game->letters_played, letter)) {
      
      strcpy(status, DUP);
      sprintf(buffer, "%s %s %d\n", RLG, status, game->trial);

    } else {

                                  // 4: LETTER IS IN WORD -> OK
      if (letter_in_word(game->word, letter)) {
        char pos_str[31] = "";
        char test[2] = "";
        int n = 0;

        strcpy(status, OK);
        for (int i = 0; i < strlen(game->word); i++) {
          if (game->word[i] == letter[0]) {
            sprintf(test, "%d", i+1);
            sprintf(pos_str, "%s %s", pos_str, test);
            game->letters_guessed++;
            game->word_status[i] = letter[0];
            n++;
          }
        }

        // 4.1: WORD IS GUESSED -> WIN
        if (game->letters_guessed == strlen(game->word)) {
          strcpy(status, WIN);
          // TODO
        }

        sprintf(buffer, "%s %s %d %d%s\n", RLG, status, game->trial++, n, pos_str);

      // 5: LETTER IS NOT IN WORD -> NOK
      } else {
        strcpy(status, NOK);
        game->errors++;

        // 5.1: MAX ERRORS REACHED -> OVR
        if (game->errors == game->max_errors) {
          strcpy(status, OVR);
          
        }

        sprintf(buffer, "%s %s %d\n", RLG, status, game->trial++);
      }
      strcpy(game->last_letter, letter);

      update_game_status(game_id, letter);
    }

  } else {
    strcpy(status, ERR);
    sprintf(buffer, "%s %s\n", RLG, status);
  }

  return buffer;
}

char* guess_word(char *message) {
  char code[4];
  char plid[7];
  char word[31];
  int trial;
  char status[4];
  char *buffer;
  buffer = (char*) malloc(256 * sizeof(char));

  sscanf(message, "%s %s %s %d", code, plid, word, &trial);

  if (verbose == 1){
    printf("Player %s sent a guess: %s.\n", plid, word);
  }

  game_id *game_id = get_game(plid);
  if (game_id != NULL){
    game_data *game = game_id->game_data;
    if (trial != game->trial){
      if (trial == game->trial-1 && strcmp(game->guesses->word, word) == 0){
        return NULL; // RESEND LAST GUESS RESPONSE
      }
      else{
        strcpy(status, INV);
        sprintf(buffer, "%s %s %d\n", RWG, status, game->trial);
      }
    }
    else if (word_played(word, game->guesses) == 1){
      strcpy(status, DUP);
      sprintf(buffer, "%s %s %d\n", RWG, status, game->trial);
    }

    else{

      if (strcmp(game->word, word) == 0){
        strcpy(status, WIN);
        sprintf(buffer, "%s %s %d\n", RWG, status, game->trial++);
        game->letters_guessed = strlen(game->word); // TODO
        strcpy(game->word_status, game->word);
        update_game_status(game_id, word);
        // archive_game(game_id);
      }
      else{
        game->errors++;

        if (game->errors == game->max_errors){
          strcpy(status, OVR);
          // archive_game(game_id);
        }

        else{
          strcpy(status, NOK);
        }

        sprintf(buffer, "%s %s %d\n", RWG, status, game->trial++);
      }

      strcpy(game->guesses->word, word);
      // add word to guessed_words and link them
      strcpy(game->guesses->word, word);
      guessed_word *next_word = (guessed_word*) malloc(sizeof(guessed_word));
      next_word->word = (char*) malloc(256 * sizeof(char));
      next_word->prev = game->guesses;
      game->guesses = next_word;

      update_game_status(game_id, word);
    }
  }
  else{
    strcpy(status, ERR);
    sprintf(buffer, "%s %s\n", RWG, status);
  }
  return buffer;
  //TODO IMPLEMENT DUP CASE - CHOOSE BETWEEN STRUCT OR FILE
}

char* quit(char *message) {
  char code[4];
  char plid[7];
  char status[4];
  char* buffer;
  buffer = (char*) malloc(256 * sizeof(char));

  sscanf(message, "%s %s", code, plid);

  if (verbose == 1)
    printf("Player %s requested to quit the game.\n", plid);

  game_id *curr_game = get_game(plid);
  if (curr_game == NULL) { // NO ACTIVE GAMES FOUND FOR PLAYER
    printf("DEBUG: No active games found for player\n");

    strcpy(status, ERR);
  } else { // ACTIVE GAME FOUND
    state(curr_game);
    delete_game(curr_game); // DELETE GAME
    strcpy(status, OK);
  }

  sprintf(buffer, "%s %s\n", RQT, status);
  // send message to client
  return buffer;
}

// Returns the length of the word
int set_game_word(game_data *game_data) {
  char word[31];
  char file[64];
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
    fscanf(fp, "%s %s", word, file);
  }

  word_length = strlen(word);

  // set game data
  game_data->word = malloc((word_length + 1) * sizeof(char));
  game_data->file = malloc((strlen(file) + 1) * sizeof(char));
  game_data->guesses = malloc(sizeof(guessed_word));
  game_data->guesses->prev = NULL;
  game_data->guesses->word = malloc((256) * sizeof(char));

  strcpy(game_data->word, word);
  strcpy(game_data->file, file);
  
  if (word_length < 7)
    game_data->max_errors = 7;
  else if (word_length < 11)
    game_data->max_errors = 8;
  else
    game_data->max_errors = 9;

  game_data->errors = 0;
  game_data->trial = 1;
  game_data->letters_guessed = 0;
  strcpy(game_data->last_letter, "");
  strcpy(game_data->letters_played, "");

  for (int i = 0; i < word_length; i++) {
    game_data->word_status[i] = '-';
  }
  game_data->word_status[word_length] = '\0';

  fclose(fp);
  return word_length;
}

game_id *get_game(char *plid) {
  game_id *curr_game = games;

  while (curr_game != NULL) {
    if (strcmp(curr_game->plid, plid) == 0)
      return curr_game;
    curr_game = curr_game->next;
  }

  return NULL;
}

// Function which checks if letter has already been played
int letter_in_word(char *letters_guessed, char *letter) {
  int i;
  for (i = 0; i < strlen(letters_guessed); i++) {
    if (letters_guessed[i] == letter[0])
      return 1;
  }
  return 0;
}

// void archive_game(game_id* game) {
//   if (game->last_game != NULL) {
//     // Free everything in game_data
//     free(game->last_game->word);
//     free(game->last_game);
//   }
//   game->last_game = game->game_data;
//   game->game_data = NULL;
// }

void delete_game(game_id *game) {
  if (game->prev == NULL) // GAME IS FIRST IN LIST
    games = game->next;
  else
    game->prev->next = game->next;

  if (game->next != NULL) // GAME IS NOT LAST IN LIST
    game->next->prev = game->prev;

  // Free everything in game_data
  free(game->game_data->word);
  free(game->game_data);
  
  guessed_word *curr_word = game->game_data->guesses;
  guessed_word *prev_word = NULL;
  while (curr_word != NULL) {
    prev_word = curr_word;
    curr_word = curr_word->prev;
    free(prev_word->word);
    free(prev_word);
  }

  // Archiving twice game_data is the same as deleting
  // archive_game(game);
  // archive_game(game);
}

int word_played(char *word, guessed_word *guessed_words) {

  while (guessed_words != NULL) {
    if (strcmp(guessed_words->word, word) == 0)
      return 1;
    guessed_words = guessed_words->prev;
  }
  return 0;
}

/* --------------------------------------------------------------------------------------------------------------


                                                SERVER TCP FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

void update_game_status(game_id *game, char* attempt) {
  char filename[256];
  
  sprintf(filename, "GAMES/game_%s.txt", game->plid);

  FILE *fp = fopen(filename, "a");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  if (strlen(attempt) == 1) // Letter trial
    fprintf(fp, "Letter trial: %s\n", attempt);
  else // Word trial
    fprintf(fp, "Word guess: %s\n", attempt);

  fclose(fp);
}

/* Function which writes to file in directory GAMES and file game_plid.txt 
* the current state of the game.
*/
char* state(game_id *game) {
  char filename[256];
  char buffer[256];
  char* out;
  out = (char*)malloc(5024 * sizeof(char));

  sprintf(filename, "GAMES/game_%s.txt", game->plid);

  FILE *pl = fopen(filename, "r");
  if (pl == NULL) {
    perror("fopen");
    exit(1);
  }

  int n = 0;
  int ongoing_game = 1;
  if (strcmp(game->game_data->word, game->game_data->word_status) == 0
    || game->game_data->errors == game->game_data->max_errors)
    ongoing_game = 0;

  if (ongoing_game == 0) {
    n=sprintf(out, "Last finalized game for player %s\n", game->plid);
    n+=sprintf(out+n, "Word: %s Hint file: %s\n", game->game_data->word,
      game->game_data->file);
    n+=sprintf(out+n, "%d transactions found:\n", game->game_data->trial-1);
  } else {
    n+=sprintf(out+n, "Active game found for player %s\n", game->plid);
    if (game->game_data->trial == 1)
      n+=sprintf(out+n, "Game started - no transactions found\n");
    else
      n+=sprintf(out+n, "--- Transactions found: %d---\n", game->game_data->trial-1);
  }

  for (int i = 0; i < game->game_data->trial-1; i++) {
    fgets(buffer, 256, pl);
    n+=sprintf(out+n, "%s", buffer);
  }
  
  if (strcmp(game->game_data->word_status, game->game_data->word) == 0)
    n+=sprintf(out+n, "TERMINATION: WIN\n");
  else if (game->game_data->errors == game->game_data->max_errors)
    n+=sprintf(out+n, "TERMINATION: FAIL\n");
  else
    n+=sprintf(out+n, "Solved so far: %s\n", game->game_data->word_status);

  fclose(pl);
  return out;
}