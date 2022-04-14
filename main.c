#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_NAME "config.txt"
#define WORD_LEN 50
#define MAX_INIT_USERS_NUM 5
#define MAX_USERS_NUM 10
#define MAX_MARKETS_NUM 2
#define MAX_ACTIONS_NUM 3

typedef struct {
    char username[WORD_LEN], password[WORD_LEN];
} adminData;

typedef struct{
    char username[WORD_LEN], password[WORD_LEN];
    double balance;
} userData;

typedef struct{
    char name[WORD_LEN];
    double balance;
} actionData;

typedef struct{
    char name[WORD_LEN];
    actionData actions[MAX_ACTIONS_NUM];
} stockMarket;

typedef struct{
    int users_len;
    adminData admin;
    userData users[MAX_INIT_USERS_NUM];
    stockMarket markets[MAX_MARKETS_NUM];
} configData;

configData *read_file() {
    int i, j, market_num = 0, actions_num[MAX_MARKETS_NUM], first_market = 1;

    for (j = 0; j < MAX_MARKETS_NUM; j++) actions_num[j] = 0;

    double temp_action_balance;
    char last_market[WORD_LEN], new_market[WORD_LEN], temp_action_name[WORD_LEN];
    configData *file_data = (configData *) malloc(sizeof(configData));
    FILE *fp = fopen(FILE_NAME, "r");

    if (fp != NULL) {
        fscanf(fp, "%[^/ ]/ %50s", file_data->admin.username, file_data->admin.password);
        fscanf(fp, "%d", &file_data->users_len);

        if (file_data->users_len <= 5 && file_data->users_len >= 1) {
            for (i = 0; i < file_data->users_len; i++) {
                fscanf(fp, " %[^; ] ; %[^; ] ; %lf", file_data->users[i].username, file_data->users[i].password,
                       &file_data->users[i].balance);
            }
        } else if (file_data->users_len > MAX_INIT_USERS_NUM) {
            printf("Number of initial users needs to be lower than 6\n");
            exit(1);
        } else {
            printf("Number of initial users needs to be higher than 0\n");
            exit(1);
        }

        while (market_num <= 2) {
            if (fscanf(fp, " %[^; ] ; %[^; ] ; %lf", new_market, temp_action_name, &temp_action_balance) != 3)
                break;
            if (first_market) {
                strcpy(file_data->markets[market_num].name, new_market);
                strcpy(last_market, new_market);
                first_market = 0;
            } else if (strcmp(last_market, new_market) != 0) {  // Não são iguais
                    market_num++;
                    strcpy(file_data->markets[market_num].name, new_market);
                    strcpy(last_market, new_market);
            }
            strcpy(file_data->markets[market_num].actions[actions_num[market_num]].name,temp_action_name);
            file_data->markets[market_num].actions[actions_num[market_num]++].balance = temp_action_balance;
            if (actions_num[market_num]++ > MAX_ACTIONS_NUM){
                printf("Number of actions in a market needs to be lower than 4\n");
                exit(1);
            }
        }
        fclose(fp);
        return file_data;
    } else {
        fclose(fp);
        free(file_data);
        printf("Error in config file\n");
        exit(1);
    }
}

int main() {
    printf("Hello, World!\n");
    configData * lol = read_file();
    free(lol);
    return 0;
}


