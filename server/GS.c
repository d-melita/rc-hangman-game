#include "GS.h"

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
  struct game_id *prev;
  struct game_data *game_data;
} game_id;

typedef struct guessed_word {
  char *word;
  struct guessed_word *prev;
} guessed_word;

int table_size = DEFAULT_TABLE_SIZE;
int num_games = 0;
game_id** games; // array of game_id pointers

int tcp = 0;
struct addrinfo hints, *res;

char port[6];
char wordfile[256];
int verbose = 0; // 0 false, 1 true
int count = 1; // keep track of next word to assign

int main(int argc, char *argv[]) {
  parse_args(argc, argv);
  signal(SIGPIPE, handler);
  signal(SIGINT, handler);
  signal(SIGCHLD, SIG_IGN); // ignore child signals

  printf("Server started on port %s\n", port);
  if (verbose)
    puts(VERBOSE_ON);

  int pid = fork();

  if (pid == 0) {
    // Child process
    tcp = 1; // flag to ignore table deletion later
    
    message_tcp();
  } else if (pid > 0) {
    // Parent process
    games = malloc(sizeof(game_id*) * DEFAULT_TABLE_SIZE);
    if (games == NULL) {
      perror(ERR_MALLOC);
      exit(1);
    }
    memset(games, 0, sizeof(game_id*) * DEFAULT_TABLE_SIZE);

    message_udp();
  } else {
    // Error
    perror(ERR_FORK);
    exit(1);
  }

  return 0;
}

// Functions related to table

int hash(char* plid) {
  return atoi(plid) % table_size;
}

int set_game(char* plid) {
  int word_length = -1;
  int hash_value = hash(plid);
  game_id* curr = get_game(plid);
  game_id* prev = NULL;

  if (curr == NULL) {
    curr = (game_id*) malloc(sizeof(game_id));
    strcpy(curr->plid, plid);
    curr->prev = games[hash_value];
  }

  word_length = set_game_data(curr);

  if (num_games >= (table_size/2)) {
    if (resize_table() != 0)
      return 1;
  }

  games[hash_value] = curr;

  num_games++;
  return 0;
}

int resize_table() { // TODO TEST IF WORKS
  game_id** new_table = malloc(sizeof(game_id*) * table_size * 2);
  if (new_table == NULL) {
    perror(ERR_MALLOC);
    return 1;
  }
  memset(new_table, 0, sizeof(game_id*) * table_size * 2); 

  int old_size = table_size;
  table_size *= 2;
  for (int i = 0; i < old_size; i++) {
    game_id* curr = games[i]; // TODO
    game_id* prev = NULL;
    while (curr != NULL) {
      int new_hash = hash(curr->plid);
      game_id* new_game = new_table[new_hash]; // get the game at new hash
      prev = curr->prev; // save pointer next pointer
      curr->prev = new_game; // join the new collision chain
      new_table[new_hash] = curr; // set it atop
      curr = prev; // go to next
    }
  }

  free(games);
  games = new_table;
  return 0;
}

int delete_table() {
  for (int i = 0; i < table_size; i++) {
    game_id* curr = games[i];
    game_id* last = NULL;
    while (curr != NULL) {
      last = curr;
      curr = curr->prev;
      delete_game(last);
      free(last);
    }
  }
  free(games);
  return 0;
}

// Parse the arguments given to the program (host and port)
void parse_args(int argc, char *argv[]) {
  // Two arguments, -v and -p
  strcpy(port, DEFAULT_PORT); // TODO word file parameter

  if (argc < 1 || (access(argv[1], F_OK) == -1) || (access(argv[1], R_OK) == -1)) {
    puts(ERR_INVALID_WORDFILE);
    exit(1);
  } else {
    strcpy(wordfile, argv[1]);
  }

  for (int i = 2; i < argc; i++) { // Parse each given option and argument

    if (strcmp(argv[i], "-v") == 0) { // Verbose Option (-v)
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
      puts(ERR_INVALID_OPTION);
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
  struct sockaddr_in addr;
  char buf[256];
  char *response;
  int num = 0;

  fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
  if (fd == -1) {
    perror(ERR_SOCKET);
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
    perror(ERR_BIND);
    exit(1);
  }

  while (1) {
    addrlen = sizeof(addr);
    n = recvfrom(fd, buf, 256, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
      perror(ERR_RECVFROM);
      exit(1);
    }

    buf[n] = '\0';

    if (verbose == 1) {
      puts(UDP_RECV);
      printf("CLIENT IP: %s | PORT: %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    }

    response = parse_message_udp(buf);
    if (response != NULL) { // No problems encountered, send response
      n = sendto(fd, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);
      if (n == -1) {
        perror(ERR_SENDTO);
        exit(1);
      }
      
      if (verbose == 1) {
        puts("--------------------"); // just to separate the messages
      }

      free(response);
    }
  }
  close(fd);

  freeaddrinfo(res);
}

// function to parse the message received from the client
char* parse_message_udp(char *message) {
  char code[4];
  sscanf(message, "%3s", code);

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
    char* response = (char*) malloc(6 * sizeof(char));
    sprintf(response, "%s\n", ERR);
    return response;
  }
}

char* start_new_game(char *message) {
  char code[4];
  char plid[7];
  char status[4];
  int word_length;
  char* response;
  response = (char*) malloc(256 * sizeof(char));

  sscanf(message, "%3s %6s", code, plid);

  if (clear_input(message) != 0) {
    sprintf(response, "%s %s\n", SNG, ERR);
    return response;
  }

  if (verbose == 1)
    printf("[%s] Player %s requested a new game.\n", code, plid);

  game_id *curr_game = get_game(plid);  
  if (curr_game == NULL || curr_game->game_data == NULL) { // PLAYER HAS NO ONGOING GAME
    if (set_game(plid) != 0)
      exit(1);

    curr_game = get_game(plid);
    word_length = strlen(curr_game->game_data->word);

    strcpy(status, OK);

    sprintf(response, "%s %s %d %d\n", RSG, status, word_length, MAX_ERRORS);

    update_game_status(curr_game, NULL, ACT);
    if (verbose == 1)
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

  sscanf(message, "%s %6s %1s %d\n", code, plid, letter, &trial);
  letter[0] = toupper(letter[0]);
  if (clear_input(message) != 0) {
    strcpy(status, ERR);
    sprintf(buffer, "%s %s\n", RLG, status);
    return buffer;
  }
  
  if (verbose == 1)
    printf("[%s] Player %s requested to play letter %s.\n", code, plid, letter);

  game_id *game_id = get_game(plid);
  if (game_id != NULL && game_id->game_data != NULL) {          // 1: THERES AN ACTIVE GAME FOR PLAYER
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
    } else if (letter_in_word(game->letters_played, letter) == 1) {
      
      strcpy(status, DUP);
      sprintf(buffer, "%s %s %d\n", RLG, status, game->trial);

    } else {
      // 4: LETTER IS IN WORD -> OK
      if (letter_in_word(game->word, letter) == 1) {
        char pos_str[31] = "";
        char temp[5] = "";
        int ocurrences = 0;

        for (int i = 0; i < strlen(game->word); i++) {
          if (game->word[i] == letter[0]) {
            sprintf(temp, " %d", i+1);
            strcat(pos_str, temp);
            game->letters_guessed++;
            game->word_status[i] = letter[0];
            ocurrences++;
          }
        }

        // 4.1: WORD IS GUESSED -> WIN
        if (game->letters_guessed == strlen(game->word)) {
          strcpy(status, WIN);
          sprintf(buffer, "%s %s %d\n", RLG, status, game->trial);
          update_game_status(game_id, letter, WIN);
          store_game(game_id);
          delete_game(game_id);
        } else{
          update_game_status(game_id, letter, ACT);
          strcpy(status, OK);
          sprintf(buffer, "%s %s %d %d%s\n", RLG, status, game->trial++, ocurrences, pos_str);
        }

      // 5: LETTER IS NOT IN WORD -> NOK
      } else {
        game->errors++;

        // 5.1: MAX ERRORS REACHED -> OVR
        if (game->errors > game->max_errors) {
          strcpy(status, OVR);
          sprintf(buffer, "%s %s %d\n", RLG, status, game->trial);
          update_game_status(game_id, letter, OVR);
          store_game(game_id);
          delete_game(game_id);
        } else{ 
          update_game_status(game_id, letter, ACT);
          strcpy(status, NOK);
          sprintf(buffer, "%s %s %d\n", RLG, status, game->trial++);
        }  
      }
      strcpy(game->last_letter, letter);
      strcat(game->letters_played, letter); // add letter played to list
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

  sscanf(message, "%s %6s %30s %d", code, plid, word, &trial);
  // TODO do we format word later toupper?
  if (clear_input(message) != 0) {
    strcpy(status, ERR);
    sprintf(buffer, "%s %s\n", RWG, status);
    return buffer;
  }

  if (verbose == 1){
    printf("[%s] Player %s sent a guess: %s.\n", code, plid, word);
  }

  game_id *game_id = get_game(plid);
  if (game_id != NULL && game_id->game_data != NULL){
    game_data *game = game_id->game_data;
    
    if (trial != game->trial){
      if (trial == game->trial-1 && game->guesses->word != NULL
       && strcmp(game->guesses->word, word) == 0){
        return NULL; // RESEND LAST GUESS RESPONSE TODO
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
      // add word to guessed_words and link them
      game->guesses->word = (char*) malloc((strlen(word)+1) * sizeof(char));
      strcpy(game->guesses->word, word);
      guessed_word *next_word = (guessed_word*) malloc(sizeof(guessed_word));
      next_word->word = NULL;
      next_word->prev = game->guesses;
      game->guesses = next_word;

      if (strcmp(game->word, word) == 0){
        strcpy(status, WIN);
        sprintf(buffer, "%s %s %d\n", RWG, status, game->trial);

        game->letters_guessed = strlen(game->word); // TODO
        strcpy(game->word_status, game->word);
        update_game_status(game_id, word, WIN);
        store_game(game_id);
        delete_game(game_id);
      }
      else{
        game->errors++;

        if (game->errors > game->max_errors){ // GAMEOVER
          strcpy(status, OVR);
          update_game_status(game_id, word, OVR);
          sprintf(buffer, "%s %s %d\n", RWG, status, game->trial);
          store_game(game_id);
          delete_game(game_id);
        }
        else{ // FAIL BUT NOT GAMEOVER
          strcpy(status, NOK);
          sprintf(buffer, "%s %s %d\n", RWG, status, game->trial++);
          update_game_status(game_id, word, ACT);
        }
      }
    }
  }
  else{
    strcpy(status, ERR);
    sprintf(buffer, "%s %s\n", RWG, status);
  }
  return buffer;
}

char* quit(char *message) {
  char code[4];
  char plid[7];
  char status[4];
  char* buffer;
  buffer = (char*) malloc(256 * sizeof(char));

  sscanf(message, "%s %6s", code, plid);

  if (get_game(plid) == NULL || clear_input(message) != 0) { // TODO || 
    strcpy(status, ERR);
    sprintf(buffer, "%s %s\n", RQT, status);
    return buffer;
  }

  if (verbose == 1)
    printf("[%s] Player %s requested to quit the game.\n", code, plid);

  game_id *curr_game = get_game(plid);
  // ACTIVE GAME FOUND
  delete_game(curr_game); // DELETE GAME
  strcpy(status, OK);

  sprintf(buffer, "%s %s\n", RQT, status);
  // send message to client
  // store game in scoreboard
  return buffer;
}

/* --------------------------------------------------------------------------------------------------------------


                                    ADD GAME TO SCOREBOARD


   --------------------------------------------------------------------------------------------------------------
 */

void store_game(game_id *curr_game){
  char line[256];
  int trials = (curr_game->game_data->trial);

  int successfull_trials = trials - curr_game->game_data->errors;
  int score = (int)((successfull_trials / (float)trials) * 100.0);

  FILE *fp;
  if (access(SCOREBOARD, F_OK) == -1) {
    fp = fopen(SCOREBOARD, "w+"); // Create SB file if it doesn't exist
  } else
    fp = fopen(SCOREBOARD, "r"); // Read existing scoreboard

  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  flock(fileno(fp), LOCK_EX);

  char temp_filename[64];
  sprintf(temp_filename, "%d%s", getpid(), TEMP);
  FILE* temp = fopen(temp_filename, "w"); // Create temporary auxiliary file
  if (temp == NULL) {
    perror("fopen");
    exit(1);
  }

  int done = 0;
  int counter = 0;
  
  char plid[7];
  int temp_score;
  int temp_trials;
  int temp_succ_trials;
  char temp_word[31];
  while (fgets(line, 256, fp) != NULL && counter < 11){
    sscanf(line, "%d %s %d %d %s", &temp_score, plid, &temp_succ_trials, &temp_trials, temp_word);
    if (done == 0) { // try to add new score
      // If the new score should steal the spot
      if (temp_score < score || (temp_score == score && (temp_succ_trials < successfull_trials 
       || (temp_succ_trials == successfull_trials)))){
        add_scoreboard_line(temp, curr_game);
        counter++;
        done++;
      } 
    }
    if (counter+1 < 11) { // if old score can still fit in
      fprintf(temp, "%s", line);
      ++counter;
    }
  }
  if (done == 0 && counter < 11) // If all old scores were better, but theres still room
    add_scoreboard_line(temp, curr_game);

  // Reset file pointers to the beginning of the files (and change modes)
  fclose(temp);
  temp = fopen(temp_filename, "r");
  if (temp == NULL) {
    perror("fopen");
    exit(1);
  }
  fclose(fp);
  
  fp = fopen(SCOREBOARD, "w");
  if (temp == NULL) {
    perror("fopen");
    exit(1);
  }

  // Copy the lines from temp.txt to scoreboard.txt
  while (fgets(line, 256, temp) != NULL) {
    fprintf(fp, "%s", line);
  }
  flock(fileno(fp), LOCK_UN);
  fclose(fp);
  fclose(temp);

  remove(temp_filename); // delete auxiliary file
}

void add_scoreboard_line(FILE* fp, struct game_id *curr_game){

  int trials = (curr_game->game_data->trial);

  int successfull_trials = trials - curr_game->game_data->errors;
  int score = (int)((successfull_trials / (float)trials) * 100.0);

  if (curr_game->game_data->letters_guessed == 0
   && curr_game->game_data->guesses->word == NULL){
    score = 0;
  }

  fprintf(fp, "%d %s %d %d %s\n", score, curr_game->plid, successfull_trials, curr_game->game_data->trial, curr_game->game_data->word);
}

/* --------------------------------------------------------------------------------------------------------------


                                  SERVER UDP AUXILIAR FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

// Returns the length of the word
int set_game_data(game_id * game_id) {
  char word[31];
  char file[64];
  int i;
  int word_length;
  int random_number;

  FILE *fp = fopen(wordfile, "r");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  // // get random number
  // srand(time(0));
  // random_number = rand() % SIZE_WORDFILE; 
  // // dont forget to update wordfile size

  // // get word from file
  // for (i = 0; i <= random_number; i++) {
  //   fscanf(fp, "%s %s", word, file);
  // }

  // Get number of lines in file
  int size = 0;
  while (fscanf(fp, "%s %s", word, file) != EOF) {
    size++;
  }
  fseek(fp, 0, SEEK_SET);


  for (i = 1; i <= count; i++) {
    fscanf(fp, "%s %s", word, file);
  }
  count++;
  if (count > size)
    count = 1;

  for (i = 0; i < strlen(word); i++) {
    word[i] = toupper(word[i]);
  }

  word_length = strlen(word);

  game_data* game_data = (struct game_data*) malloc(sizeof(struct game_data));
  game_id->game_data = game_data;

  // set game data
  game_data->word = malloc((word_length + 1) * sizeof(char));
  game_data->file = malloc((strlen(file) + 1) * sizeof(char));
  game_data->guesses = malloc(sizeof(guessed_word));
  game_data->guesses->word = NULL;
  game_data->guesses->prev = NULL;

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
  game_id *curr_game = games[hash(plid)];
  while (curr_game != NULL) {
    if (strcmp(curr_game->plid, plid) == 0)
      return curr_game;
    curr_game = curr_game->prev;
  }
  return NULL;
}

// Function which checks if letter has already been played
int letter_in_word(char *letters_guessed, char *letter) {
  for (int i = 0; i < strlen(letters_guessed); i++) {
    if (letters_guessed[i] == letter[0])
      return 1;
  }
  return 0;
}

void delete_game(game_id *game) {

  // Free everything in game_data
  if (game->game_data != NULL) {
    free(game->game_data->word);
    free(game->game_data->file);

    guessed_word* curr_word = game->game_data->guesses;
    guessed_word* last_word = NULL;
    while(curr_word != NULL) {
      last_word = curr_word;
      curr_word = curr_word->prev;
      free(last_word->word);
      free(last_word);
    }

    free(game->game_data);

    game->game_data = NULL;
  }
}

int word_played(char *word, guessed_word *guessed_words) {

  while (guessed_words != NULL) {
    if (guessed_words->word != NULL)
      if (strcmp(guessed_words->word, word) == 0)
        return 1;
    guessed_words = guessed_words->prev;
  }
  return 0;
}

/* --------------------------------------------------------------------------------------------------------------


                                    UPDATE GAME STATUS


   --------------------------------------------------------------------------------------------------------------
 */

void update_game_status(game_id *game, char* attempt, char* status) {
  char filename[256];
  char st_filename[256];

  check_gamesFolder();

  sprintf(filename, GAMES, game->plid);
  sprintf(st_filename, GAMES_STATUS, game->plid);

  // Write current game status, word and word file to gamestatus file
  FILE *fp = fopen(st_filename, "w");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  fprintf(fp, "%s %s %s %s\n", status, game->game_data->word, 
          game->game_data->file, game->game_data->word_status);
  fclose(fp);

  if (attempt != NULL) {
    fp = fopen(filename, "a");
    if (fp == NULL) {
      perror("fopen");
      exit(1);
    }

    if (strlen(attempt) == 1) // Letter trial
      fprintf(fp, "%s%s\n", LETTER_TRIAL, attempt);
    else // Word trial
      fprintf(fp, "%s%s\n", WORD_GUESS,attempt);
  } else {
    fp = fopen(filename, "w");
    if (fp == NULL) {
      perror("fopen");
      exit(1);
    }
  }
  fclose(fp);
}

/* --------------------------------------------------------------------------------------------------------------


                                                SERVER TCP FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

void message_tcp() {
  int fd, newfd, errcode;
  ssize_t n;
  socklen_t addrlen;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
  if (fd == -1) {
    perror(ERR_SOCKET);
    exit(1);
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  errcode = getaddrinfo(NULL, port, &hints, &res);
  if (errcode != 0) {
    perror(ERR_GETADDRINFO);
    exit(1);
  }

  n = bind(fd, res->ai_addr, res->ai_addrlen);
  if (n == -1) {
    perror(ERR_BIND);
    exit(1);
  }

  if (listen(fd, 5) == -1) {
    perror(ERR_LISTEN);
    exit(1);
  }

  while (1){
    addrlen = sizeof(addr);

    if ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) {
      perror(ERR_ACCEPT);
      exit(1);
    }
    setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    pid_t pid = fork();
    if (pid == -1) {
      perror(ERR_FORK);
      exit(1);
    }
    if (pid == 0) { // Child process
      puts(CONN_ACCP);

      char buffer[256];

      n = read(newfd, buffer, 256);
      if (n == -1) {
        perror(ERR_READ);
        exit(1);
      }
      buffer[n] = 0;

      if (verbose == 1) {
        puts(TCP_RECV);
        printf("CLIENT IP: %s | PORT: %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
      }

      parse_message_tcp(buffer, newfd);

      if (verbose == 1) {
        puts("--------------------"); // just to separate the messages
      }

      freeaddrinfo(res);
      close(newfd);
      return;
    }
  }

  close(fd);
  freeaddrinfo(res);
}

void parse_message_tcp(char *message, int fd){
  char code[4];
  char plid[7];

  sscanf(message, "%s", code);

  if (strcmp(code, GSB) == 0){
    if (clear_input(message) != 0){
      char* response = (char*) malloc(9 * sizeof(char));
      sprintf(response, "%s %s\n", code, ERR);
      send_message_tcp(fd, response);
      free(response);
      return;
    }
    get_scoreboard(fd);
  }

  else if(strcmp(code, GHL) == 0){
    if (clear_input(message) != 0){
      char* response = (char*) malloc(9 * sizeof(char));
      sprintf(response, "%s %s\n", code, ERR);
      send_message_tcp(fd, response);
      free(response);
      return;
    }
    sscanf(message, "%s %s", code, plid);
    get_hint(fd, plid);
  }

  else if(strcmp(code, STA) == 0){
    if (clear_input(message) != 0){
      char* response = (char*) malloc(9 * sizeof(char));
      sprintf(response, "%s %s\n", code, ERR);
      send_message_tcp(fd, response);
      free(response);
      return;
    }
    sscanf(message, "%s %s", code, plid);
    state(plid, fd);
  }

  else{
    char* response = (char*) malloc(6 * sizeof(char));
    sprintf(response, "%s\n", ERR);
    send_message_tcp(fd, response);
    free(response);
  }
}

/* Function which writes to file in directory GAMES and file game_plid.txt 
* the current state of the game.
*/
char* state(char* plid, int fd) {
  char filename[256]; // File with game history
  char st_filename[256]; // File with game status
  char trans_buffer[5024] = ""; // Temporary buffer for all the attempts taken
  char temp[256];
  char* reply = (char*)malloc(5024 * sizeof(char));
  char* file = (char*)malloc(5024 * sizeof(char));

  if (verbose == 1){
    printf("Player %s requested game state\n", plid);
  }

  check_gamesFolder();

  sprintf(st_filename, GAMES_STATUS, plid);
  // Check if file exists 
  if (access(st_filename, F_OK) == -1) {
    sprintf(reply, "%s %s\n", RST, NOK);
    return reply;
  }
  FILE *fp = fopen(st_filename, "r"); // Open file with game status
  if (fp == NULL) {
    perror(ERR_FOPEN);
    exit(1);
  }

  char game_status[6];
  char word[31];
  char word_file[64];
  char word_status[31];
  if (fscanf(fp, "%s %s %s %s", game_status, word, word_file, word_status) == EOF) {
    perror(ERR_FSCANF);
    exit(1);
  }
  fclose(fp);

  if (strcmp(game_status, OVR) == 0) // ask the teachers about this lmfao
    strcpy(game_status, FAIL);

  sprintf(filename, GAMES, plid);
  fp = fopen(filename, "r"); // Open file with plays
  if (fp == NULL) {
    perror(ERR_FOPEN);
    exit(1);
  } 

  int trials = 0;
  // Read every line from filename and append them to transbuffer, while increasing trials
  while (fgets(temp, 256, fp) != NULL) {
    trials++;
    strcat(trans_buffer, temp);
  }

  int ongoing_game = 1;
  if (strcmp(game_status, WIN) == 0 || strcmp(game_status, OVR) == 0)
    ongoing_game = 0;

  int n = 0;
  if (ongoing_game == 0) {
    n=sprintf(file, LAST_FIN_GAME, plid);
    n+=sprintf(file+n, WORD_HINT, word, word_file);
    n+=sprintf(file+n, TRANSACTIONS, trials);
  } else {
    n+=sprintf(file+n, ACTIVE_GAME, plid);
    if (trials == 0) // no transaction found in file
      n+=sprintf(file+n, GAME_STARTED);
    else
      n+=sprintf(file+n, TRANSACTIONS_FOUND, trials);
  }
  
  if (trials > 0)
    n+=sprintf(file+n, "%s", trans_buffer);

  sprintf(filename, STATE, plid);
  if (ongoing_game == 0) {
    n+=sprintf(file+n, TERMINATION, game_status);
    sprintf(reply, "%s %s %s %ld ", RST, FIN, filename, strlen(file));
  } else {
    n+=sprintf(file+n, SOLVED_SO_FAR, word_status);
    sprintf(reply, "%s %s %s %ld ", RST, ACT, filename, strlen(file));
  }

  send_message_tcp(fd, reply);
  send_file(fd, file, strlen(file));
  free(file);
  free(reply);
  fclose(fp);
}

void get_hint(int fd, char* plid){
  char status[4];
  char st_filename[64];
  char *reply;

  if (verbose == 1){
    printf("Player %s requested hint\n", plid);
  }

  check_gamesFolder();

  sprintf(st_filename, GAMES_STATUS, plid);

  FILE *fp = fopen(st_filename, "r"); // Open file with game status
  if (fp == NULL) {
    perror(ERR_FOPEN);
    exit(1);
  }

  char game_status[6];
  char word[31];
  char word_file[64];
  char word_status[31];
  if (fscanf(fp, "%s %s %s %s", game_status, word, word_file, word_status) == EOF) {
    perror(ERR_FSCANF);
    exit(1);
  }
  fclose(fp);

  // get file with word_file name

  char *filename = (char*) malloc(strlen(WORDS) + strlen(word_file) + 1);
  sprintf(filename, "%s%s", WORDS, word_file);

  fp = fopen(filename, "r");
  if (access(filename, F_OK) == -1 || access(filename, R_OK) == -1) {
    puts(ERR_FILE_DNE);
    reply = (char*)malloc(strlen(RHL) + 1 + strlen(NOK) + 2);
    sprintf(reply, "%s %s\n", RHL, NOK);
    send_message_tcp(fd, reply);
    free(reply);
    return;
  }

  // get filename size
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);

  // go back to the beginning of the file
  fseek(fp, 0, SEEK_SET);

  // get file content
  char *file = (char*)malloc(fsize + 1);
  if (fread(file, fsize, 1, fp) < 1) {
    perror(ERR_FREAD);
    exit(1);
  }

  fclose(fp);

  reply = (char*)malloc(strlen(RHL) + strlen(OK) + strlen(word_file) + fsize +5);
  sprintf(reply, "%s %s %s %ld ", RHL, OK, word_file, fsize);

  send_message_tcp(fd, reply);
  send_file(fd, file, fsize);
  send_message_tcp(fd, "\n");
  free(reply);
  free(file);
  free(filename);
}

void get_scoreboard(int fd){
  char *reply;
  char *filename = (char*) malloc(strlen(SCOREBOARD) + 1);

  if (verbose == 1)
    puts(SB_REQUEST);

  strcpy(filename, SCOREBOARD);

  FILE *fp = fopen(filename, "r");
  long fsize = 0;

  if (fp != NULL && access(filename, F_OK) == 0) {
    // Acquire a read lock on the file
    flock(fileno(fp), LOCK_SH);
    
    // go to the end of the file to know the size
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    // go back to the beginning of the file
    fseek(fp, 0, SEEK_SET);
  } // TODO : if R_OK is false, send NOK??

  if (fsize == 0) { // if file is empty or doesn't exist
    reply = (char*)malloc(strlen(RSB) + strlen(EMPTY) + 3);
    sprintf(reply, "%s %s\n", RSB, EMPTY);
    send_message_tcp(fd, reply);
    free(filename);
    free(reply);
    fclose(fp);
    return;
  }

  char temp_filename[64];
  sprintf(temp_filename, "%d%s", getpid(), TEMP);
  FILE *temp = fopen(temp_filename, "w+");

  int score, strials, trials, i = 1;
  char plid[7];
  char word[31];

  fprintf(temp, "%s", SCOREBOARD_HEADER);
  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    sscanf(line, "%d %s %d %d %s", &score, plid, &strials, &trials, word);
    fprintf(temp, "%d-  %d    %s            %d                %d          %s\n", i, score, plid, strials, trials, word);
    i++;
  }
  // Release the lock
  flock(fileno(fp), LOCK_UN);

  fsize = ftell(temp);
  fseek(temp, 0, SEEK_SET);

  // get file content
  char *file = (char*)malloc(fsize + 1);
  if (fread(file, fsize, 1, temp) < 1) {
    perror(ERR_FREAD);
    exit(1);
  }

  reply = (char*)malloc(strlen(RHL) + strlen(OK) + strlen(filename) + fsize + 2);
  sprintf(reply, "%s %s %s %ld ", RSB, OK, filename, fsize);

  send_message_tcp(fd, reply);
  send_file(fd, file, fsize);
  send_message_tcp(fd, "\n");
  
  free(reply);
  free(file);
  free(filename);
  fclose(fp);
  fclose(temp);
  remove(temp_filename);
}

/* --------------------------------------------------------------------------------------------------------------


                                  SERVER AUXILIAR TCP FUNCTIONS


   --------------------------------------------------------------------------------------------------------------
 */

void send_message_tcp(int fd, char* message){
  ssize_t n = write(fd, message, strlen(message));
  while (n < strlen(message)) {
    n += write(fd, message+n, strlen(message)-n);
  }
}

void send_file(int fd, char* file, int fsize){
  ssize_t n = write(fd, file, fsize);
  while (n < fsize) {
    n += write(fd, file+n, fsize-n);
  }
}

static void handler(int signum) {
  switch (signum) {
  case SIGINT:
    puts(SERVER_SD);
    if (tcp == 0)
      delete_table();

    freeaddrinfo(res);
    exit(0);
  default:
    puts(IG_SIG);
  }
}

void check_gamesFolder(){
  DIR *dir = opendir(GAMES_DIR);
  if (dir) {
    closedir(dir);
  } else if (ENOENT == errno) {
    mkdir(GAMES_DIR, 0700);
  } else {
    perror(ERR_OPENDIR);
    exit(1);
  }
}

// Verifies request follows protocol and is valid
int clear_input(char *message) {
  // 0 = clear input
  // 1 = error
  int i = 0;
  char code[4];
  char plid[7];

  sscanf(&message[i], "%3s", code);
  i += strlen(code);

  // messages like CODE\n actually
  if (strcmp(code, GSB) == 0) {
    if (message[i] == '\n' && message[i+1] == '\0')
      return 0;
    return 1;
  }

  // messages like "CODE PLID ?\n"
  if (message[i] != ' ' || message[++i] == ' ')
    return 1;

  sscanf(&message[i], "%6s", plid);
  i += strlen(plid);

  if (is_number(plid) == 0 || strlen(plid) != 6 
   || message[++i] == ' ')
    return 1;
    
  // messages like "CODE PLID x TRIAL\n"
  if (strcmp(code, PLG) == 0 || strcmp(code, PWG) == 0){
    if (message[i-1] != ' ') return 1;

    char word[31];
    int trial;
    if (strcmp(code, PLG) == 0){
      word[0] = message[i++]; //A 1\n\0
      word[1] = '\0';
    }
    else {
      sscanf(&message[i], "%30s", word);
      i += strlen(word);
    }
    if (is_word(word) == 0 || message[i++] != ' ')
      return 1;

    sscanf(&message[i], "%d", &trial);
    i += strlen(&message[i]);
  }

  if (message[i-1] != '\n' || message[i] != '\0')
    return 1;
  return 0;
}

int is_word(char* buf) {
  int i = 0;
  for (i = 0; i < strlen(buf); i++) {
    char c = buf[i];
    // check if char is a letter
    if ((c < 'A' || c > 'Z') &&
     (c < 'a' || c > 'z')) {
      return 0;
    }
  }
  return 1;
}

int is_number(char* buf) {
  int i = 0;
  for (i = 0; i < strlen(buf); i++) {
    char c = buf[i];
    // check if char is a number
    if ((c < '0' || c > '9')) {
      return 0;
    }
  }
  return 1;
}

// TODO clean up code
// TODO Comments here and in header files
// TODO format everything nicely with clang
// TODO make single makefile in project root folder
// TODO move these functions to a shared file between client and server
// TODO rever timeouts e etc