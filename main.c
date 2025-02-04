#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "stock_server.h"
#include "admin_console.h"
#include "shared_memory.h"

#define MAX_INIT_USERS_NUM 5

typedef struct {
    char username[WORD_LEN], password[WORD_LEN];
} AdminData;


typedef struct{
    int users_len;
    int markets_num;
    AdminData admin;
    UserData users[MAX_INIT_USERS_NUM];
    StockMarket markets[MAX_MARKETS_NUM];
} ConfigData;

ConfigData * file_data;

void read_file(const char * FILE_NAME) {
    int i, j, first_market = 1;

    double temp_stock_balance;
    char last_market[WORD_LEN], new_market[WORD_LEN], temp_stock_name[WORD_LEN];
    file_data = (ConfigData *) malloc(sizeof(ConfigData));
    if(file_data == NULL){
    	printf("Error allocating memory for the config file data\n");
    	exit(1);
    }
    
    	
    for (j = 0; j < MAX_MARKETS_NUM; j++) file_data->markets[j].stock_number = 0;
    
    FILE *fp = fopen(FILE_NAME, "r");
	
    if (fp != NULL) {
    	//read file data
        if(fscanf(fp, " %[^/]/ %s", file_data->admin.username, file_data->admin.password) != 2){
        	printf("Wrong file format\n");
        	exit(1);
        }
        if(fscanf(fp, "%d", &file_data->users_len) != 1){
        	printf("Wrong file format\n");
        	exit(1);
        }
        // There can be 6 users (admin + 5 initial ones)
        if (file_data->users_len <= 5 && file_data->users_len >= 1) {
            for (i = 0; i < file_data->users_len; i++) {
                if(fscanf(fp, " %[^;]; %[^;]; %lf", file_data->users[i].username, file_data->users[i].password,
                       &file_data->users[i].balance) != 3){
                	printf("Wrong file format\n");
        			exit(1);      
                }
            }
        } else if (file_data->users_len > MAX_INIT_USERS_NUM) {
            printf("Number of initial users needs to be lower than 6\n");
            exit(1);
        } else {
            printf("Number of initial users needs to be higher than 0\n");
            exit(1);
        }
		file_data->markets_num = 0;
        while (file_data->markets_num < 2) {
            if (fscanf(fp, " %[^;]; %[^;]; %lf", new_market, temp_stock_name, &temp_stock_balance) != 3)
                break;
            if (first_market) {
                strcpy(file_data->markets[file_data->markets_num].name, new_market);
                strcpy(last_market, new_market);
                first_market = 0;
            } else if (strcmp(last_market, new_market) != 0) {
                    file_data->markets_num++;
                    strcpy(file_data->markets[file_data->markets_num].name, new_market);
                    strcpy(last_market, new_market);
            }
            //store current stock data 
            strcpy(file_data->markets[file_data->markets_num].stocks[file_data->markets[file_data->markets_num].stock_number].name, temp_stock_name);
            file_data->markets[file_data->markets_num].stocks[file_data->markets[file_data->markets_num].stock_number].seller_price = temp_stock_balance;
            file_data->markets[file_data->markets_num].stocks[file_data->markets[file_data->markets_num].stock_number].buyer_price = temp_stock_balance-0.02;
            file_data->markets[file_data->markets_num].stocks[file_data->markets[file_data->markets_num].stock_number].buyer_shares = ((rand()%10)+1)*10;
            file_data->markets[file_data->markets_num].stocks[file_data->markets[file_data->markets_num].stock_number].seller_shares = ((rand()%10)+1)*10;
            if (++(file_data->markets[file_data->markets_num].stock_number) > MAX_STOCKS_NUM){
                printf("Number of stocks in a market needs to be lower than 4\n");
                exit(1);
            }
        }
        total_num_markets = ++(file_data->markets_num);
        fclose(fp);
    } else {
        free(file_data);
        printf("Error in config file\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
	if(argc != 4){
		printf("Wrong number of arguments\n");
		exit(1);
	}
	
	int i;
    printf("Server opened!\n");
    read_file(argv[3]);
    if(create_shm(file_data->markets) < 0){
        exit(1);
    }
    
    //store users read from file
	for(i = 0; i < file_data->users_len; i++){
		create_user(file_data->users[i].username, file_data->users[i].password, file_data->users[i].markets, file_data->users[i].balance, file_data->users[i].num_markets);
	}
	
	//create process to manage admin console
    if((admin_console_pid = fork())==0){
    	if(admin_console(file_data->admin.username, file_data->admin.password, atoi(argv[2]))<0)
    		exit(1);
    	exit(0);
    }
    
    free(file_data);
    
    //start the stock server
    PORTO_BOLSA = atoi(argv[1]);
	if(stock_server() < 0){
		exit(1);
	}
    
    wait(NULL);
    close_shm();
    printf("Server closed!\n");
    exit(0);
}


