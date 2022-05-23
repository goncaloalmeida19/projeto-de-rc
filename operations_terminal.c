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

void operations_terminal(int argc, char *argv[]){
    struct hostent *hostPtr;
    struct sockaddr_in addr;
    char server_addr[100], buffer[MSG_LEN], login_msg[MSG_LEN], * markets, client_markets[MAX_MARKETS_NUM][WORD_LEN];
    int fd, nread = 0, client_id = 0, option = 0, no_input = 0, n_markets = 0;

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
        if(sscanf(buffer, "credentials %d", &client_id) == 1)
            printf("Name/Password: ");
        else if(sscanf(buffer, "login %s", login_msg) == 1){
            if(strcmp(login_msg,"1") == 0 || strcmp(login_msg,"2") == 0) {
                if (strcmp(login_msg, "1") == 0){
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
            } else{
                markets = strtok(login_msg, " ");
                while (markets != strtok(NULL, " "))
                    strcpy(client_markets[n_markets++], markets);
                if(n_markets == 1){
                    printf("Login successful! Client can access to the market %s.\n", client_markets[0]);
                } else {
                    printf("Login successful! Client can access to the markets %s and %s.\n", client_markets[0], client_markets[1]);
                }
            }
        }else erro("comando invalido");

        // Read the input and sent it to the server
        scanf("%s", buffer);
        write(fd, buffer, strlen(buffer)+1);

    }while(1);

    close(fd);
    exit(0);
}
