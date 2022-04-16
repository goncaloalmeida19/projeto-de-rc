#include <semaphore.h>
#define WORD_LEN 50
#define MAX_USERS_NUM 10
#define MSG_LEN WORD_LEN * 100
#define MAX_MARKETS_NUM 2
#define DEFAULT_REFRESH_TIME 2

typedef struct{
    char username[WORD_LEN], password[WORD_LEN];
    double balance;
    char markets[MAX_MARKETS_NUM][WORD_LEN];
    int num_markets;
} userData;

typedef struct{
    int users_len;
    userData users[MAX_USERS_NUM];
    int refresh_time;
}SharedMemory;


sem_t* shm_mutex;
SharedMemory* shared_var;
int shmid;

void close_shm();
char * print_users();
char * update_refresh_time(int refresh);
int create_shm(int users_len);
int delete_user(char username[WORD_LEN]);
int create_user(char username[WORD_LEN], char password[WORD_LEN], char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets);
