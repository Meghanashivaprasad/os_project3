#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
#include<malloc.h>
#include<pthread.h>
#include"utility.h"
#define high_priced_seller_count 1
#define medium_priced_seller_count 3
#define low_priced_seller_count 6
#define total_seller (high_priced_seller_count + medium_priced_seller_count + low_priced_seller_count)
#define concert_hall_row 10
#define concert_hall_col 10
#define simulation_duration 60

//struct for seller
typedef struct sell_arg_struct {
	char seller_no;
	char seller_type;
	queue *seller_queue;
} sell_arg;

typedef struct customer_struct {
	char cust_no;
	int arrival_time;
} customer;

//Global Variable
int sim_time;
int N = 5;
int at1[15] = {0}, st1[15] = {0}, tat1[15] = {0}, bt1[15]={0}, rt1[15]={0};
float throughput[3] = {0};

float avg_rt=0, avg_tat=0, num_cust_served = 0;
char concert_seat_matrix[concert_hall_row][concert_hall_col][5];	//4 to hold L002\0

//Variables for thread handling
pthread_t seller_t[total_seller];
pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_waiting_for_clock_tick_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reservation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_completion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;


//Function definitions
void create_seller_threads(pthread_t *thread, char seller_type, int no_of_sellers);
void wait_for_thread_to_serve_current_time_slice();
void wakeup_all_seller_threads();
void *sell(void *);
queue * generate_customer_queue(int);
int compare_customers_by_arrival_time(void * data1, void * data2);
int findAvailableSeat(char seller_type);

int thread_count = 0;
int threads_waiting_for_clock_tick = 0;
int active_thread = 0;
int verbose = 0;
int main(int argc, char** argv) {
    	srand(4388);

	if(argc == 2) {
		N = atoi(argv[1]);
	}

	//Initialize the variable concert_seat_matrix
	for(int r=0; r<concert_hall_row; r++) {
		for(int c=0; c<concert_hall_col; c++) {
			strncpy(concert_seat_matrix[r][c],"-",1);
		}
	}

	//creation of all high, medium and low priced seller threads
	create_seller_threads(seller_t, 'H', high_priced_seller_count);
	create_seller_threads(seller_t + high_priced_seller_count, 'M', medium_priced_seller_count);
	create_seller_threads(seller_t + high_priced_seller_count + medium_priced_seller_count, 'L', low_priced_seller_count);

	//Wait for threads to finish initialization and wait for synchronized clock tick
	while(1) {
		pthread_mutex_lock(&thread_count_mutex);
		if(thread_count == 0) {
			pthread_mutex_unlock(&thread_count_mutex);
			break;
		}
		pthread_mutex_unlock(&thread_count_mutex);
	}

	//Simulate each time quanta/slice as one iteration
	printf("Simulation of Ticket selling starts here: \n\n\n");
	threads_waiting_for_clock_tick = 0;
	wakeup_all_seller_threads(); //For first tick
	
	do {

		//Wake up all thread
		wait_for_thread_to_serve_current_time_slice();
		sim_time = sim_time + 1;
		wakeup_all_seller_threads();
		//Wait for thread completion
	} while(sim_time < simulation_duration);

	//Wakeup all thread so that no more thread keep waiting for clock Tick in limbo
	wakeup_all_seller_threads();

	while(active_thread);

	//Here we display the concert hall chart
	printf("\n\n");
	printf("Concert hall seat chart for all the tickets sold: \n");
	printf("-----------------------------------------------------------------------------\n");

	int h_customers = 0,m_customers = 0,l_customers = 0;
	for(int r=0;r<concert_hall_row;r++) {
		for(int c=0;c<concert_hall_col;c++) {
			if(c!=0)
				printf("\t");
			printf("%5s",concert_seat_matrix[r][c]);
			if(concert_seat_matrix[r][c][0]=='H') h_customers++;
			if(concert_seat_matrix[r][c][0]=='M') m_customers++;
			if(concert_seat_matrix[r][c][0]=='L') l_customers++;
		}
		printf("\n");
	}
	printf("-----------------------------------------------------------------------------\n\n");
	
	for(int z1=0; z1<N; z1++){
        	int ct = 0;
        	ct = st1[z1] + bt1[z1];
        	rt1[z1] = abs(st1[z1]-at1[z1]);
        	tat1[z1] = abs(ct - at1[z1]);
    	}

    for(int j1=0; j1<N; j1++){
        avg_tat += tat1[j1];
        avg_rt += rt1[j1];
    }

    printf("\nThe Average Turn Around time for this execution cycle is %.2f\n", avg_tat/N);
    printf("The Average Run time for this execution cycle is  is %.2f\n", avg_rt/N);
    printf("Throughput of seller H for this execution cycle is %.2f\n", throughput[0]/60.0);
    printf("Throughput of seller M for this execution cycle is %.2f\n", throughput[1]/60.0);
    printf("Throughput of seller L for this execution cycle is %.2f\n", throughput[2]/60.0);

	printf("\n\nExecution Statistics for N = %02d processes.\n" ,N);
	printf("--------------------------------------------------------------------\n");
	printf("--------------------------------------------------------------------\n");
	printf("|%3c | Total Customers Arrived | Tickets Sold | Customers Returned |\n",' ');
	printf("--------------------------------------------------------------------\n");
	printf("|%3c | %23d | %12d | %18d |\n",'H',high_priced_seller_count*N,h_customers,(high_priced_seller_count*N)-h_customers);
	printf("|%3c | %23d | %12d | %18d |\n",'M',medium_priced_seller_count*N,m_customers,(medium_priced_seller_count*N)-m_customers);
	printf("|%3c | %23d | %12d | %18d |\n",'L',low_priced_seller_count*N,l_customers,(low_priced_seller_count*N)-l_customers);
	printf("--------------------------------------------------------------------\n");

	return 0;
}

// This function creates seller threads for high(H1), medium(M1-M3) and low(L1-L6) priced sellers 
void create_seller_threads(pthread_t *thread, char seller_type, int no_of_sellers){
	//Create all threads
	for(int t_no = 0; t_no < no_of_sellers; t_no++) {
		sell_arg *seller_arg = (sell_arg *) malloc(sizeof(sell_arg));
		seller_arg->seller_no = t_no;
		seller_arg->seller_type = seller_type;
		seller_arg->seller_queue = generate_customer_queue(N);

		pthread_mutex_lock(&thread_count_mutex);
		thread_count++;
		pthread_mutex_unlock(&thread_count_mutex);
		if(verbose)
			printf("Creating thread %c%02d\n",seller_type,t_no);
		pthread_create(thread+t_no, NULL, &sell, seller_arg);
	}
}

void wait_for_thread_to_serve_current_time_slice(){
	//Check if all threads has finished their jobs for this time slice
	while(1){
		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		if(threads_waiting_for_clock_tick == active_thread) {
			threads_waiting_for_clock_tick = 0;	
			pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
			break;
		}
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
	
	
	}
}

//This method wakes up all seller threads to start sales;
void wakeup_all_seller_threads() {

	pthread_mutex_lock( &condition_mutex );
	if(verbose)
		printf("00:%02d Main Thread Broadcasting Clock Tick\n",sim_time);
	pthread_cond_broadcast( &condition_cond);
	pthread_mutex_unlock( &condition_mutex );
}

void *sell(void *t_args) {
	//Initializing thread
	sell_arg *args = (sell_arg *) t_args;
	queue * customer_queue = args->seller_queue;
	queue * seller_queue = create_queue();
	char seller_type = args->seller_type;
	int seller_no = args->seller_no + 1;
	
	pthread_mutex_lock(&thread_count_mutex);
	thread_count--;
	active_thread++;
	pthread_mutex_unlock(&thread_count_mutex);

	customer *cust = NULL;
	int random_wait_time = 0;
    	int temp1 = 0;
	

	while(sim_time < simulation_duration) {
		//Waiting for clock tick
		pthread_mutex_lock(&condition_mutex);
		if(verbose)
			printf("00:%02d %c%02d Waiting for next clock tick\n",sim_time,seller_type,seller_no);
		
		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		threads_waiting_for_clock_tick++;
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
		
		pthread_cond_wait( &condition_cond, &condition_mutex);
		if(verbose)
			printf("00:%02d %c%02d Received Clock Tick\n",sim_time,seller_type,seller_no);
		pthread_mutex_unlock( &condition_mutex );

		// Sell
		if(sim_time == simulation_duration) break;
		//All New Customer Came
		while(customer_queue->size > 0 && ((customer *)customer_queue->head->data)->arrival_time <= sim_time) {
			customer *temp = (customer *) dequeue (customer_queue);
			enqueue(seller_queue,temp);
			printf("00:%02d %c%d/%02d Customer arrived and is waiting for %c%d\n",sim_time,seller_type,seller_no,temp->cust_no,seller_type,seller_no);
		}
		//Serve next customer
		if(cust == NULL && seller_queue->size>0) {
			cust = (customer *) dequeue(seller_queue);
			printf("00:%02d %c%d/%02d Customer arrived and is being served by %c%d\n",sim_time,seller_type,seller_no,cust->cust_no,seller_type,seller_no);
			switch(seller_type) {
				case 'H':
				random_wait_time = (rand()%2) + 1;
                		bt1[temp1] = random_wait_time;
                		temp1++;
				break;
				case 'M':
				random_wait_time = (rand()%3) + 2;
                		bt1[temp1] = random_wait_time;
                		temp1++;
				break;
				case 'L':
				random_wait_time = (rand()%4) + 4;
                		bt1[temp1] = random_wait_time;
                		temp1++;
			}
		}
		if(cust != NULL) {
			//printf("Wait time %d\n",random_wait_time);
			if(random_wait_time == 0) {
				//Selling Seat
				pthread_mutex_lock(&reservation_mutex);

				// Find seat
				int seatIndex = findAvailableSeat(seller_type);
				if(seatIndex == -1) {
					printf("00:%02d %c%d/%02d Customer has been told that Concert is Sold Out by %c%d.\n",sim_time,seller_type,seller_no,cust->cust_no,seller_type,seller_no);
				} else {
					int row_no = seatIndex/concert_hall_col;
					int col_no = seatIndex%concert_hall_col;
					sprintf(concert_seat_matrix[row_no][col_no],"%c%d%02d",seller_type,seller_no,cust->cust_no);
					printf("00:%02d %c%d/%02d Customer is assigned row %d, seat %d by %c%d  \n",sim_time,seller_type,seller_no,cust->cust_no,row_no + 1,col_no + 1,seller_type,seller_no);
                    			num_cust_served++;
                    			if (seller_type == 'L')
                        			throughput[0]++;
                    			else if (seller_type=='M')
                        			throughput[1]++;
                    			else if (seller_type == 'H')
                        			throughput[2]++;
				}
				pthread_mutex_unlock(&reservation_mutex);
				cust = NULL;
			} else {
				random_wait_time--;
			}
		} else {
			//printf("00:%02d %c%02d Waiting for customer\n",sim_time,seller_type,seller_no);
		}
	}

	while(cust!=NULL || seller_queue->size > 0) {
		if(cust==NULL)
			cust = (customer *) dequeue(seller_queue);
		printf("00:%02d %c%d/%02d Customer Leaves as Ticket Sale is Closed by %c%d\n",sim_time,seller_type,seller_no,cust->cust_no,seller_type,seller_no);
		cust = NULL;
	}

	pthread_mutex_lock(&thread_count_mutex);
	active_thread--;
	pthread_mutex_unlock(&thread_count_mutex);
}

int findAvailableSeat(char seller_type){
	int seatIndex = -1;

	if(seller_type == 'H') {
		for(int row_no = 0;row_no < concert_hall_row; row_no ++ ){
			for(int col_no = 0;col_no < concert_hall_col; col_no ++) {
				if(strcmp(concert_seat_matrix[row_no][col_no],"-") == 0) {
					seatIndex = row_no * concert_hall_col + col_no;
					return seatIndex;
				}
			}
		}
	} else if(seller_type == 'M') {
		int mid = (concert_hall_row / 2)-1;
		int row_jump = 0;
		int next_row_no = mid;
		for(row_jump = 0;;row_jump++) {
			int row_no = mid+row_jump;
			if(mid + row_jump < concert_hall_row) {
				for(int col_no = 0;col_no < concert_hall_col; col_no ++) {
					if(strcmp(concert_seat_matrix[row_no][col_no],"-") == 0) {
						seatIndex = row_no * concert_hall_col + col_no;
						return seatIndex;
					}
				}
			}
			row_no = mid - row_jump;
			if(mid - row_jump >= 0) {
				for(int col_no = 0;col_no < concert_hall_col; col_no ++) {
					if(strcmp(concert_seat_matrix[row_no][col_no],"-") == 0) {
						seatIndex = row_no * concert_hall_col + col_no;
						return seatIndex;
					}
				}
			}
			if(mid + row_jump >= concert_hall_row && mid - row_jump < 0) {
				break;
			}
		}
	} else if(seller_type == 'L') {
		for(int row_no = concert_hall_row - 1;row_no >= 0; row_no -- ){
			for(int col_no = concert_hall_col - 1;col_no >= 0; col_no --) {
				if(strcmp(concert_seat_matrix[row_no][col_no],"-") == 0) {
					seatIndex = row_no * concert_hall_col + col_no;
					return seatIndex;
				}
			}
		}
	}

	return -1;
}

// This method generates a list of N customers
queue * generate_customer_queue(int N){
	queue * customer_queue = create_queue();
	char cust_no = 0;
	while(N--) {
		customer *cust = (customer *) malloc(sizeof(customer));
		cust->cust_no = cust_no;
		cust->arrival_time = rand() % simulation_duration;
        	at1[cust_no] = cust->arrival_time;
		enqueue(customer_queue,cust);
		cust_no++;
	}
	sort(customer_queue, compare_customers_by_arrival_time);
	node * ptr = customer_queue->head;
	cust_no = 0;
	while(ptr!=NULL) {
		cust_no ++;
		customer *cust = (customer *) ptr->data;
		cust->cust_no = cust_no;
		ptr = ptr->next;
	}
	return customer_queue;
}


// This method is used by sort function to sort the customers based on the arrival time
int compare_customers_by_arrival_time(void * data1, void * data2) {
	customer *c1 = (customer *)data1;
	customer *c2 = (customer *)data2;
	if(c1->arrival_time < c2->arrival_time) {
		return -1;
	} else if(c1->arrival_time == c2->arrival_time){
		return 0;
	} else {
		return 1;
	}
}
