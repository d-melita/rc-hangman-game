#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define START "start"
#define SG "sg"
#define PLAY "play"
#define PL "pl"
#define GUESS "guess"
#define GW "gw"
#define SCOREBOARD "scoreboard"
#define HINT "hint"
#define STATE "state"
#define QUIT "quit"
#define EXIT "exit"
#define DEFAULT_PORT "58011" // must be 58000 + Group Number but it isnt defined yet so using default for now
#define GN "GN" // group number

#define SNG "SNG"
#define RSG "RSG"
#define QUT "QUT"
#define PLG "PLG"
#define RLG "RLG"
#define PWG "PWG"

#define OK "OK"
#define NOK "NOK"
#define ERR "ERR"

char ip[16];
char port[6];
char plid[7];
int trial = 1;

struct current_game {
    int trial;
    char word[31];
    int lenght;
    int errors;
    int max_errors;
    char last_letter[2];
};

struct current_game current_game;

void message_udp(char *buffer);
void set_new_game(char *message);
void play_made(char *message);

//get the ip of current machine when -n not specified
void get_ip() {
    char host[256];
    char *ipbuff;
    struct hostent *hent;
    int hostname;

    hostname = gethostname(host, sizeof(host));
    if (hostname == -1) {
        perror("error getting hostname");
        exit(1);
    }
    hent = gethostbyname(host);
    if (hent == NULL) {
        perror("error getting host by name");
        exit(1);
    }
    ipbuff = inet_ntoa(*((struct in_addr *) hent->h_addr_list[0]));
    strcpy(ip, ipbuff);
}

// get the ip of host when -n specified
void get_ip_know_host(char *host){
    struct addrinfo hints, *res, *p;
    int errcode;
    char buffer[INET_ADDRSTRLEN];
    struct in_addr *addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if ((errcode = getaddrinfo(host, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "error: gettaddrinfo: %s\n", gai_strerror(errcode));
    }
    else {
        for (p = res; p != NULL; p = p->ai_next){
            addr = &((struct sockaddr_in *) p -> ai_addr) -> sin_addr;
            inet_ntop(p->ai_family, addr, buffer, sizeof(buffer));
            strcpy(ip, buffer);
        }
        freeaddrinfo(res);
    }
}

// function to check if ip provided is valid
void check_ip(char *ip_arg){
    char *aux;
    char *copy = strdup(ip_arg);
    aux = strtok(copy, ".");
    while (aux != NULL) {
        // check if aux is a number between 0 and 255 (atoi) but since it is a string we also need to check if each char is a number
        if (atoi(aux) > 255 || atoi(aux) < 0 || aux[0] < '0' || aux[0] > '9' || aux[1] < '0' 
            || aux[1] > '9' || aux[2] < '0' || aux[2] > '9' || aux[3] < '0' || aux[3] > '9') {
            get_ip_know_host(ip_arg);
            return;
        }
        aux = strtok(NULL, ".");
    }
    strcpy(ip, ip_arg);
}


//function to parse arguments (ip and port)
void parse_args(int argc, char *argv[]){
    // we have 4 cases: empty; -n host; -p port; -n host -p port
        if (argc == 1){
        strcpy(port, DEFAULT_PORT);
        get_ip(); // get ip of current machine
    }

    else if (argc == 3){
        if (strcmp(argv[1], "-n") == 0){
            check_ip(argv[2]);
            strcpy(port, DEFAULT_PORT);
        }

        else if (strcmp(argv[1], "-p") == 0){
            strcpy(port, argv[2]);
            get_ip();
        }

        else{
            printf("Invalid arguments");
            exit(1);
        }
    }

    else if (argc == 5){
        if (strcmp(argv[1], "-n") == 0 && strcmp(argv[3], "-p") == 0){
            check_ip(argv[2]);
            strcpy(port, argv[4]);
        }

        else{
            printf("Invalid arguments");
            exit(1);
        }
    }

    else{
        printf("Invalid arguments");
        exit(1);
    }

}

//function to parse server response
void parse_response(char *message){
    char code[4];
    char status[4];
    char *word;

    // scan the message and get the code and status
    sscanf(message, "%s %s", code, status);

    if (strcmp(code, RSG) == 0){
        if (strcmp(status, OK) == 0){
            set_new_game(message);
        }
        else if (strcmp(status, NOK) == 0){
            printf("Error: game already started\n");
        }
    }

    else if (strcmp(code, RLG) == 0){
        if (strcmp(status, OK) == 0){
            play_made(message);
        }
        else if (strcmp(status, NOK) == 0){
            printf("Error: The word does not appear in current word\n");
        }
    }
}

// function to set a new game
void set_new_game(char *message){
    char code[4];
    char status[4];
    char *word; // word only containing _ _ _ _ _ (because we dont know the lenght of the word)
    sscanf(message, "%s %s %d %d", code, status, 
    &current_game.lenght, &current_game.max_errors);
    word = (char*) malloc(current_game.lenght * sizeof(char));
    for (int i = 0; i < current_game.lenght; i++){
        word[i] = '_';
    }
    // set new game components
    strcpy(current_game.word, word);
    current_game.errors = 0;
    current_game.trial = 1;
    strcpy(current_game.last_letter, "");
    printf("New game started (max %d errors): %s (word lenght: %d)\n", 
    current_game.max_errors, current_game.word, current_game.lenght);
}

// function to know which positions to change in the word
void parse_message_play(char *message, char pos[]){
    char c;
    int ne = 0; // nb of white spaces
    int i = 0;
    int j, index = 0;
    for (i; i<strlen(message); i++){
        if (message[i] == ' '){
            ne++;
        }
        if (ne == 4) { // i know i will have exactly 4 spaces before the positions
            break;
        }
    }
    for (j = i; j < strlen(message); j++){
        if (message[j] != ' '){
            pos[index] = message[j];
            index++;
        }
    }

}

//function to show the play made
void play_made(char *message){
    int n; // number of positions where the letter appears
    char code[4];
    char status[4];

    sscanf(message, "%s %s %d %d", code, status, &current_game.trial, &n);
    char pos[n];

    parse_message_play(message, pos);

    for (int i = 0; i < n; i++){
        int index = pos[i] - '0';
        current_game.word[index-1] = current_game.last_letter[0];
    }
    printf("Correct Letter: %s\n", current_game.word);
}

// function to send a message to the server to start a new game
void start_function(){
    char message[12];
    scanf("%s", plid);
    sprintf(message, "%s %s\n", SNG, plid);
    message_udp(message);
}

// function to send a message to the server to make a play
void play_function(){
    char *message;
    char trial_str[10];
    int trial = current_game.trial;

    sprintf(trial_str, "%d", current_game.trial);
    message = malloc(13 + strlen(trial_str));

    scanf("%s", current_game.last_letter);
    sprintf(message, "%s %s %s %d\n", PLG, plid, current_game.last_letter, current_game.trial);

    message_udp(message);
    // increment trial
    current_game.trial++;
    free(message);
}

// function to send a message to the server to make a guess
void guess_function(){
    char *word, *message;
    char trial_str[10];
    word = (char*)malloc(100*sizeof(char));

    sprintf(trial_str, "%d", trial);
    scanf("%s", word);

    message = malloc(12 + strlen(word) + strlen(trial_str));

    sprintf(message, "%s %s %s %d\n", PWG, plid, word, trial);
    message_udp(message);
    trial++;

    free(word);
    free(message);
}

// function to send a message to the server to quit the game
void quit_function(){
    char message[12];
    sprintf(message, "%s %s\n", QUT, plid);
    message_udp(message);
}

void message_udp(char *buffer){
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char response[128];

    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1){
        perror("socket error");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    n = sendto(fd, buffer, strlen(buffer), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1){
        perror("sendto error");
        exit(1);
    }

    addrlen=sizeof(addr);
    n = recvfrom(fd, response, 128, 0, (struct sockaddr *)&addr, &addrlen);

    if (n == -1){
        perror("recvfrom error");
        exit(1);
    }

    write(1, response, n);
    response[n] = '\0';
    parse_response(response);

    freeaddrinfo(res);
    close(fd);
}


int main(int argc, char *argv[]){   
    char command[20];
    scanf("%s", command);

    parse_args(argc, argv);

    while (strcmp(command, EXIT) != 0){
        
        if (strcmp(command, START) == 0 || strcmp(command, SG) == 0){
            start_function();
        }

        else if (strcmp(command, PLAY) == 0 || strcmp(command, PL) == 0){
            play_function();
        }

        else if (strcmp(command, QUIT) == 0){
            quit_function();
        }

        else if (strcmp(command, GUESS) == 0 || strcmp(command, GW) == 0){
            guess_function();
        }

        else {
            printf("Invalid command\n");
        }
        scanf("%s", command);
    }
    quit_function();
    return 0;
}

/*
TO DO:
EXIT - verificar se está um jogo em curso ou não antes de fechar e se estiver terminá-lo
acabar a parte da PLG com os diferentes status
continuar
*/