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

#define WORD_LEN 50
#define MSG_LEN WORD_LEN*20

char username[WORD_LEN];

int main(int argc, char *argv[]){
    struct hostent *hostPtr;
    struct sockaddr_in addr;
    char server_addr[WORD_LEN], buffer[MSG_LEN], market1[WORD_LEN], market2[WORD_LEN];
    int fd, nread = 0, option = 0, login_error;

    if(argc != 3){
        printf("Wrong number of arguments\n");
        exit(1);
    }

    strcpy(server_addr, argv[1]);
    if ((hostPtr = gethostbyname(server_addr)) == 0)
        printf("Can't obtain the address\n");

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) atoi(argv[2]));

    if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
        printf("socket\n");
    if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
        printf("Connect\n");

    char password[WORD_LEN];

    do{
        // Receive message from server
        nread = read(fd, buffer, MSG_LEN-1);
        if(nread <= 0){
            printf("Server closed.\n");
            break;
        }
        buffer[nread] = '\0';
		printf("%s\n", buffer);
        // Verify the content of the mensage sent by the server
        if(strcmp(buffer, "asklogin") == 0) {
            printf("Name Password: ");
            scanf("%s %s", username, password);
            sprintf(buffer, "login %s %s", username, password);
            printf("%s\n", buffer);
        }
        else if(sscanf(buffer, "login %d", &login_error) == 1) {
            if (login_error == 1) {
                printf("Username not found.\n");
            } else {
                printf("Wrong Password.\n");
            }
            printf("1 - Try again\n2 - Close console\nOption: \n");
            scanf("%d", &option);
            if (option == 1) {
                printf("Name Password: ");
                scanf("%s %s", username, password);
                sprintf(buffer, "login %s %s", username, password);
            } else if (option == 2){
                printf("Console closed.\n");
                break;
            }
            else printf("Invalid command.\n");
        }
        else if(sscanf(buffer, "login %s", market1) == 1)
            printf("Login successful! Client can access to the market %s.\n", market1);
        else if(sscanf(buffer, "login %s %s", market1, market2) == 1)
            printf("Login successful! Client can access to the markets %s and %s.\n", market1, market2);
        else if(strncmp(buffer, "login", 5) == 0)
        	printf("Login successful! Client doesn't have access to any markets.\n");
        else printf("Invalid command.\n");

        // Sent input to the server
        write(fd, buffer, strlen(buffer)+1);

    }while(1);

    close(fd);
    return 0;
}
