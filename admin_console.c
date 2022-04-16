#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "shared_memory.h"

#define PORT 9000

int admin_console(char* admin_username, char* admin_password, SharedMemory* shared_var){
	struct sockaddr_in si_server, si_admin;
    socklen_t slen = sizeof(struct sockaddr_in);
    int s, recv_len, quit_server = 0, refresh;
    double balance;
    char buf[WORD_LEN], msg[MSG_LEN], username[WORD_LEN], password[WORD_LEN], market[WORD_LEN], market2[WORD_LEN], * msg2, markets[MAX_MARKETS_NUM][WORD_LEN];
    
	// Cria um socket para recepção de pacotes UDP
    if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("Error creating socket\n");
        exit(1);
    }
	
	// Preenchimento da socket address structure
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Associa o socket à informação de endereço
    if(bind(s,(struct sockaddr*)&si_server, sizeof(si_server)) == -1) {
        printf("Bind error\n");
        exit(1);
    }
	
	while(1){
		// Espera recepção de tentativa de autenticação
    	if ((recv_len = recvfrom(s, buf, WORD_LEN, 0, (struct sockaddr *) &si_admin, (socklen_t * ) & slen)) == -1) {
        	printf("Recvfrom error\n");
        	exit(1);
    	}
    	buf[recv_len] = '\0';
    	
		//Autenticação
		if(!(strcmp(username, admin_username) == 0 && strcmp(password, admin_password) == 0)){
			sprintf(msg, "Wrong username or password");
			sendto(s, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &si_admin, slen);
			continue;
		}
		
		while(1){
			//Receber comandos
			if ((recv_len = recvfrom(s, buf, WORD_LEN, 0, (struct sockaddr *) &si_admin, (socklen_t *) &slen)) == -1) {
        		printf("Recvfrom error\n");
        		exit(1);
    		}
    		buf[recv_len] = '\0';
    		
    		//Executar comandos
    		if(sscanf(buf, "ADD_USER %s %s %s %lf", username, password, market, &balance) == 4){
                strcpy(markets[0], market);
                if(create_user(username, password, markets, balance, 1) == 0) sprintf(msg, "A new user has been added");
                else sprintf(msg, "New user cannot be created!");
    		}else if(sscanf(buf, "ADD_USER %s %s %s %s %lf", username, password, market, market2, &balance) == 5){
                strcpy(markets[0], market);
                strcpy(markets[1], market2);
                if(create_user(username, password, markets, balance, 2) == 0) sprintf(msg, "A new user has been added");
                else sprintf(msg, "New user cannot be created!");
    		}else if(sscanf(buf, "DEL %s", username) == 1){
                if(delete_user(username) == 0) sprintf(msg, "User with the username %s has been deleted!", username);
                else sprintf(msg, "User with the username %s not found!", username);
    		}else if(strcmp(buf, "LIST") == 0){
                msg2 = print_users();
                strcpy(msg, msg2);
    		}else if(sscanf(buf, "REFRESH %d", &refresh) == 1){
                msg2 = update_refresh_time(refresh);
                strcpy(msg, msg2);
    		}else if(strcmp(buf, "QUIT") == 0){
    			break;
    		}else if(strcmp(buf, "QUIT_SERVER") == 0){
    			quit_server = 1;
    			break;
    		}
            sendto(s, (const char *) msg2, strlen(msg2), 0, (const struct sockaddr*) &si_admin, slen);
            free(msg2);
		}
		
		if(quit_server) break;
    }
    
    // Fecha socket e termina programa
    close(s);
    return 0;

}
