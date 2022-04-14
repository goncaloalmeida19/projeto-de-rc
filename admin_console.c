#include <stdio.h>
#include "admin_console.h"
#include "shared_memory.h"

int admin_console(char* admin_username, char* admin_password, SharedMemory* shared_var){
	struct sockaddr_in si_server, si_admin;
    socklen_t slen = sizeof(sockaddr_in);
    int s, recv_len, quit_server = 0, refresh;
    double balance;
    char buf[BUF_LEN], msg[BUF_LEN], username[BUF_LEN], password[BUF_LEN], market[BUF_LEN], market2[BUF_LEN];
    
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
    	if ((recv_len = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr *) &si_admin, (socklen_t * ) & slen)) == -1) {
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
			if ((recv_len = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr *) &si_admin, (socklen_t * ) & slen)) == -1) {
        		printf("Recvfrom error\n");
        		exit(1);
    		}
    		buf[recv_len] = '\0';
    		
    		//Executar comandos
    		if(sscanf(buf, "ADD_USER %s %s %s %lf", username, password, market, &balance) == 4){
    		
    		}else if(sscanf(buf, "ADD_USER %s %s %s %s %lf", username, password, market, market2, &balance) == 5){
    		
    		}else if(sscanf(buf, "DEL %s", username) == 1){
    		
    		}else if(strcmp(buf, "LIST") == 0){
    			
    		}else if(sscanf(buf, "REFRESH %d", refresh) == 1){
    		
    		}else if(strcmp(buf, "QUIT") == 0){
    			break;
    		}else if(strcmp(buf, "QUIT_SERVER") == 0){
    			quit_server = 1;
    			break;
    		}
		}
		
		if(quit_server) break;
    }
    
    // Fecha socket e termina programa
    close(s);
    return 0;

}
