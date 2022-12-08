#include "server.h"

char port[6];
int verbose = 0; // 0 false, 1 true

int main(int argc, char *argv[]){

    parse_args(argc, argv);

    message_udp();

    return 0;
}

void parse_args(int argc, char *argv[]){
    if (argc == 2){
        strcpy(port, PORT);
    }

    else if (argc == 3){
        if (strcmp(argv[2], "-v") == 0){
            verbose = 1;
            strcpy(port, PORT);
        }
        else{
            printf("Invalid arguments\n");
            exit(1);
        }
    }

    else if (argc == 4){
        if (strcmp(argv[2], "-p") == 0){
            strcpy(port, argv[3]);
        }
        else{
            printf("Invalid arguments\n");
            exit(1);
        }
    }

    else if (argc == 5){
        if (strcmp(argv[2], "-p") == 0 && strcmp(argv[4], "-v") == 0){
            strcpy(port, argv[3]);
            verbose = 1;
        }
        else{
            printf("Invalid arguments\n");
            exit(1);
        }
    }

    else{
        printf("Invalid arguments\n");
        exit(1);
    }
}

void message_udp(){
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buf[256];

    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1){
        perror("socket");
        exit(1);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(1);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1){
        perror("bind");
        exit(1);
    }

    while (1){
        addrlen = sizeof(addr);
        n = recvfrom(fd, buf, 256, 0, (struct sockaddr *) &addr, &addrlen);
        if (n == -1){
            perror("recvfrom");
            exit(1);
        }

        buf[n] = '\0';
        
        if (verbose == 1){
            printf("Received message: %s\n", buf);
        }

        parse_message_udp(buf);
    }
}

void parse_message_udp(char *message){
    char code[4];
    sscanf(message, "%s", code);

    if (strcmp(code, SNG) == 0){
        // start new game
        start_new_game(message);
    }

    else if (strcmp(code, PLG)){
        play_letter(message);
    }

    else if (strcmp(code, PWG) == 0){
        // guess word
        guess_word(message);
    }

    else if (strcmp(code, QUT)){
        // quit
        quit(message);
    }

    else{
        // error
    }
}

void start_new_game(char *message){
    char code[4];
    char plid[7];
    char status[4];

    sscanf(message, "%s %s", code, plid);

    // check on going game
    // if no on going game, strcpy(status, NOK);

    // else, strcpy(status, OK) and choose word from file

    // send message to client
}

void play_letter(char *message){
    char code[4];
    char plid[7];
    char letter[2];
    int trial;
    char status[4];

    sscanf(message, "%s %s %s %d", code, plid, letter, trial);

    // if letter is in word, strcpy(status, OK); give number of occurences and positions

    // else if letter is in word and word == guessed, strcpy(status, WIN);

    // else if letter is not in word & errors++ != max_errors, strcpy(status, NOK);

    // else if letter is not in word & errors++ == max_errors, strcpy(status, OVR) and end game;

    // else if letter sent on a previous trial, strcpy(status, DUP);

    // else if trial != trial, strcpy(status, INV);

    // check on going game
    // if no on going game, strcpy(status, ERR);

    // send message to client
}

void guess_word(char *message){
    char code[4];
    char plid[7];
    char word[31];
    int trial;
    char status[4];

    sscanf(message, "%s %s %s %d", code, plid, word, trial);

    // if word == guessed, strcpy(status, WIN);

    // else if word != guessed & errors++ != max_errors, strcpy(status, NOK);

    // else if word != guessed & errors++ == max_errors, strcpy(status, OVR) and end game;

    // else if word sent on a previous trial, strcpy(status, DUP);

    // else if trial != trial, strcpy(status, INV);

    // check on going game
    // if no on going game, strcpy(status, ERR);

    // send message to client
}

void quit(char *message){
    char code[4];
    char plid[7];
    char status[4];

    sscanf(message, "%s %s", code, plid);

    // check on going game
    // if no on going game, strcpy(status, ERR);

    // else, strcpy(status, OK) and end game;

    // send message to client
}