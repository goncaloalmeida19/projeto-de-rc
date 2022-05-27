#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "stock_server.h"
#include "shared_memory.h"

#define MAX_USERS 5
#define MAX_TOTAL_USERS 100
#define MARKET_BASE_GROUP "239.0.0."

int user_count = 0, total_user_count = 0, client_fds[MAX_TOTAL_USERS], server_fd, feed_fd;
pthread_t threads[MAX_TOTAL_USERS], feed_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void clean_resources(){
	//kill admin console
	kill(admin_console_pid, SIGINT);
	wait(NULL);
	//clean other resources
	close_shm();
	close(server_fd);
	close(feed_fd);
	for(int i = 0; i < total_user_count; i++) close(client_fds[i]);
}

void sigint(int signum){
	clean_resources();
	printf("Server closed!\n");
    exit(0);
}

void* server_thread(void *t){
	double price;
	int fd = *((int*)t), nread, shares;
	char msg[MSG_LEN], buffer[MSG_LEN], msg_aux1[MSG_LEN], msg_aux2[MSG_LEN], username[WORD_LEN], *msg_temp;
	
	//ask client for log in information
	strcpy(msg, "asklogin"); 
	write(fd, msg, strlen(msg)+1);
	
	while(1){
		//receive client command
		nread = read(fd, buffer, MSG_LEN-1);
		if(nread <= 0){
			user_count--;
			close(fd);
			pthread_exit(NULL);
		}
		buffer[nread] = '\0';
		if(sscanf(buffer, "login %s %s", msg_aux1, msg_aux2) == 2){
			int result = log_in(msg_aux1, msg_aux2);
			if(result < 0){
				sprintf(msg, "login %d", -result);
				write(fd, msg, strlen(msg)+1);
				continue;
			}
			msg_temp = user_markets(msg_aux1);
			if(msg_temp == NULL){
				strcpy(msg, "internal_error");
				write(fd, msg, strlen(msg)+1);
				continue;
			}
			strcpy(username, msg_aux1);
			sprintf(msg, "login %s", msg_temp);
			free(msg_temp);
		}
		else if(strcmp(buffer, "logout") == 0){
			log_out(username);
			continue;
		}
		else if(sscanf(buffer, "subscribe %s", msg_aux1) == 1){
			int market = subscribe_market(username, msg_aux1);
			if(market < 0) sprintf(msg, "subscribe %d", -market);
			else sprintf(msg, "subscribe 0 %s%d %d", MARKET_BASE_GROUP, market+1, market);
		}
		else if(sscanf(buffer, "buy %s %d %lf", msg_aux1, &shares, &price) == 3){
			int result = buy_share(username, msg_aux1, &shares, &price);
			if(result < 0) sprintf(msg, "buy %d", -result);
			else sprintf(msg, "buy 0 %d %lf", shares, price);
		}
		else if(sscanf(buffer, "sell %s %d %lf", msg_aux1, &shares, &price) == 3){
			int result = sell_share(username, msg_aux1, &shares, &price);
			if(result < 0) sprintf(msg, "sell %d", -result);
			else sprintf(msg, "sell 0 %d %lf", shares, price);
		}
		else if(strcmp(buffer, "wallet") == 0){
			msg_temp = user_wallet(username);
			if(msg_temp == NULL) strcpy(msg, "internal_error");
			else{
				sprintf(msg, "wallet %s#", msg_temp);
				free(msg_temp);
			}
		}
		else {
			printf("Wrong command => %s\n", buffer);
			continue;
		}
		write(fd, msg, strlen(msg)+1);
	}
	user_count--;
	close(fd);
	pthread_exit(NULL);
		
}

void* feed(){
	int multicastTTL = 255;
	char *msg_aux, market_group[WORD_LEN];
	struct sockaddr_in markets[MAX_MARKETS_NUM];
	
	//setup a multicast group for each market
	for(int i = 0; i < total_num_markets; i++){
		//address is 239.0.0.(i+1)
		sprintf(market_group, "%s%d", MARKET_BASE_GROUP, i+1);
		bzero((void *) &markets[i], sizeof(markets[i]));
  		markets[i].sin_family = AF_INET;
  		markets[i].sin_addr.s_addr = inet_addr(market_group);
  		markets[i].sin_port = htons(PORTO_BOLSA);
	}
	feed_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (feed_fd < 0) {
		perror("feed error: socket");
		exit(1);
	}

	if (setsockopt(feed_fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){
		perror("feed error: sockopt");
		exit(1);
	}
	
	int reuseaddr = 1;
	if (setsockopt(feed_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &reuseaddr, sizeof(reuseaddr)) < 0){
		perror("feed error: socket opt 2");
		exit(1);
	}
	
	//send each market feed to its respective multicast group every REFRESH_TIME seconds
	while(1){
		for(int i = 0; i < total_num_markets; i++){
			msg_aux = market_feed(i);
			int cnt = sendto(feed_fd, msg_aux, 4*MSG_LEN, 0, (struct sockaddr *) &markets[i], sizeof(markets[i]));
			free(msg_aux);
			if (cnt < 0) {
				perror("feed error: sendto");
				pthread_exit(NULL);
			}
		}
		sleep(get_refresh_time());
	}

	pthread_exit(NULL);
}

int stock_server(){
	char msg[MSG_LEN];
	int new_client, sockaddr_in_size;
	struct sockaddr_in addr, new_client_addr;
	bzero((void *) &addr, sizeof(addr));
  	addr.sin_family = AF_INET;
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	addr.sin_port = htons(PORTO_BOLSA);
  	
  	//redirect SIGINT
    signal(SIGINT, sigint);
  	
  	if ( (server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Stock Server: Error creating socket\n");
		return -1;
	}
	
  	if ( bind(server_fd,(struct sockaddr*)&addr,sizeof(addr)) < 0){
		printf("Stock Server: Bind error\n");
		return -1;	
	}
  	if( listen(server_fd, MAX_USERS) < 0){
		printf("Stock Server: Listen error\n");
		return -1;	
	}
	
	pthread_create(&feed_thread, NULL, feed, NULL);
	
  	sockaddr_in_size = sizeof(struct sockaddr_in);
  	//accept multiple clients
	while(1){
		new_client = accept(server_fd,(struct sockaddr *)&new_client_addr,(socklen_t *)&sockaddr_in_size);
		if(new_client > 0){
			if(user_count >= MAX_USERS){
				strcpy(msg, "userlimit"); 
				write(new_client, msg, strlen(msg)+1);
				close(new_client);
				continue;
			}
			client_fds[total_user_count] = new_client;
			pthread_create(&threads[total_user_count], NULL, server_thread, &client_fds[total_user_count]);
			user_count++;
			total_user_count++;
		}
	}
}
