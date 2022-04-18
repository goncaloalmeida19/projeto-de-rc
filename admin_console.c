#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "shared_memory.h"

void sockaddr_in_copy(struct sockaddr_in *new, struct sockaddr_in *old){
	new->sin_family = old->sin_family;
    new->sin_port = old->sin_port;
    new->sin_addr.s_addr = old->sin_addr.s_addr;
}

int compare_sockaddr_in(struct sockaddr_in *si1, struct sockaddr_in *si2){
	return (si1->sin_family == si2->sin_family) && (si1->sin_port == si2->sin_port) && (si1->sin_addr.s_addr == si2->sin_addr.s_addr);
}

int admin_console(char* admin_username, char* admin_password, const int PORTO_CONFIG){
	struct sockaddr_in si_server, si_admin, si_admin_copy;
    socklen_t slen = sizeof(struct sockaddr_in);
    int s, recv_len, quit_server = 0, quit_console = 0, refresh;
    double balance;
    char buf[MSG_LEN], msg[2*MSG_LEN], username[WORD_LEN], password[WORD_LEN], market[WORD_LEN], market2[WORD_LEN], markets[MAX_MARKETS_NUM][WORD_LEN];
    
	// Create socket to receive UDP packets
    if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("Error creating socket\n");
        return -1;
    }
	
	// Filling the socket address structure
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORTO_CONFIG);
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Bind the socket to the address data
    if(bind(s,(struct sockaddr*)&si_server, sizeof(si_server)) == -1) {
        printf("Bind error\n");
        exit(1);
    }
    
	
	while(1){
		// Wait for authentication attempt
    	if ((recv_len = recvfrom(s, buf, MSG_LEN, 0, (struct sockaddr *) &si_admin, (socklen_t * ) & slen)) == -1) {
        	printf("Recvfrom error\n");
        	return -1;
    	}
    	buf[recv_len] = '\0';
    	
    	if(sscanf(buf, " %[^/]/ %s", username, password) != 2){
    		if(recv_len != 0){
    			strcpy(msg, "Please insert admin username and password in this format: username/password\n");
				sendto(s, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &si_admin, slen);
			}
			continue;
    	}
    	
    	
		//Authentication
		if(!(strcmp(username, admin_username) == 0 && strcmp(password, admin_password) == 0)){
			strcpy(msg, "Wrong username or password\n");
			sendto(s, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &si_admin, slen);
			continue;
		}
		
    	//Save the current admin information to ensure that
    	//no other entity can send commands without logging in
    	sockaddr_in_copy(&si_admin_copy, &si_admin);
		
		strcpy(msg, "Admin logged in!\n");
		sendto(s, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &si_admin, slen);
		
		while(1){
			//Receive commands
			if ((recv_len = recvfrom(s, buf, MSG_LEN, 0, (struct sockaddr *) &si_admin, (socklen_t *) &slen)) == -1) {
        		printf("Recvfrom error\n");
        		exit(1);
    		}
    		buf[recv_len] = '\0';
    		
    		//Verify if the command was received from the entity who successfully logged in
    		if(!compare_sockaddr_in(&si_admin_copy, &si_admin)){
    			strcpy(msg, "Admin already connected! Wait to log in before sending commands!\n");
				sendto(s, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &si_admin, slen);
				continue;
    		}
    		
    		char *msg_temp;
    		//Execute commands
    		if(sscanf(buf, "ADD_USER %s %s %s %lf\n", username, password, market, &balance) == 4){
                strcpy(markets[0], market);
                int return_value = create_user(username, password, markets, balance, 1);
                if(return_value == 0) strcpy(msg, "A new user has been added!\n");
                else if(return_value == -1) strcpy(msg, "New user cannot be created!\n");
                else if(return_value == -2) strcpy(msg, "User found but the password is wrong!\n");
                else if(return_value == -3) strcpy(msg, "Market does not exist!\n");
                else strcpy(msg, "User updated\n");
                
    		}else if(sscanf(buf, "ADD_USER %s %s %s %s %lf\n", username, password, market, market2, &balance) == 5){
                strcpy(markets[0], market);
                strcpy(markets[1], market2);
                int return_value = create_user(username, password, markets, balance, 2);
                if(return_value == 0) strcpy(msg, "A new user has been added!\n");
                else if(return_value == -1) strcpy(msg, "New user cannot be created!\n");
                else if(return_value == -2) strcpy(msg, "User found but the password is wrong!\n");
                else if(return_value == -3) strcpy(msg, "Market does not exist!\n");
                else strcpy(msg, "User updated\n");
                
    		}else if(sscanf(buf, "DEL %s\n", username) == 1){
                if(delete_user(username) == 0) sprintf(msg, "User with the username %s has been deleted!\n", username);
                else sprintf(msg, "User with the username %s not found!\n", username);
                
    		}else if(strcmp(buf, "LIST\n") == 0){
    			msg_temp = print_users();
    			if(msg_temp == NULL)
    				strcpy(msg, "There are no users registered\n");
    			else{
                	strcpy(msg, msg_temp);
                	free(msg_temp);
                }
                
    		}else if(sscanf(buf, "REFRESH %d\n", &refresh) == 1){
                update_refresh_time(refresh);
                sprintf(msg, "The stocks value refresh time is now %d!\n", refresh);
                
    		}else if(strcmp(buf, "QUIT\n") == 0){
    			strcpy(msg, "Quitting console\n");
    			quit_console = 1;
    			
    		}else if(strcmp(buf, "QUIT_SERVER\n") == 0){
    			strcpy(msg, "Shutting down server\n");
    			quit_console = 1;
    			quit_server = 1;
    			
    		}else sprintf(msg, "Unknown command or wrong number of arguments: %s",buf);
    		
            sendto(s, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &si_admin, slen);
            if(quit_console){
            	quit_console = 0;
            	break;
            }
            	
		}
		
		if(quit_server){
			quit_server = 0;
			break;
		}
    }
    
    // Fecha socket e termina programa
    close(s);
    return 0;

}
