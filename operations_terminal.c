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
int n_markets, fd;

int login(char buffer[MSG_LEN], char msg[MSG_LEN]){
    int option = 0, login_error;
    char password[WORD_LEN];

    // Verify the content of the mensage sent by the server
    if(strcmp(buffer, "asklogin") == 0) {
        printf("Name Password: ");
        scanf("%s %s", username, password);
        sprintf(msg, "login %s %s", username, password);
        write(fd, msg, strlen(msg) + 1);
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
        switch (option){
            case 1:
                printf("Name Password: ");
                scanf("%s %s", username, password);
                sprintf(msg, "login %s %s", username, password);
                write(fd, msg, strlen(msg) + 1);
                return 0;
            case 2:
                printf("Console closed.\n");
                return -1;
            default:
                printf("Invalid command.\n");
                return 2;
        }
    }
    else if(sscanf(buffer, "login %s", market1) == 1) {
        printf("Login successful! Client can access to the market %s.\n", market1);
        n_markets = 1;
        return 1;
    }
    else if(sscanf(buffer, "login %s %s", market1, market2) == 1) {
        printf("Login successful! Client can access to the markets %s and %s.\n", market1, market2);
        n_markets = 2;
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

void subscribe_markets(){
    char buffer[MSG_LEN], msg[MSG_LEN], market[MSG_LEN];
    int option = 0, nread, subscribe_error;
    printf("Market name: ");
    scanf("%s", market);

    sprintf(msg, "subscribe %s", market);
    write(fd, msg, strlen(msg) + 1);

    nread = read(fd, buffer, MSG_LEN-1);
    if(nread <= 0){
        printf("Server closed.\n");
    } else{
        if(sscanf(buffer, "subscribe 0 %s", market1) == 1) {
            printf("Login successful! Client can access to the market %s.\n", market1);
        }
    }
}

int menu(){
    int option = 0, b_s_option = 0, ver = 1;
    while(ver == 1) {
        printf("Menu:\n"
               "1 - Subscribe market\n"
               "2 - Buy/Sell shares\n"
               "3 - Turn on/off stocks update feed\n"
               "4 - Wallet information\n"
               "5 - Close session\n"
               "Option: ");
        scanf("%d", &option);
        switch (option) {
            case 1:
                subscribe_markets();
                break;
            case 2:
                printf("Menu:\n"
                       "1 - Buy share\n"
                       "2 - Sell share\n"
                       "3 - Go back\n"
                       "Option: ");
                scanf("%d", &b_s_option);
                if (b_s_option == 1)
                    buy_share();
                else if (b_s_option == 2)
                    sell_share();
                else if (b_s_option == 3)
                    continue;
                else
                    printf("Invalid command.\n");
                break;
            case 3:
                turn_on_off_stock_update_feed();
                break;
            case 4:
                get_wallet_info();
                break;
            case 5:
                ver = -1;
                break;
            default:
                printf("Invalid command.\n");
                break;
        }
    }
    if (ver == -1) return -1;
    return 0;
}

int main(int argc, char *argv[]){
    struct hostent *hostPtr;
    struct sockaddr_in addr;
    char server_addr[WORD_LEN], buffer[MSG_LEN], msg[MSG_LEN];
    int nread = 0, login_return = 0;

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
            if (menu() == -1)
                login_return = 0;

        if(login_return == -1)
            break;
        else if(login_return == 0 || login_return == 2) {
            continue;
        } else if(login_return == 1){
            menu();
        }
    }while(1);

    close(fd);
    return 0;
}
