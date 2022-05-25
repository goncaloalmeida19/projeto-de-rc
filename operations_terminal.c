/*
    Realizado por:
        João Bernardo de Jesus Santos, nº2020218995
        Gonçalo Fernandes Diogo de Almeida, nº2020218868
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

#define WORD_LEN 50
#define MSG_LEN WORD_LEN*20
#define MAX_MARKET_NUM 2

pthread_t feed_thread;
pthread_mutex_t markets_mutex = PTHREAD_MUTEX_INITIALIZER;
char username[WORD_LEN], market1[WORD_LEN], market2[WORD_LEN];
int n_markets, fd, feed_fd, SERVER_PORT;

void set_n_markets(int n){
	pthread_mutex_lock(&markets_mutex);
	n_markets = n;
	pthread_mutex_unlock(&markets_mutex);
}

int get_n_markets(){
	pthread_mutex_lock(&markets_mutex);
	int n = n_markets;
	pthread_mutex_unlock(&markets_mutex);
	return n;
}


void* feed(){
	int addr_len = sizeof(struct sockaddr_in);
	char msg[4*MSG_LEN];
	struct ip_mreq mreq[MAX_MARKET_NUM];
	struct sockaddr_in addr;
	
	feed_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (feed_fd < 0) {
		perror("feed error: socket");
		exit(1);
	}
	int multicastTTL = 255; // by default TTL=1; the packet is not transmitted to other networks
	if (setsockopt(feed_fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){
		perror("feed error: socket opt");
		exit(1);
	}
	
	int reuseaddr = 1;
	if (setsockopt(feed_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &reuseaddr, sizeof(reuseaddr)) < 0){
		perror("feed error: socket opt 2");
		exit(1);
	}
	
	bzero((void *) &addr, sizeof(addr));
  	addr.sin_family = AF_INET;
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	addr.sin_port = htons(SERVER_PORT);

	if (bind(feed_fd, (struct sockaddr *) &addr, addr_len) < 0) { 
		perror("feed: bind");
		exit(1);
	} 
	
	//temporario
	//para desativar basta por n_markets = 0
	n_markets = 2;
	mreq[0].imr_multiaddr.s_addr = inet_addr("239.0.0.1"); 
	mreq[0].imr_interface.s_addr = htonl(INADDR_ANY); 
	if (setsockopt(feed_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq[0], sizeof(mreq[0])) < 0) {
		perror("setsockopt mreq1");
		exit(1);
	} 
	mreq[1].imr_multiaddr.s_addr = inet_addr("239.0.0.2"); 
	mreq[1].imr_interface.s_addr = htonl(INADDR_ANY); 
	if (setsockopt(feed_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq[1], sizeof(mreq[1])) < 0) {
		perror("setsockopt mreq2");
		exit(1);
	} 
	//end temporario
	
	while (1) {
		for(int i = 0; i < get_n_markets(); i++){
			int cnt = recvfrom(feed_fd, msg, sizeof(msg), 0, (struct sockaddr *) &addr, (socklen_t *)&addr_len);
			if (cnt < 0) {
				perror("feed: recvfrom");
				exit(1);
			} else if (cnt == 0) {
			break;
			}
			printf("feed: \"%s\"\n", msg);
		}
	}
	printf("leave\n");
	pthread_exit(NULL);
}

void buy_share(){
	printf("buy share\n");
}


void sell_share(){
	printf("sell share\n");
}

void turn_on_off_stock_update_feed(){
	printf("turn_on_off_stock_update_feed\n");
}

void get_wallet_info(){
	printf("get_wallet_info\n");
}

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
        set_n_markets(1);
        return 1;
    }
    else if(sscanf(buffer, "login %s %s", market1, market2) == 1) {
        printf("Login successful! Client can access to the markets %s and %s.\n", market1, market2);
        set_n_markets(2);
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
    char buffer[MSG_LEN], msg[MSG_LEN], market[WORD_LEN];
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
    printf("markets %d\n", n_markets);
    pthread_create(&feed_thread, NULL, feed, NULL);
    while(ver == 1) {
        printf("Menu:\n"
               "1 - Subscribe market\n"
               "2 - Buy/Sell shares\n"
               "3 - Turn on/off stocks update feed\n"
               "4 - Wallet information\n"
               "5 - Close session\n"
               "Option: ");
        fflush(stdout);
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
    if((hostPtr = gethostbyname(server_addr)) == 0){
        perror("Can't obtain the address");
	}
	
	SERVER_PORT = atoi(argv[2]);
	
    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) SERVER_PORT);

    if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
        perror("socket");
    if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
        perror("connect");

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
