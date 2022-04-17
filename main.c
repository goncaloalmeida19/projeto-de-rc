#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "admin_console.h"
#include "shared_memory.h"

#define FILE_NAME "config.txt"
#define MAX_INIT_USERS_NUM 5
#define MAX_STOCKS_NUM 3

typedef struct {
    char username[WORD_LEN], password[WORD_LEN];
} AdminData;

typedef struct{
    char name[WORD_LEN];
    double balance;
} StockData;

typedef struct{
    char name[WORD_LEN];
    StockData stocks[MAX_STOCKS_NUM];
} StockMarket;

typedef struct{
    int users_len;
    AdminData admin;
    UserData users[MAX_INIT_USERS_NUM];
    StockMarket markets[MAX_MARKETS_NUM];
} ConfigData;

ConfigData * file_data;

void read_file() {
    int i, j, market_num = 0, stocks_num[MAX_MARKETS_NUM], first_market = 1;

    for (j = 0; j < MAX_MARKETS_NUM; j++) stocks_num[j] = 0;

    double temp_stock_balance;
    char last_market[WORD_LEN], new_market[WORD_LEN], temp_stock_name[WORD_LEN];
    file_data = (ConfigData *) malloc(sizeof(ConfigData));
    if(file_data == NULL){
    	printf("Error allocating memory for the config file data\n");
    	exit(1);
    }
    FILE *fp = fopen(FILE_NAME, "r");

    if (fp != NULL) {
        fscanf(fp, " %[^/]/ %s", file_data->admin.username, file_data->admin.password);
        fscanf(fp, "%d", &file_data->users_len);
        // There can be 6 users (admin + 5 initial ones)
        if (file_data->users_len <= 5 && file_data->users_len >= 1) {
            for (i = 0; i < file_data->users_len; i++) {
                fscanf(fp, " %[^;]; %[^;]; %lf", file_data->users[i].username, file_data->users[i].password,
                       &file_data->users[i].balance);
            }
        } else if (file_data->users_len > MAX_INIT_USERS_NUM) {
            printf("Number of initial users needs to be lower than 6\n");
            exit(1);
        } else {
            printf("Number of initial users needs to be higher than 0\n");
            exit(1);
        }

        while (market_num < 2) {
            if (fscanf(fp, " %[^;]; %[^;]; %lf", new_market, temp_stock_name, &temp_stock_balance) != 3)
                break;
            if (first_market) {
                strcpy(file_data->markets[market_num].name, new_market);
                strcpy(last_market, new_market);
                first_market = 0;
            } else if (strcmp(last_market, new_market) != 0) {
                    market_num++;
                    strcpy(file_data->markets[market_num].name, new_market);
                    strcpy(last_market, new_market);
            }
            strcpy(file_data->markets[market_num].stocks[stocks_num[market_num]].name, temp_stock_name);
            file_data->markets[market_num].stocks[stocks_num[market_num]].balance = temp_stock_balance;
            if (stocks_num[market_num]++ > MAX_STOCKS_NUM){
                printf("Number of stocks in a market needs to be lower than 4\n");
                exit(1);
            } 
        }
        fclose(fp);
    } else {
        free(file_data);
        printf("Error in config file\n");
        exit(1);
    }
}

int main() {
	int i;
    printf("Server opened!\n");
    read_file();
    if(create_shm() < 0){
        exit(1);
    }

	for(i = 0; i < file_data->users_len; i++){
		create_user(file_data->users[i].username, file_data->users[i].password, file_data->users[i].markets, file_data->users[i].balance, file_data->users[i].num_markets);
	}
	
    if(fork()==0){
    	if(admin_console(file_data->admin.username, file_data->admin.password)<0)
    		exit(1);
    	exit(0);
    }
    
    wait(NULL);
    close_shm();
    free(file_data);
    exit(0);
}


