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

#define START "start"
#define PLAY "play"
#define GUESS "guess"
#define SCOREBOARD "scoreboard"
#define HINT "hint"
#define STATE "state"
#define QUIT "quit"
#define EXIT "exit"
#define DEFAULT_PORT "58011" // must be 58000 + Group Number but it isnt defined yet so using default for now
#define GN "GN" // group number

#define SNG "SNG "
#define QUT "QUT\n"

char ip[16];
char port[6];

void message_udp(char *buffer);

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

void parse_args(int argc, char *argv[]){
        if (argc == 1){
        strcpy(port, DEFAULT_PORT);
        // get ip of current machine
        get_ip();
    }

    else if (argc == 3){
        if (strcmp(argv[1], "-n") == 0){
            strcpy(ip, argv[2]);
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
            strcpy(ip, argv[2]);
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


void start_function(){
    char plid[7];
    char message[12];
    scanf("%s", plid);

    strcpy(message, SNG); // SNG 364589\n
    strcat(message, plid);
    strcat(message, "\n");
    message_udp(message);
}


void quit_function(){
    char message[5];
    strcpy(message, QUT);
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


int main(int argc, char *argv[])
{   
    
    char command[20];
    scanf("%s", command);

    parse_args(argc, argv);

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
        scanf("%s", command);
    }
    quit_function();
    return 0;
}