#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define WORD_LEN 50
#define MAX_USERS_NUM 10
#define MSG_LEN WORD_LEN * 20
#define MAX_MARKETS_NUM 2
#define DEFAULT_REFRESH_TIME 2

typedef struct{
    char username[WORD_LEN], password[WORD_LEN];
    double balance;
    char markets[MAX_MARKETS_NUM][WORD_LEN];
    int num_markets;
} UserData;

void close_shm();
char* print_users();
void update_refresh_time(int refresh);
int create_shm();
int delete_user(char *username);
int create_user(char *username, char *password, char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets);

#endif
