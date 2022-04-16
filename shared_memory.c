#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "shared_memory.h"

typedef struct{
    int users_len;
    userData users[MAX_USERS_NUM];
    int refresh_time;
}SharedMemory;

sem_t* shm_mutex;
SharedMemory* shared_var;
int shmid;

int create_semaphore(){
    sem_unlink("SHM_MUTEX");
    if((shm_mutex = sem_open("SHM_MUTEX",O_CREAT|O_EXCL,0700,1)) == SEM_FAILED)
        return -1;

    return 0;
}

int create_shm(){
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
    return 0;
}

void close_semaphore(){
    sem_close(shm_mutex);
    sem_unlink("SHM_MUTEX");
}

void close_shm(){
    close_semaphore();
    shmdt(shared_var);
    shmctl(shmid, IPC_RMID, NULL);
}

char * print_users(){
	sem_wait(shm_mutex);
    int i;
    if(shared_var->users_len <= 0){
    	sem_post(shm_mutex);
    	return NULL;
    }
    char * msg = (char *) malloc(sizeof(char) * shared_var->users_len * WORD_LEN);

    strcpy(msg, "List of all users (except admin):\n");
    for(i = 0; i < shared_var->users_len; i++){
        strcat(msg, shared_var->users[i].username);
        strcat(msg, "\n");
    }
    sem_post(shm_mutex);
    return msg;
}

char * update_refresh_time(int refresh){
	sem_wait(shm_mutex);
    char * msg = (char *) malloc(sizeof(char) * MSG_LEN);
    
    shared_var->refresh_time = refresh;
    sem_post(shm_mutex);
    return msg;
}

int delete_user(char username[WORD_LEN]){
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

int create_user(char username[WORD_LEN], char password[WORD_LEN], char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets){
	sem_wait(shm_mutex);
	int i;
    if (shared_var->users_len > MAX_USERS_NUM) {
    	sem_post(shm_mutex);
        return -1;
    }
    userData *current_user = &shared_var->users[shared_var->users_len];
    for(i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
        	current_user = &shared_var->users[i];
        	shared_var->users_len--;
        	break;
       	}
    }
    strcpy(current_user->username, username);
    strcpy(current_user->password, password);

    
    for (i = 0; i < num_markets; i++) {
        strcpy(current_user->markets[i], markets[i]);
    }

    current_user->balance = balance;
    current_user->num_markets = num_markets; 
    shared_var->users_len++; // users_len is incremented by 1
    sem_post(shm_mutex);
    return 0;
}
