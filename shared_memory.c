#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "shared_memory.h"

int create_semaphore(){
    sem_unlink("MUTEX");
    if((shm_mutex = sem_open("MUTEX",O_CREAT|O_EXCL,0700,1)) == SEM_FAILED)
        return -1;

    return 0;
}

int create_shm(int users_len){
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

    shared_var->users_len = users_len;
    shared_var->refresh_time = DEFAULT_REFRESH_TIME;
    return 0;
}

void close_semaphore(){
    sem_close(shm_mutex);
    sem_unlink("MUTEX");
}

void close_shm(){
    close_semaphore();
    shmdt(shared_var);
    shmctl(shmid, IPC_RMID, NULL);
}

char * print_users(){
    int i;
    char * msg = (char *) malloc(sizeof(char) * shared_var->users_len * MSG_LEN);

    strcpy(msg, "\nList of all users (except admin):\n");
    for(i = 0; i < shared_var->users_len; i++){
        strcat(msg, shared_var->users[i].username);
        strcat(msg, "\n");
    }
    return msg;
}

char * update_refresh_time(int refresh){
    char * msg = (char *) malloc(sizeof(char) * MSG_LEN);
    sprintf(msg, "\nThe stocks value refresh time is now %d\n", refresh);
    shared_var->refresh_time = refresh;
    return msg;
}

int delete_user(char username[WORD_LEN]){
    int i, j;
    for(i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
            // Delete user
            shared_var->users_len--;
            for (j = i; j < shared_var->users_len; j++) shared_var->users[j] = shared_var->users[j + 1];
            return 0;
        }
    }
    return -1;
}

int create_user(char username[WORD_LEN], char password[WORD_LEN], char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets){
    if (shared_var->users_len > MAX_USERS_NUM) {
        return -1;
    }
    strcpy(shared_var->users[shared_var->users_len - 1].username, username);
    strcpy(shared_var->users[shared_var->users_len - 1].password, password);

    int i;
    for (i = 0; i < num_markets; i++) {
        strcpy(shared_var->users[shared_var->users_len - 1].markets[i], markets[i]);
    }

    shared_var->users[shared_var->users_len - 1].balance = balance;
    shared_var->users[shared_var->users_len++ - 1].num_markets = num_markets; // users_len is incremented in 1
    return 0;
}