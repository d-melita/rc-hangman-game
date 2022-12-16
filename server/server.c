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

  int pid = fork();

  if (pid == 0) {
    // Child process
    message_tcp();
  } else if (pid > 0) {
    // Parent process
    message_udp();
  } else {
    // Error
    perror("fork");
    exit(1);
  }

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
    if (curr_game == NULL) {
      curr_game = malloc(sizeof(game_id));
      strcpy(curr_game->plid, plid);
      curr_game->next = games;
    }
    curr_game->game_data = malloc(sizeof(game_data));

    if (games != NULL) // Set new game at the start of the list
      games->prev = curr_game;
    games = curr_game;

    word_length = set_game_word(curr_game->game_data);
    strcpy(status, OK);

    sprintf(response, "%s %s %d %d\n", RSG, status, word_length, MAX_ERRORS);

    update_game_status(curr_game, NULL, ACT);
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
          update_game_status(game_id, letter, WIN);
          delete_game(game_id);
          store_game(game_id);
        } else
          update_game_status(game_id, letter, ACT);

        sprintf(buffer, "%s %s %d %d%s\n", RLG, status, game->trial++, n, pos_str);

      // 5: LETTER IS NOT IN WORD -> NOK
      } else {
        strcpy(status, NOK);
        game->errors++;

        // 5.1: MAX ERRORS REACHED -> OVR
        if (game->errors == game->max_errors) {
          strcpy(status, OVR);
          update_game_status(game_id, letter, OVR);
          delete_game(game_id);
          store_game(game_id);
        } else 
          update_game_status(game_id, letter, ACT);

        sprintf(buffer, "%s %s %d\n", RLG, status, game->trial++);
      }
      strcpy(game->last_letter, letter);
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
      // add word to guessed_words and link them
      game->guesses->word = (char*) malloc((strlen(word)+1) * sizeof(char));
      strcpy(game->guesses->word, word);
      guessed_word *next_word = (guessed_word*) malloc(sizeof(guessed_word));
      next_word->word = NULL;
      next_word->prev = game->guesses;
      game->guesses = next_word;

      if (strcmp(game->word, word) == 0){
        strcpy(status, WIN);
        sprintf(buffer, "%s %s %d\n", RWG, status, game->trial++);

        game->letters_guessed = strlen(game->word); // TODO
        strcpy(game->word_status, game->word);
        update_game_status(game_id, word, WIN);
        store_game(game_id);
        delete_game(game_id);
      }
      else{
        game->errors++;

        if (game->errors == game->max_errors){ // GAMEOVER
          strcpy(status, OVR);
          update_game_status(game_id, word, OVR);
          store_game(game_id);
          delete_game(game_id);
        }
        else{ // FAIL BUT NOT GAMEOVER
          strcpy(status, NOK);
          update_game_status(game_id, word, ACT);
        }
        sprintf(buffer, "%s %s %d\n", RWG, status, game->trial++);
      }
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
    delete_game(curr_game); // DELETE GAME
    strcpy(status, OK);
  }

  sprintf(buffer, "%s %s\n", RQT, status);
  // send message to client
  // store game in scoreboard
  store_game(curr_game);
  return buffer;
}

void store_game(game_id *curr_game){
  int score;
  char line[256];
  int counter;

  puts("DEBUG: Storing game in scoreboard");
  int successfull_trials = curr_game->game_data->trial - curr_game->game_data->errors;
  score = successfull_trials / curr_game->game_data->trial * 100;

  FILE *fp = fopen(SCOREBOARD, "r");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  puts("DEBUG: Reading scoreboard");
  fgets(line, 256, fp); // skip first line

  while (fgets(line, 256, fp) != NULL || counter < 10){
    char plid[7];
    int temp_score;
    int temp_trials;
    int temp_succ_trials;
    char temp_word[31];
    puts("DEBUG: Reading scoreboard line");
    if (fgets(line, 256, fp) != NULL ){
      puts("DEBUG: Reading scoreboard line1");
      counter++;
      sscanf(line, "%s %d %d %d %s", plid, &temp_score, &temp_trials, &temp_succ_trials, temp_word);
      if (temp_score < score){
        //update scoreboard
        break;
      }

      else if (temp_score == score){
        if (temp_succ_trials < successfull_trials){
          //update scoreboard
          break;
        }
        else if (temp_succ_trials == successfull_trials){
          if (temp_trials > curr_game->game_data->trial){
            //update scoreboard
            break;
          }
        }
      }
    }
    else{
      puts("DEBUG: Reading scoreboard line2");
      // add new line to scoreboard at the end
      add_scoreboard_line(curr_game);
      break;
    }
  }
}

void add_scoreboard_line(struct game_id *curr_game){
  int score, successfull_trials;
  puts("DEBUG: Adding new line to scoreboarddddddddddddd");
  FILE *fp = fopen(SCOREBOARD, "a");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  successfull_trials = curr_game->game_data->trial - curr_game->game_data->errors;

  printf("DEBUG: %d %d ST: %d\n", curr_game->game_data->trial, curr_game->game_data->errors, successfull_trials);

  score = ((successfull_trials*100) / curr_game->game_data->trial);

  puts("DEBUG: Adding new line to scoreboard");

  printf("%d\n", score);

  fprintf(fp, "%d %s %d %d %s\n", score, curr_game->plid, successfull_trials, curr_game->game_data->trial, curr_game->game_data->word);
  fclose(fp);
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
  random_number = rand() % SIZE_WORDFILE; 
  // dont forget to update wordfile size

  // get word from file
  for (i = 0; i <= random_number; i++) {
    fscanf(fp, "%s %s", word, file);
  }

  word_length = strlen(word);

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

void delete_game(game_id *game) {
  // Free everything in game_data
  free(game->game_data->word);
  free(game->game_data->file);

  guessed_word* curr_word = game->game_data->guesses;
  guessed_word* last_word = NULL;
  while(curr_word->word != NULL) {
    last_word = curr_word;
    curr_word = curr_word->prev;
    free(last_word->word);
    free(last_word);
  }

  free(game->game_data->guesses);
  free(game->game_data);

  game->game_data = NULL;
}

int word_played(char *word, guessed_word *guessed_words) {

  while (guessed_words->word != NULL) {
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

void update_game_status(game_id *game, char* attempt, char* status) {
  char filename[256];
  char st_filename[256];

  sprintf(filename, "GAMES/GAME_%s.txt", game->plid);
  sprintf(st_filename, "GAMES/STATUS_%s.txt", game->plid);

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
      fprintf(fp, "Letter trial: %s\n", attempt);
    else // Word trial
      fprintf(fp, "Word guess: %s\n", attempt);
  } else {
    fp = fopen(filename, "w");
    if (fp == NULL) {
      perror("fopen");
      exit(1);
    }
  }
  fclose(fp);
}

/* Function which writes to file in directory GAMES and file game_plid.txt 
* the current state of the game.
*/
char* state(char* plid, int fd) {
  char filename[256]; // File with game history
  char st_filename[256]; // File with game status
  char trans_buffer[5024]; // Temporary buffer for all the attempts taken
  char* reply = (char*)malloc(5024 * sizeof(char));
  char* file = (char*)malloc(5024 * sizeof(char));

  sprintf(st_filename, "GAMES/STATUS_%s.txt", plid);
  // Check if file exists 
  if (access(st_filename, F_OK) == -1) {
    puts("File does not exist");
    sprintf(reply, "%s %s\n", RST, NOK);
    return reply;
  }
  FILE *fp = fopen(st_filename, "r"); // Open file with game status
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  char game_status[6];
  char word[31];
  char word_file[64];
  char word_status[31];
  fscanf(fp, "%s %s %s %s", game_status, word, word_file, word_status);
  fclose(fp);

  if (strcmp(game_status, OVR) == 0) // ask the teachers about this lmfao
    strcpy(game_status, "FAIL");


  sprintf(filename, "GAMES/GAME_%s.txt", plid);
  fp = fopen(filename, "r"); // Open file with plays
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  } 

  int trials = 0;
  // Read every line from filename and append them to transbuffer, while increasing trials
  while (fgets(trans_buffer+strlen(trans_buffer), 5024, fp) != NULL) {
    trials++;
  }

  int ongoing_game = 1;
  if (strcmp(game_status, WIN) == 0 || strcmp(game_status, OVR) == 0)
    ongoing_game = 0;

  int n = 0;
  if (ongoing_game == 0) {
    n=sprintf(file, "Last finalized game for player %s\n", plid);
    n+=sprintf(file+n, "Word: %s Hint file: %s\n", word, word_file);
    n+=sprintf(file+n, "%d transactions found:\n", trials);
  } else {
    n+=sprintf(file+n, "Active game found for player %s\n", plid);
    if (trials == 0) // no transaction found in file
      n+=sprintf(file+n, "Game started - no transactions found\n");
    else
      n+=sprintf(file+n, "--- Transactions found: %d---\n", trials);
  }
  
  if (trials > 0)
    n+=sprintf(file+n, "%s", trans_buffer);

  sprintf(filename, "STATE_%s.txt", plid);
  if (ongoing_game == 0) {
    n+=sprintf(file+n, "TERMINATION: %s\n", game_status);
    sprintf(reply, "%s %s %s %ld ", RST, FIN, filename, strlen(file));
  } else {
    n+=sprintf(file+n, "Solved so far: %s\n", word_status);
    sprintf(reply, "%s %s %s %ld ", RST, ACT, filename, strlen(file));
  }

  send_message_tcp(fd, reply);
  send_file(fd, file, strlen(file));
  free(file);
  fclose(fp);
}

void message_tcp() {
  int fd, newfd, errcode;
  ssize_t n;
  socklen_t addrlen;
  struct sockaddr_in addr;
  struct addrinfo hints, *res;
  char buffer[256];
  char *response;

  fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
  if (fd == -1) {
    perror("socket");
    exit(1);
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  errcode = getaddrinfo(NULL, port, &hints, &res);
  if (errcode != 0) {
    perror("getaddrinfo");
    exit(1);
  }

  n = bind(fd, res->ai_addr, res->ai_addrlen);
  if (n == -1) {
    perror("bind");
    exit(1);
  }

  if (listen(fd, 5) == -1) {
    perror("listen");
    exit(1);
  }

  while (1){
    addrlen = sizeof(addr);

    if ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) {
      perror("accept");
      exit(1);
    }
    

    puts("Connection accepted");

    n = read(newfd, buffer, 256);
    if (n == -1) {
      perror("read");
      exit(1);
    }

    parse_tcp(buffer, newfd);

    puts("Sending response");
    
    n = write(newfd, response, strlen(response));
    while (n < strlen(response)) {
      n += write(newfd, response+n, strlen(response)-n);
    }

    free(response);
    close(newfd);
  }

  close(fd);
  freeaddrinfo(res);
}

void parse_tcp(char *message, int fd){
  char code[4];
  char plid[7];

  sscanf(message, "%s", code);
  puts(code);

  if (strcmp(code, GSB) == 0){
    get_scoreboard(fd);
  }

  else if(strcmp(code, GHL) == 0){
    sscanf(message, "%s %s", code, plid);
    get_hint(fd, plid);
  }

  else if(strcmp(code, STA) == 0){
    sscanf(message, "%s %s", code, plid);
    state(plid, fd);
  }
}

void get_hint(int fd, char* plid){
  char status[4];
  char st_filename[64];
  char *reply;
  sprintf(st_filename, "GAMES/STATUS_%s.txt", plid);

  FILE *fp = fopen(st_filename, "r"); // Open file with game status
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  char game_status[6];
  char word[31];
  char word_file[64];
  char word_status[31];
  fscanf(fp, "%s %s %s %s", game_status, word, word_file, word_status);
  fclose(fp);

  // get file with word_file name

  char *filename = (char*) malloc(strlen(WORDS) + strlen(word_file) + 1);
  sprintf(filename, "%s%s", WORDS, word_file);

  printf("File name: %s\n", filename);

  fp = fopen(filename, "r");
  if (access(filename, F_OK) == -1 || access(filename, R_OK) == -1) {
    printf(FILE_DNE);
    reply = (char*)malloc(strlen(RHL) + strlen(NOK) + 2);
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
  fread(file, fsize, 1, fp);

  reply = (char*)malloc(strlen(RHL) + strlen(OK) + strlen(word_file) + fsize +5);
  sprintf(reply, "%s %s %s %ld ", RHL, OK, word_file, fsize);

  send_message_tcp(fd, reply);
  send_file(fd, file, fsize);
  send_message_tcp(fd, "\n");
  free(reply);
}

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

void get_scoreboard(int fd){
  char *reply;
  char *filename;
  strcpy(filename, SCOREBOARD);
  FILE *fp = fopen(filename, "r");

  // get filename size
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);

  if (fsize == 0){
    reply = (char*)malloc(strlen(RSB) + strlen(EMPTY) + 3);
    sprintf(reply, "%s %s\n", RSB, EMPTY);
    send_message_tcp(fd, reply);
    return;
  }

  // go back to the beginning of the file
  fseek(fp, 0, SEEK_SET);

  // get file content
  char *file = (char*)malloc(fsize + 1);
  fread(file, fsize, 1, fp);

  reply = (char*)malloc(strlen(RHL) + strlen(OK) + strlen(filename) + fsize + 2);
  sprintf(reply, "%s %s %s %ld ", RSB, OK, filename, fsize);

  send_message_tcp(fd, reply);
  send_file(fd, file, fsize);
  send_message_tcp(fd, "\n");
  free(reply);
}
