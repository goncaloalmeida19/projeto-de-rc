#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include "shared_memory.h"

#define SHM_MUTEX_NAME "SHM_MUTEX"

typedef struct{
    int users_len;
    int refresh_time;
    int num_markets;
    UserData users[MAX_USERS_NUM];
    StockMarket markets[MAX_MARKETS_NUM];
}SharedMemory;

sem_t* shm_mutex;
SharedMemory* shared_var;
int shmid;

int create_semaphore(){
    sem_unlink(SHM_MUTEX_NAME);
    if((shm_mutex = sem_open(SHM_MUTEX_NAME,O_CREAT|O_EXCL,0700,1)) == SEM_FAILED)
        return -1;

    return 0;
}

int create_shm(StockMarket markets[MAX_MARKETS_NUM], int num_markets){
    int i;
    
    //create shared memory
    if((shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT|0766))<0){
        perror("Error creating shared memory");
        return -1;
    }

    //attach shared memory
    if((shared_var = (SharedMemory*) shmat(shmid, NULL, 0) )== (SharedMemory*)-1) {
        perror("Error attaching shared memory");
        return -1;
    }

    if(create_semaphore() < 0){
        perror("Error creating semaphore");
        return -1;
    }
	
    shared_var->users_len = 0;
    shared_var->refresh_time = DEFAULT_REFRESH_TIME;
    shared_var->num_markets = num_markets;
    //save markets
    for(i = 0; i < num_markets; i++){
    	shared_var->markets[i] = markets[i];
    }
    
    return 0;
}

void close_semaphore(){
    sem_close(shm_mutex);
    sem_unlink(SHM_MUTEX_NAME);
}

void close_shm(){
    close_semaphore();
    shmdt(shared_var);
    shmctl(shmid, IPC_RMID, NULL);
}

int find_user(const char* username, const char* password){
	for(int i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
        	//user found
        	
        	if(password == NULL) return i; //password is not necessary
        		
        	if(strcmp(shared_var->users[i].password, password) == 0) return i; //username and password correct
        		
			//wrong password
			return -2;
       	}
    }
    //user not found
    return -1;
}


int log_in(const char* username, const char* password){
	sem_wait(shm_mutex);
	int user = find_user(username, password);
    sem_post(shm_mutex);
    return user;
}

int log_out(const char* username){
	sem_wait(shm_mutex);
	int user = find_user(username, NULL);
    sem_post(shm_mutex);
    return user;
}

char* user_markets(const char* username){
	sem_wait(shm_mutex);
	int user = find_user(username, NULL);
	if(user < 0){
		sem_post(shm_mutex);
		return NULL;
	}
	char* msg = (char*)malloc(sizeof(char) * shared_var->users[user].num_markets * WORD_LEN + 1);
	strcpy(msg, shared_var->users[user].markets[0]);
	if(shared_var->users[user].num_markets == 2){
		strcat(msg, " ");
		strcat(msg, shared_var->users[user].markets[1]);
	}
	return msg;	
}

char * print_users(){
	sem_wait(shm_mutex);
    int i;
    if(shared_var->users_len <= 0){
    	sem_post(shm_mutex);
    	return NULL;
    }
    char * msg = (char *) malloc(sizeof(char) * shared_var->users_len * MSG_LEN);
    
    if(msg == NULL){
    	sem_post(shm_mutex);
    	return NULL;
    }

    strcpy(msg, "List of all users (except admin):\n");
    for(i = 0; i < shared_var->users_len; i++){
        strcat(msg, shared_var->users[i].username);
        strcat(msg, "\n");
    }
    sem_post(shm_mutex);
    return msg;
}

void update_refresh_time(int refresh){
	sem_wait(shm_mutex);
    shared_var->refresh_time = refresh;
    sem_post(shm_mutex);
}

int delete_user(char *username){
	sem_wait(shm_mutex);
    int i, j;
    for(i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
            // Delete user
            shared_var->users_len--; 
            for (j = i; j < shared_var->users_len; j++) shared_var->users[j] = shared_var->users[j + 1];
            sem_post(shm_mutex);
            return 0;
        }
    }
    sem_post(shm_mutex);
    return -1;
}

int create_user(char *username, char *password, char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets){
	sem_wait(shm_mutex);
	int i, j, market_exists, user_changed = 0;
    if (shared_var->users_len >= MAX_USERS_NUM) {
    	sem_post(shm_mutex);
        return -1;
    }
    UserData *current_user = &shared_var->users[shared_var->users_len];
    //check if user already exists and password is correct
    for(i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
        	if(strcmp(shared_var->users[i].password, password) == 0){
        		current_user = &shared_var->users[i];
        		user_changed = 1;
        		break;
        	}
        	else{
        		sem_post(shm_mutex);
        		return -2;
        	}
       	}
    }
    //check if markets exist
    for(i = 0; i < num_markets; i++){
    	market_exists = 0;
    	for(j = 0; j < shared_var->num_markets; j++){
    		if(strcmp(markets[i], shared_var->markets[j].name) == 0){
    			market_exists = 1;
    			break;
    		}
    	}
    	if(!market_exists){
    		sem_post(shm_mutex);
    		return -3;
    	}
    }
    strcpy(current_user->username, username);
    strcpy(current_user->password, password);

    
    for (i = 0; i < num_markets; i++) {
        strcpy(current_user->markets[i], markets[i]);
    }

    current_user->balance = balance;
    current_user->num_markets = num_markets;
    if(!user_changed)
    	shared_var->users_len++; // users_len is incremented by 1
    
    sem_post(shm_mutex);
    
    if(user_changed) return 1;
    
    return 0;
}
