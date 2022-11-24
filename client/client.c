#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define servidor "tejo.ist.utl.pt"
#define port "58011"

#define START "start"
#define PLAY "play"
#define GUESS "guess"
#define SCOREBOARD "scoreboard"
#define HINT "hint"
#define STATE "state"
#define QUIT "quit"
#define EXIT "exit"

int main(int argc, char *argv[])
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    char buffer[128];
    char command[20];
    scanf(stdin, "%s", command);

    // get gsip and gsport

    while (strcmp(command, EXIT) != 0){
        
        if (strcmp(command, PLAY) == 0){
            start_function();
        }

        else if (strcmp(command, QUIT) == 0){
            quit_function();
        }

        else{
            printf("Invalid command\n");
        }
        scanf(stdin, "%s", command);
    }
    return 0;
}

void start_function(){
    char plid[6];
    char message[12];
    scanf(stdin, "%s", plid);
    strcpy(message, "SNG ");
    strcat(message, plid);
    strcat(message, "\n");
    message_udp(message);
}

void quit_function(){
    char message[5];
    strcpy(message, "QUT\n");
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

    errcode = getaddrinfo(servidor, port, &hints, &res);
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

    write(1, "Received: ", 10);
    write(1, response, n);

    freeaddrinfo(res);
    close(fd);
}