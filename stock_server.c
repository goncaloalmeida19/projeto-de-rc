#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "stock_server.h"

#define MAX_USERS 5

void stock_server(const int PORTO_BOLSA){
	struct sockaddr_in addr;
	bzero((void *) &addr, sizeof(addr));
  	addr.sin_family = AF_INET;
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	addr.sin_port = htons(PORTO_BOLSA);
  	
  	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Stock Server: Error creating socket\n");
  	if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
		printf("Stock Server: Bind error\n");
  	if( listen(fd, MAX_USERS) < 0)
		printf("Stock Server: Listen error\n");
  	
	while(1){
		
		break;
	}
}
