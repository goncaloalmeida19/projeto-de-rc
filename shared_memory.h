#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define WORD_LEN 50
#define MAX_USERS_NUM 10
#define MSG_LEN WORD_LEN * 20
#define MAX_MARKETS_NUM 2
#define MAX_STOCKS_NUM 3
#define DEFAULT_REFRESH_TIME 2

typedef struct{
    char username[WORD_LEN], password[WORD_LEN];
    double balance;
    char markets[MAX_MARKETS_NUM][WORD_LEN];
    int shares[MAX_MARKETS_NUM][MAX_STOCKS_NUM];
    int num_markets, logged_in;
} UserData;

typedef struct{
    char name[WORD_LEN];
    int buyer_shares, seller_shares;
    double buyer_price, seller_price;
} StockData;

typedef struct{
    char name[WORD_LEN];
    StockData stocks[MAX_STOCKS_NUM];
    int stock_number;
} StockMarket;

int total_num_markets;

int create_shm(StockMarket markets[MAX_MARKETS_NUM]);
void close_shm();
char* user_markets(const char* username);
int log_in(const char* username, const char* password);
int log_out(const char* username);
int subscribe_market(char* username, char* market);
int buy_share(char* username, char* stock, int *shares, double *price);
int sell_share(char* username, char* stock, int *shares, double *price);
char* user_wallet(char* username);
char* market_feed(int market);
char* print_users();
void update_refresh_time(int refresh);
int get_refresh_time();
int create_user(char *username, char *password, char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets);
int delete_user(char *username);

#endif
