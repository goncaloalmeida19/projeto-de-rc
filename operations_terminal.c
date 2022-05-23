/*
    Realizado por:
        João Bernardo de Jesus Santos, nº2020218995
        Gonçalo Fernandes Diogo de Almeida, nº2020218868
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include "shared_memory.h"

char username[WORD_LEN];

int main(int argc, char *argv[]){
    struct hostent *hostPtr;
    struct sockaddr_in addr;
    char server_addr[100], buffer[MSG_LEN], login_msg[MSG_LEN], client_markets[MAX_MARKETS_NUM][WORD_LEN];
    int fd, nread = 0, option = 0, no_input = 0, n_markets = 0, login_error;

    if(argc != 3){
        printf("Wrong number of arguments\n");
        exit(1);
    }

    strcpy(server_addr, argv[1]);
    if ((hostPtr = gethostbyname(server_addr)) == 0)
        erro("Can't obtain the address");

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) atoi(argv[2]));

    if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
        erro("socket");
    if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
        erro("Connect");

    char password[WORD_LEN];

    do{
        // Receive message from server
        nread = read(fd, buffer, MSG_LEN-1);
        if(nread <= 0){
            printf("Server closed.\n");
            break;
        }
        buffer[nread] = '\0';

        // Verify the content of the mensage sent by the server
        if(strcmp(buffer, "asklogin") == 0)
            printf("Name/Password: ");
        else if(sscanf(buffer, "login %s", login_msg) == 1)
            printf("Login successful! Client can access to the market %s.\n", client_markets[0]);
        else if(sscanf(buffer, "login %s %s", client_markets[0], client_markets[1]) == 1)
            printf("Login successful! Client can access to the markets %s and %s.\n", client_markets[0], client_markets[1]);
        else if(sscanf(buffer, "login %d", &login_error) == 1) {
            if (login_error == 1) {
                printf("Username not found.\n");
            } else {
                printf("Wrong Password.\n");
            }
            printf("1 - Try again\n2 - Close console\nOption: ");
            scanf("%d", &option);
            if (option == 1) {
                printf("Name/Password: ");
            } else {
                printf("Console closed.\n");
                break;
            }
        }
        else printf("Invalid command.\n");

        // Read the input and sent it to the server
        scanf("%s", buffer);
        write(fd, buffer, strlen(buffer)+1);

    }while(1);

    close(fd);
    return 0;
}
