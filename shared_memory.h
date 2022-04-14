#include <semaphore.h>
#define WORD_LEN 50
#define MAX_USERS_NUM 10

typedef struct{
    char username[WORD_LEN], password[WORD_LEN];
    double balance;
} userData;

typedef struct{
    int users_len;
    userData users[MAX_USERS_NUM];
}SharedMemory;


sem_t* shm_mutex;
SharedMemory* shared_var;
int shmid;

int create_shm();
void close_shm();
void print_users();
