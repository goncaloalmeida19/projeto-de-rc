#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include "shared_memory.h"

int create_semaphore(){
    sem_unlink("MUTEX");
    if((shm_mutex = sem_open("MUTEX",O_CREAT|O_EXCL,0700,1) == SEM_FAILED)
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
    if((shared_var = (SharedMemory*)shmat(shmid, NULL, 0) == (SharedMemory*)-1) {
        perror("Error attaching shared memory");
        return -1;
    }

    if(create_semaphore() < 0){
        perror("Error creating semaphore");
        return -1;
    }
    return 0;
}

void close_semaphore(){
    sem_close(mutex);
    sem_unlink("MUTEX");
}

void close_shm(){
    close_semaphore();
    shmdt(shared_var);
    shmctl(shmid, IPC_RMID, NULL);
}

void print_users(){
    int i;
    for(i = 0; i < shared_var->users_len; i++){
        printf("%s\n", shared_var->users[i].username);
    }
}