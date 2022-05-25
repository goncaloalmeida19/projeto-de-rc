#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "stock_server.h"
#include "shared_memory.h"

#define MAX_USERS 5
#define MARKET_GROUP_1 "239.0.0.1"
#define MARKET_GROUP_2 "239.0.0.2"

int user_count = 0, client_fds[MAX_USERS];
pthread_t threads[MAX_USERS], feed_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* server_thread(void *t){
	int fd = *((int*)t), nread;
	char msg[MSG_LEN], buffer[MSG_LEN], msg_aux1[MSG_LEN], msg_aux2[MSG_LEN], *msg_temp;
	
	//ask client for log in information
	strcpy(msg, "asklogin"); 
	write(fd, msg, strlen(msg)+1);
	
	while(1){
		//receive client command
		nread = read(fd, buffer, MSG_LEN-1);
		if(nread <= 0){
			close(fd);
			pthread_exit(NULL);
		}
		buffer[nread] = '\0';
		printf("%s\n", buffer);
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
			sprintf(msg, "login %s", msg_temp);
			write(fd, msg, strlen(msg)+1);
			free(msg_temp);
		} else {
			printf("Wrong command => %s\n", msg);
		}
	}
	pthread_exit(NULL);
		
}

void* feed(){
	int feed_fd, multicastTTL = 255;
	char *msg_aux;
	struct sockaddr_in markets[MAX_MARKETS_NUM];
	bzero((void *) &markets[0], sizeof(markets[0]));
  	markets[0].sin_family = AF_INET;
  	markets[0].sin_addr.s_addr = inet_addr(MARKET_GROUP_1);
  	markets[0].sin_port = htons(PORTO_BOLSA);
  	if(total_num_markets == 2){
		markets[1] = markets[0];
		markets[1].sin_addr.s_addr = inet_addr(MARKET_GROUP_2);
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
	
	while(1){
		for(int i = 0; i < total_num_markets; i++){
			msg_aux = market_feed(i);
			int cnt = sendto(feed_fd, msg_aux, 4*MSG_LEN, 0, (struct sockaddr *) &markets[i], sizeof(markets[i]));
			printf("feed %d %s\n", i, msg_aux);
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
	int fd, new_client, sockaddr_in_size;
	struct sockaddr_in addr, new_client_addr;
	bzero((void *) &addr, sizeof(addr));
  	addr.sin_family = AF_INET;
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	addr.sin_port = htons(PORTO_BOLSA);
  	
  	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Stock Server: Error creating socket\n");
		return -1;
	}
  	if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0){
		printf("Stock Server: Bind error\n");
		return -1;	
	}
  	if( listen(fd, MAX_USERS) < 0){
		printf("Stock Server: Listen error\n");
		return -1;	
	}
	
	pthread_create(&feed_thread, NULL, feed, NULL);
	
  	sockaddr_in_size = sizeof(struct sockaddr_in);
	while(1){
		printf("accept\n");
		new_client = accept(fd,(struct sockaddr *)&new_client_addr,(socklen_t *)&sockaddr_in_size);
		if(new_client > 0){
			if(user_count >= MAX_USERS){
				strcpy(msg, "userlimit"); 
				write(new_client, msg, strlen(msg)+1);
				close(new_client);
				continue;
			}
			client_fds[user_count] = new_client;
			pthread_create(&threads[user_count], NULL, server_thread, &client_fds[user_count]);
			user_count++;
		}
		pthread_join(threads[user_count-1], NULL);
		sleep(1);
	}
}
