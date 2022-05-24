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

char username[WORD_LEN], market1[WORD_LEN], market2[WORD_LEN];

int login(char buffer[MSG_LEN], char msg[MSG_LEN]){
    int option = 0, login_error;
    char password[WORD_LEN];

    // Verify the content of the mensage sent by the server
    if(strcmp(buffer, "asklogin") == 0) {
        printf("Name Password: ");
        scanf("%s %s", username, password);
        sprintf(msg, "login %s %s", username, password);
        return 0;
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
            sprintf(msg, "login %s %s", username, password);
            return 0;
        } else if (option == 2){
            printf("Console closed.\n");
            return -1;
        }
        else {
            printf("Invalid command.\n");
            return 2;
        }
    }
    else if(sscanf(buffer, "login %s", market1) == 1) {
        printf("Login successful! Client can access to the market %s.\n", market1);
        return 1;
    }
    else if(sscanf(buffer, "login %s %s", market1, market2) == 1) {
        printf("Login successful! Client can access to the markets %s and %s.\n", market1, market2);
        return 1;
    }
    else if(strncmp(buffer, "login", 5) == 0) {
        printf("Login successful! Client doesn't have access to any markets.\n");
        return 1;
    }
    else {
        printf("Invalid command.\n");
        return 2;
    }
}

int menu(){
    printf("Menu:\n"
           "1 - Subscribe market\n"
           "2 - Buy/Sell stocks\n"
           "3 - Turn off stocks update feed\n"
           "4 - Wallet information\n"
           "5 - Close session\n"
           "Option: ");
    return 0;
}

int main(int argc, char *argv[]){
    struct hostent *hostPtr;
    struct sockaddr_in addr;
    char server_addr[WORD_LEN], buffer[MSG_LEN], msg[MSG_LEN];
    int fd, nread = 0, login_return = 0;

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

    do{
        // Receive message from server
        nread = read(fd, buffer, MSG_LEN-1);
        if(nread <= 0){
            printf("Server closed.\n");
            break;
        }
        buffer[nread] = '\0';

        if (login_return != 1)
            login_return = login(buffer, msg);
        else
            menu();

        if(login_return == -1)
            break;
        else if(login_return == 0 || login_return == 2) {
            write(fd, msg, strlen(msg) + 1);
            continue;
        } else if(login_return == 1){
            menu();
        }

        // Sent input to the server
        write(fd, msg, strlen(msg)+1);

    }while(1);

    close(fd);
    return 0;
}
