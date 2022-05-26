#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include "shared_memory.h"

#define SHM_MUTEX_NAME "SHM_MUTEX"

typedef struct{
    int users_len;
    int refresh_time;
    UserData users[MAX_USERS_NUM];
    StockMarket markets[MAX_MARKETS_NUM];
}SharedMemory;

sem_t* shm_mutex;
SharedMemory* shared_var;
int shmid;

int create_semaphore(){
    sem_unlink(SHM_MUTEX_NAME);
    if((shm_mutex = sem_open(SHM_MUTEX_NAME,O_CREAT|O_EXCL,0700,1)) == SEM_FAILED)
        return -1;

    return 0;
}

int create_shm(StockMarket markets[MAX_MARKETS_NUM]){
    int i;
    
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
    //save markets
    for(i = 0; i < total_num_markets; i++){
    	shared_var->markets[i] = markets[i];
    }
    
    return 0;
}

void close_semaphore(){
    sem_close(shm_mutex);
    sem_unlink(SHM_MUTEX_NAME);
}

void close_shm(){
    close_semaphore();
    shmdt(shared_var);
    shmctl(shmid, IPC_RMID, NULL);
}

int find_user(const char* username, const char* password){
	for(int i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
        	//user found
        	
        	if(password == NULL) return i; //password is not necessary
        		
        	if(strcmp(shared_var->users[i].password, password) == 0) return i; //username and password correct
        		
			//wrong password
			return -2;
       	}
    }
    //user not found
    return -1;
}

char* user_wallet(char* username){
	sem_wait(shm_mutex);
	int user = find_user(username, NULL);
	if(user < 0){
		sem_post(shm_mutex);
		return NULL;
	}
	
	char* msg = (char*) malloc(MSG_LEN * sizeof(char)), msg_aux[WORD_LEN*2];
	sprintf(msg, "Wallet for %s:\n\tBalance: %.2lf€\n\tOwned shares:\n", username, shared_var->users[user].balance);
	
	for(int i = 0; i < total_num_markets; i++){
		for(int j = 0; j < shared_var->markets[i].stock_number; j++){
			if(shared_var->users[user].shares[i][j] > 0){
				sprintf(msg_aux, "\t\t%s: %d\n", shared_var->markets[i].stocks[j].name, shared_var->users[user].shares[i][j]);
				strcat(msg, msg_aux);
			}
		}
	}
	
    sem_post(shm_mutex);
    return msg;
}

int log_in(const char* username, const char* password){
	sem_wait(shm_mutex);
	int user = find_user(username, password);
    sem_post(shm_mutex);
    return user;
}

int log_out(const char* username){
	sem_wait(shm_mutex);
	int user = find_user(username, NULL);
    sem_post(shm_mutex);
    return user;
}

int market_id(char* market){
	for(int i = 0; i < total_num_markets; i++){
		if(strcmp(market, shared_var->markets[i].name) == 0){
			return i;
		}
	}
	return -1;
}

int stock_id(char* stock, int* market){
	for(int i = 0; i < total_num_markets; i++){
		for(int j = 0; j < shared_var->markets[i].stock_number; j++){
			if(strcmp(stock, shared_var->markets[i].stocks[j].name) == 0){
				*market = i;
				sem_post(shm_mutex);
				return j;
			}
		}
	}
	return -1;
}

int user_has_permissions(int user_id, int market_id){
	for(int i = 0; i < shared_var->users[user_id].num_markets; i++)
		if(strcmp(shared_var->users[user_id].markets[i], shared_var->markets[market_id].name) == 0)
			return market_id;
	return -2;
}

int subscribe_market(char* username, char* market){
	sem_wait(shm_mutex);
	int m_id = market_id(market);
	if(m_id < 0){
		sem_post(shm_mutex);
		return -1;
	}
	int user_id = find_user(username, NULL);
	if(user_id < 0){
		printf("USER NOT FOUND\n");
		sem_post(shm_mutex);
		return -2;
	} 
	int result = user_has_permissions(user_id, m_id);
	sem_post(shm_mutex);
	return result;
}

void update_stock(int market, int i){
	//int seller_sh = (int)(((rand() % 2)-0.5)*2*10), buyer_sh = (int)(((rand() % 2)-0.5)*2*10);
	double price_change = ((rand() % 2)-0.5)*2*0.01;
	/*if(!(shared_var->markets[market].stocks[i].buyer_shares >= 100 && buyer_sh > 0) && !(shared_var->markets[market].stocks[i].buyer_shares <= 10 && buyer_sh < 0))
		shared_var->markets[market].stocks[i].buyer_shares += buyer_sh;
	if(!(shared_var->markets[market].stocks[i].seller_shares >= 100 && seller_sh > 0) && !(shared_var->markets[market].stocks[i].seller_shares <= 10 && seller_sh < 0))
		shared_var->markets[market].stocks[i].seller_shares += seller_sh;*/
	if(!(shared_var->markets[market].stocks[i].seller_price <= 0.01 && price_change < 0)){
		shared_var->markets[market].stocks[i].buyer_price += price_change;
		shared_var->markets[market].stocks[i].seller_price += price_change;
	}
}

char* market_feed(int market){
	sem_wait(shm_mutex);
	char* msg = (char*)malloc(MSG_LEN*4), msg_aux[MSG_LEN];
	sprintf(msg, "%s:", shared_var->markets[market].name);
	for(int i = 0; i < shared_var->markets[market].stock_number; i++){
		update_stock(market, i);
		sprintf(msg_aux, "\n\t%s-\n\t\tBuyer price: %.2lf eur, Shares: %d\n\t\tSeller price: %.2lf eur, Shares: %d", shared_var->markets[market].stocks[i].name, shared_var->markets[market].stocks[i].buyer_price, shared_var->markets[market].stocks[i].buyer_shares, shared_var->markets[market].stocks[i].seller_price, shared_var->markets[market].stocks[i].seller_shares);
		strcat(msg, msg_aux);
	}
	sem_post(shm_mutex);
	return msg;
}

int buy_share(char* username, char* stock, int *shares, double *price){
	sem_wait(shm_mutex);
	int m_id, s_id, u_id = find_user(username, NULL);
	if((s_id = stock_id(stock, &m_id)) < 0){
		sem_post(shm_mutex);
		return s_id;
	}
	if((m_id = user_has_permissions(u_id, m_id)) < 0){
		sem_post(shm_mutex);
		return m_id;
	}
	StockData *stk = &shared_var->markets[m_id].stocks[s_id];
	if(*price < stk->seller_price){
		sem_post(shm_mutex);
		return -3;
	}
	if(shared_var->users[u_id].balance < stk->seller_price){
		sem_post(shm_mutex);
		return -4;
	}
	if(*shares > stk->seller_shares) *shares = stk->seller_shares;
	if(*price > stk->seller_price) *price = stk->seller_price;
	stk->seller_shares -= *shares;
	shared_var->users[u_id].shares[m_id][s_id] += *shares;
	shared_var->users[u_id].balance -= (*price) * (*shares);
	sem_post(shm_mutex);
	return 0;
}

int sell_share(char* username, char* stock, int *shares, double *price){
	sem_wait(shm_mutex);
	int m_id, s_id, u_id = find_user(username, NULL);
	if((s_id = stock_id(stock, &m_id)) < 0){
		sem_post(shm_mutex);
		return s_id;
	}
	if((m_id = user_has_permissions(u_id, m_id)) < 0){
		sem_post(shm_mutex);
		return m_id;
	}
	StockData *stk = &shared_var->markets[m_id].stocks[s_id];
	if(*price > stk->buyer_price){
		sem_post(shm_mutex);
		return -3;
	}
	if(shared_var->users[u_id].shares[m_id][s_id] > 0){
		sem_post(shm_mutex);
		return -4;
	}
	
	if(*shares > stk->buyer_shares) *shares = stk->buyer_shares;
	if(*price < stk->buyer_price) *price = stk->buyer_price;
	
	stk->buyer_shares -= *shares;
	shared_var->users[u_id].shares[m_id][s_id] -= *shares;
	shared_var->users[u_id].balance += (*price) * (*shares);
	sem_post(shm_mutex);
	return 0;
}

char* user_markets(const char* username){
	sem_wait(shm_mutex);
	int user = find_user(username, NULL);
	if(user < 0){
		sem_post(shm_mutex);
		return NULL;
	}
	char* msg = (char*)malloc(sizeof(char) * shared_var->users[user].num_markets * WORD_LEN + 1);
	strcpy(msg, shared_var->users[user].markets[0]);
	if(shared_var->users[user].num_markets == 2){
		strcat(msg, " ");
		strcat(msg, shared_var->users[user].markets[1]);
	}
	sem_post(shm_mutex);
	return msg;	
}

char * print_users(){
	sem_wait(shm_mutex);
    int i;
    if(shared_var->users_len <= 0){
    	sem_post(shm_mutex);
    	return NULL;
    }
    char * msg = (char *) malloc(sizeof(char) * shared_var->users_len * MSG_LEN);
    
    if(msg == NULL){
    	sem_post(shm_mutex);
    	return NULL;
    }

    strcpy(msg, "List of all users (except admin):\n");
    for(i = 0; i < shared_var->users_len; i++){
        strcat(msg, shared_var->users[i].username);
        strcat(msg, "\n");
    }
    sem_post(shm_mutex);
    return msg;
}

void update_refresh_time(int refresh){
	sem_wait(shm_mutex);
    shared_var->refresh_time = refresh;
    sem_post(shm_mutex);
}

int get_refresh_time(){
	sem_wait(shm_mutex);
    int rt = shared_var->refresh_time;
    sem_post(shm_mutex);
    return rt;
}

int delete_user(char *username){
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

int create_user(char *username, char *password, char markets[MAX_MARKETS_NUM][WORD_LEN], double balance, int num_markets){
	sem_wait(shm_mutex);
	int i, j, market_exists, user_changed = 0;
    if (shared_var->users_len >= MAX_USERS_NUM) {
    	sem_post(shm_mutex);
        return -1;
    }
    UserData *current_user = &shared_var->users[shared_var->users_len];
    //check if user already exists and password is correct
    for(i = 0; i < shared_var->users_len; i++){
        if(strcmp(shared_var->users[i].username, username) == 0){
        	if(strcmp(shared_var->users[i].password, password) == 0){
        		current_user = &shared_var->users[i];
        		user_changed = 1;
        		break;
        	}
        	else{
        		sem_post(shm_mutex);
        		return -2;
        	}
       	}
    }
    //check if markets exist
    for(i = 0; i < num_markets; i++){
    	market_exists = 0;
    	for(j = 0; j < total_num_markets; j++){
    		if(strcmp(markets[i], shared_var->markets[j].name) == 0){
    			market_exists = 1;
    			break;
    		}
    	}
    	if(!market_exists){
    		sem_post(shm_mutex);
    		return -3;
    	}
    }
    strcpy(current_user->username, username);
    strcpy(current_user->password, password);

    
    for (i = 0; i < num_markets; i++) {
        strcpy(current_user->markets[i], markets[i]);
    }

    current_user->balance = balance;
    current_user->num_markets = num_markets;
    if(!user_changed)
    	shared_var->users_len++; // users_len is incremented by 1
    
    sem_post(shm_mutex);
    if(user_changed) return 1;
    
    return 0;
}
