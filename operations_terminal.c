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
#define MAX_MARKETS_NUM 2

pthread_t feed_thread[MAX_MARKETS_NUM];
pthread_mutex_t feed_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t feed_cond = PTHREAD_COND_INITIALIZER;
char ips[MAX_MARKETS_NUM][WORD_LEN];
int fd, feed_fd[MAX_MARKETS_NUM], feed_id[MAX_MARKETS_NUM], SERVER_PORT, subs_markets[MAX_MARKETS_NUM];
int feed_on = 1;

void* feed(void *t){
	int id = *((int*)t), addr_len = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	struct ip_mreq mreq;
	char msg[4*MSG_LEN];
	
	
	feed_fd[id] = socket(AF_INET, SOCK_DGRAM, 0);
	if (feed_fd[id] < 0) {
		perror("feed error: socket");
		exit(1);
	}
	int multicastTTL = 255;
	if (setsockopt(feed_fd[id], IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){
		perror("feed error: socket opt");
		exit(1);
	}
	
	int reuseaddr = 1;
	if (setsockopt(feed_fd[id], SOL_SOCKET, SO_REUSEADDR, (void *) &reuseaddr, sizeof(reuseaddr)) < 0){
		perror("feed error: socket opt 2");
		exit(1);
	}
	
	bzero((void *) &addr, sizeof(addr));
  	addr.sin_family = AF_INET;
  	addr.sin_port = htons(SERVER_PORT);
  	addr.sin_addr.s_addr = inet_addr(ips[id]);
	mreq.imr_multiaddr.s_addr = inet_addr(ips[id]); 
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	
	if(setsockopt(feed_fd[id], IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
		perror("setsockopt mreq");
		exit(1);
	}
	
	if (bind(feed_fd[id], (struct sockaddr *) &addr, addr_len) < 0) { 
		perror("feed: bind");
		exit(1);
	} 
	
	while (1) {
		pthread_mutex_lock(&feed_mutex);
		while(!feed_on){
			pthread_cond_wait(&feed_cond, &feed_mutex);
		}
		pthread_mutex_unlock(&feed_mutex);
		
		int cnt = recvfrom(feed_fd[id], msg, sizeof(msg), 0, (struct sockaddr *) &addr, (socklen_t*)&addr_len);

		pthread_mutex_lock(&feed_mutex);
		if(!feed_on){
			pthread_mutex_unlock(&feed_mutex);
			continue;
		}
		pthread_mutex_unlock(&feed_mutex);
		
		if (cnt < 0) {
			perror("feed: recvfrom");
			exit(1);
		} else if (cnt == 0) {
			break;
		}
		pthread_mutex_lock(&feed_mutex);
		printf("feed%d: \"%s\"\n", id, msg);
		pthread_mutex_unlock(&feed_mutex);
	}
	printf("leave\n");
	pthread_exit(NULL);
}


int buy_share(){
    char buffer[MSG_LEN], msg[MSG_LEN], stock[WORD_LEN];
    int n_shares, nread, buy_error;
    double price;
	printf("Name of the stock: ");
    scanf("%s", stock);
    printf("Number of shares: ");
    scanf("%d", &n_shares);
    if (n_shares % 10 != 0){
        printf("Number of shares invalid. Operation aborted.\n");
    } else{
        printf("Price of the share: ");
        scanf("%lf", &price);

        sprintf(msg, "buy %s %d %lf", stock, n_shares, price);
        write(fd, msg, strlen(msg) + 1);

        nread = read(fd, buffer, MSG_LEN-1);
        if(nread <= 0){
            return -1;
        } else{
            if(sscanf(buffer, "buy 0 %d %lf", &n_shares, &price) == 2) {
                printf("%d shares were bought at the price %lf of the stock %s\n", n_shares, price, stock);
            }
            else if(sscanf(buffer, "buy %d", &buy_error) == 1) {
                if(buy_error == 1)
                    printf("Invalid stock.\n");
                else if(buy_error == 2)
                	printf("User doesn't have permissions to access that stock.\n");
                else if(buy_error == 3)
                    printf("Operation refused. Buying price is lower than the selling price.\n");
                else if(buy_error == 4)
                    printf("Insufficient funds.\n");
                else
                    printf("Invalid command.\n");
            }
        }
    }
    return 0;
}


int sell_share(){
    char buffer[MSG_LEN], msg[MSG_LEN], stock[WORD_LEN];
    int n_shares, nread, sell_error;
    double price;
    printf("Name of the stock: ");
    scanf("%s", stock);
    printf("Number of shares: ");
    scanf("%d", &n_shares);
    if (n_shares % 10 != 0){
        printf("Number of shares invalid. Operation aborted.\n");
    } else{
        printf("Price of the share: ");
        scanf("%lf", &price);

        sprintf(msg, "sell %s %d %lf", stock, n_shares, price);
        write(fd, msg, strlen(msg) + 1);

        nread = read(fd, buffer, MSG_LEN-1);
        if(nread <= 0){
            return -1;
        } else{
            if(sscanf(buffer, "sell 0 %d %lf", &n_shares, &price) == 2) {
                printf("%d shares were sold at the price %lf of the stock %s\n", n_shares, price, stock);
            }
            else if(sscanf(buffer, "sell %d", &sell_error) == 1) {
                if(sell_error == 1)
                    printf("Invalid stock.\n");
                else if(sell_error == 2)
                	printf("User doesn't have permissions to access that stock.\n");
                else if(sell_error == 3)
                    printf("Operation refused. Selling price is higher than the buying price.\n");
                else if(sell_error == 4)
                    printf("Not enough shares in the wallet to sell.\n");
                else
                    printf("Invalid command.\n");
            }
        }
    }
    return 0;
}


void turn_on_off_stock_update_feed(){
	pthread_mutex_lock(&feed_mutex);
	if(feed_on) feed_on = 0;
	else feed_on = 1;
	pthread_cond_broadcast(&feed_cond);
	pthread_mutex_unlock(&feed_mutex);
}


int get_wallet_info(){
    char buffer[MSG_LEN], msg[MSG_LEN], wallet_info[MSG_LEN];
    int nread;

    sprintf(msg, "wallet");
    write(fd, msg, strlen(msg) + 1);

    nread = read(fd, buffer, MSG_LEN-1);
    if(nread <= 0){
        return -1;
    } else{
        if(sscanf(buffer, "wallet %[^#]", wallet_info) == 1)
            printf("%s", wallet_info);
        else
            printf("Invalid command.\n");
    }
    return 0;
}


int login(char buffer[MSG_LEN], char msg[MSG_LEN]){
    int option = 0, login_error;
    char password[WORD_LEN], username[WORD_LEN], market1[WORD_LEN], market2[WORD_LEN];

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
    else if(sscanf(buffer, "login %s %s", market1, market2) == 2) {
        printf("Login successful! Client can access to the markets %s and %s.\n", market1, market2);
        return 1;
    }
    else if(sscanf(buffer, "login %s", market1) == 1) {
        printf("Login successful! Client can access to the market %s.\n", market1);
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


int subscribe_markets(){
    char buffer[MSG_LEN], msg[MSG_LEN], market[WORD_LEN], ip[WORD_LEN];
    int nread, subscribe_error, subs_id;
    printf("Market name: ");
    scanf("%s", market);

    sprintf(msg, "subscribe %s", market);
    write(fd, msg, strlen(msg) + 1);

    nread = read(fd, buffer, MSG_LEN-1);
    if(nread <= 0){
        return -1;
    } else{
        if(sscanf(buffer, "subscribe 0 %s %d", ip, &subs_id) == 2){
        	if(subs_markets[subs_id]){
        		printf("Market %s has already been subscribed.\n", market);
        	}else{
            	printf("Market %s has been subscribed\n", market);
            	subs_markets[subs_id] = 1;
            	strcpy(ips[subs_id], ip);
				feed_id[subs_id] = subs_id;
    			pthread_create(&feed_thread[subs_id], NULL, feed, &feed_id[subs_id]);
    		}
        }
        else if(sscanf(buffer, "subscribe %d", &subscribe_error) == 1) {
            if(subscribe_error == 1)
                printf("The indicated market does not exist.\n");
            else if(subscribe_error == 2)
                printf("The market cannot be subscribed because user does not have permission to subscribe it.\n");
            else
                printf("Invalid command.\n");
        }else printf("Invalid command => %s\n", buffer);
    }
    return 0;
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
        fflush(stdout);
        scanf("%d", &option);
        switch (option) {
            case 1:
                if(subscribe_markets() == -1) ver = -1;
                break;
            case 2:
                printf("Menu:\n"
                       "1 - Buy share\n"
                       "2 - Sell share\n"
                       "3 - Go back\n"
                       "Option: ");
                scanf("%d", &b_s_option);
                if (b_s_option == 1) {
                    if (buy_share() == -1) ver = -1;
                }
                else if (b_s_option == 2) {
                    if (sell_share() == -1) ver = -1;
                }
                else if (b_s_option == 3)
                    continue;
                else
                    printf("Invalid command.\n");
                break;
            case 3:
                turn_on_off_stock_update_feed();
                break;
            case 4:
                if(get_wallet_info() == -1) ver = -1;
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
    struct sockaddr_in addr_terminal;
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
	
    bzero((void *) &addr_terminal, sizeof(addr_terminal));
    addr_terminal.sin_family = AF_INET;
    addr_terminal.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr_terminal.sin_port = htons((short) SERVER_PORT);

    if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
        perror("socket");
    if (connect(fd,(struct sockaddr *)&addr_terminal,sizeof (addr_terminal)) < 0)
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
                login_return = -1;

        if(login_return == -1)
            break;
        else if(login_return == 0 || login_return == 2) {
            continue;
        } else if(login_return == 1){
            menu();
        }
    }while(1);
	for(int i = 0; i < MAX_MARKETS_NUM; i++){
		if(subs_markets[i]){
			pthread_cancel(feed_thread[i]);
			pthread_join(feed_thread[i], NULL);
			close(feed_fd[i]);
		}
	}
    close(fd);
    return 0;
}
