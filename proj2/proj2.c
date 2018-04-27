#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <sys/types.h> 	//getpid()
#include <unistd.h>		//getpid(), fork()
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>		//O_CREAT

sem_t *semaphore_mutex = NULL;
sem_t *semaphore_enter = NULL;		// Allows riders to enter the station
sem_t *semaphore_board = NULL;		// Allows riders to board
sem_t *semaphore_depart = NULL;		// Allows the bus to depart
sem_t *semaphore_finish = NULL;		// Allows riders to exit the bus and finish

// Number of the line in output file
int *sh_output_order = NULL;
int sh_output_order_id = 0;

// Total generated riders (used for numbering them)
int *sh_rider_amount = NULL;
int sh_rider_amount_id = 0;

// Amount of riders waiting at the bus stop
int *sh_riders_waiting = NULL;
int sh_riders_waiting_id = 0;

// Amount of riders left to board during current boarding
int *sh_riders_left_boarding = NULL;
int sh_riders_left_boarding_id = 0;

// Total amount of riders to be transported
int *sh_riders_remaining = NULL;
int sh_riders_remaining_id = 0;


FILE *out_file = NULL;
	
void parse_input(int *R, int *C, int *ART, int *ABT, int argc, char *argv[])
{
	if (argc != 5)
	{
		fprintf(stderr, "Error: Invalid amount of arguments\n");
		exit(1);
	}
	
	for (int i = 1; i < argc; i++)
	{
		for(int j = 0; argv[i][j] != '\0'; j++)
		{
			if (!isdigit(argv[i][j]))
			{
				fprintf(stderr, "Error: Invalid argument %d\n", i-1);
				exit(1);
			}		
		}
	}
	
	*R = atoi(argv[1]);
	*C = atoi(argv[2]);
	*ART = atoi(argv[3]);
	*ABT = atoi(argv[4]);
	
	if (*R <= 0)
	{
		fprintf(stderr, "Invalid R argument\n");
		exit(1);
	}
	if (*C <= 0)
	{
		fprintf(stderr, "spatny C\n");
		exit(1);
	}	
	if (*ART < 0 || *ART > 1000)
	{
		fprintf(stderr, "spatny ART\n");
		exit(1);
	}
	if (*ABT < 0 || *ABT > 1000)
	{
		fprintf(stderr, "spatny ABT\n");
		exit(1);
	}
}

int init()
{	
	if ((semaphore_mutex = sem_open("/xhavli47.semaphore0", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
		return -1;	
	if ((semaphore_enter = sem_open("/xhavli47.semaphore1", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
		return -1;
	if ((semaphore_board = sem_open("/xhavli47.semaphore2", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
		return -1;
	if ((semaphore_depart = sem_open("/xhavli47.semaphore3", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
		return -1;
	if ((semaphore_finish = sem_open("/xhavli47.semaphore4", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
		return -1;		
	
	// if (semaphore_mutex == SEM_FAILED || semaphore_enter == SEM_FAILED || semaphore_board == SEM_FAILED || semaphore_depart == SEM_FAILED || semaphore_finish == SEM_FAILED)
	// {
		// fprintf(stderr,"Error creating a semaphore\n");
		// return -1;
	// }
	
	if ((sh_output_order_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
		return -1;
    if ((sh_output_order = shmat(sh_output_order_id, NULL, 1)) == NULL)
		return -1;
	if ((sh_rider_amount_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
		return -1;
    if ((sh_rider_amount = shmat(sh_rider_amount_id, NULL, 1)) == NULL)
		return -1;	
	if ((sh_riders_waiting_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
		return -1;
    if ((sh_riders_waiting = shmat(sh_riders_waiting_id, NULL, 0)) == NULL)
		return -1;		
	if ((sh_riders_left_boarding_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
		return -1;
    if ((sh_riders_left_boarding = shmat(sh_riders_left_boarding_id, NULL, 0)) == NULL)
		return -1;
	if ((sh_riders_remaining_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
		return -1;
    if ((sh_riders_remaining = shmat(sh_riders_remaining_id, NULL, 0)) == NULL)
		return -1;
	
	// if (sh_output_order_id == -1 || sh_output_order == -1 || sh_rider_amount_id == -1 || sh_rider_amount == -1 || sh_riders_waiting_id == -1 || sh_riders_waiting == -1 || sh_riders_left_boarding_id == -1 || sh_riders_left_boarding == -1 || sh_riders_remaining_id == -1 || sh_riders_remaining == -1 ||)
	
	if ((out_file = fopen("proj2.out", "w")) == NULL)
		return -1;
	
	
	
	printf("DEBUG: Init success\n");
	return 0;
}

void clean_up()
{
	sem_close(semaphore_mutex);
	sem_unlink("/xhavli47.semaphore0");
	sem_close(semaphore_enter);
	sem_unlink("/xhavli47.semaphore1");
	sem_close(semaphore_board);
	sem_unlink("/xhavli47.semaphore2");
	sem_close(semaphore_depart);
	sem_unlink("/xhavli47.semaphore3");
	sem_close(semaphore_finish);
	sem_unlink("/xhavli47.semaphore4");
	
	shmctl(sh_output_order_id, IPC_RMID, NULL);
	shmctl(sh_rider_amount_id, IPC_RMID, NULL);
	shmctl(sh_riders_waiting_id, IPC_RMID, NULL);
	shmctl(sh_riders_left_boarding_id, IPC_RMID, NULL);
	shmctl(sh_riders_remaining_id, IPC_RMID, NULL);
	
	printf("DEBUG: Called fclose\n");
	if (out_file != NULL)
		fclose(out_file);
}

void print_file(FILE * stream, const char * format, ...)
{
  va_list args;
  
  va_start (args, format);
  fseek(stream, 0, SEEK_END);
  vfprintf (stream, format, args);
  fflush(stream);
  va_end (args);
  
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
}

void process_bus(int capacity, int delay)
{
	print_file(out_file, "%d\t: BUS\t: start\n", ++(*sh_output_order));
	
	while(*sh_riders_remaining > 0)
	{
		// Bus arriving to the station
		sem_wait(semaphore_enter);		// New riders are now unable to enter the station
		sem_wait(semaphore_mutex);
		print_file(out_file, "%d\t: BUS\t: arrival\n", ++(*sh_output_order));
		sem_post(semaphore_mutex);
		
		
		// Waiting riders - board them
		// No waiting riders - depart
		sem_wait(semaphore_mutex);
		// printf("DEBUG BUS: Waiting: %d; left boarding: %d; remaining: %d\n", *sh_riders_waiting, *sh_riders_left_boarding, *sh_riders_remaining);
		if (*sh_riders_waiting > 0)
		{
			print_file(out_file, "%d\t: BUS\t: start boarding %d\n", ++(*sh_output_order), *sh_riders_waiting);
			if (*sh_riders_waiting < capacity)
				*sh_riders_left_boarding = *sh_riders_waiting;
			else
				*sh_riders_left_boarding = capacity;
			
			
			sem_post(semaphore_mutex);		// Some riders might want to exit, give them a chance
			sem_wait(semaphore_finish); 	// Don't allow any riders to exit
			sem_wait(semaphore_mutex);		// No more exiting, we are boarding
			
			sem_post(semaphore_board);		// Allow riders to board, wait for them to tell the bus to depart
			// sem_wait(semaphore_finish); 	// Don't allow any riders to exit
			sem_post(semaphore_mutex);
			
			sem_wait(semaphore_depart);		// Leave after all riders have boarded
		
			sem_wait(semaphore_mutex);
			print_file(out_file, "%d\t: BUS\t: end boarding %d\n", ++(*sh_output_order), *sh_riders_waiting);
		}
		else
		{
			sem_post(semaphore_mutex);		// Some riders might want to exit, give them a chance
			sem_wait(semaphore_finish); 	// Don't allow any riders to exit
			sem_wait(semaphore_mutex);		// No more exiting, we are departing
		}
		print_file(out_file, "%d\t: BUS\t: depart\n", ++(*sh_output_order));
		
		sem_post(semaphore_enter);		// New riders are able to enter the station now
		sem_post(semaphore_mutex);
		
		if (delay != 0)
			usleep(((rand() % delay))*1000);
		
		sem_wait(semaphore_mutex);
		print_file(out_file, "%d\t: BUS\t: end\n", ++(*sh_output_order));
		sem_post(semaphore_finish);		// Riders are able to exit now
		sem_post(semaphore_mutex);
	}
	
	sem_wait(semaphore_mutex);
	print_file(out_file, "%d\t: BUS\t: finish\n", ++(*sh_output_order));
	sem_post(semaphore_mutex);
	exit(0);	
}

void process_rider()
{
	// Rider starting
	sem_wait(semaphore_mutex);
	int rider_id = ++(*sh_rider_amount);
	print_file(out_file, "%d\t: RID %d\t: start\n", ++(*sh_output_order), rider_id);
	sem_post(semaphore_mutex);

	// Rider entering station, bus must not be in station, waiting++
	sem_wait(semaphore_enter);
	sem_wait(semaphore_mutex);
	print_file(out_file, "%d\t: RID %d\t: enter: %d\n", ++(*sh_output_order), rider_id, ++(*sh_riders_waiting));
	sem_post(semaphore_mutex);
	sem_post(semaphore_enter);

	// Rider boarding, bus must arrive to the station first
	sem_wait(semaphore_board);
	sem_wait(semaphore_mutex);
	print_file(out_file, "%d\t: RID %d\t: boarding\n", ++(*sh_output_order), rider_id);
	(*sh_riders_waiting)--;
	(*sh_riders_left_boarding)--;
	(*sh_riders_remaining)--;
	// printf("DEBUG: Waiting: %d; left boarding: %d on board: %d\n", *sh_riders_waiting, *sh_riders_left_boarding, *sh_riders_onboard);
	
	// Keep unlocking boarding semaphore while there are waiting riders or bus capacity has not been reached
	if (*sh_riders_left_boarding > 0)
		sem_post(semaphore_board);
	
	// Don't unlock boarding semaphore if there is no capacity in the bus or nobody is waiting
	if (*sh_riders_left_boarding == 0)
		sem_post(semaphore_depart);
	sem_post(semaphore_mutex);
	
	// Waiting for bus to end
	sem_wait(semaphore_finish);
	sem_wait(semaphore_mutex);
	print_file(out_file, "%d\t: RID %d\t: finish\n", ++(*sh_output_order), rider_id);
	// printf("DEBUG: Remaining: %d on board: %d\n", *sh_riders_remaining, *sh_riders_onboard);
	sem_post(semaphore_mutex);
	sem_post(semaphore_finish);


	exit(0);
}

void generate_riders(int amount, int delay)
{
	int status = 0;
	printf("DEBUG: Called generate_riders function %d %d\n", amount, delay);
	for (int i = 0; i < amount; i++)
	{
		pid_t rider_id = fork();
		if (rider_id == 0)
		{
			// Child process
			process_rider();
		}
		// Main process
		// SLEEP(delay);
		// printf("Random time: %d\n", ((float)(rand() % delay))/1000);
		if (delay != 0)
			usleep(((rand() % delay))*1000);
	}
	printf("Process generate_riders waiting...\n");
	while (wait(&status) > 0);
	printf("DEBUG: Exiting generate_riders process\n");
	exit(0);	
}

int main(int argc, char *argv[])
{
	printf("DEBUG: Hello, World!\n");
	int R;		// Amount of rider processes; R > 0
	int C;		// Capacity of the bus; C > 0
	int ART;	// Maximum time (ms) between generating rider processes; 0 <= ART <= 1000
	int ABT;	// Maximum time (ms) while process bus simulates a ride; 0 <= ABT <= 1000
	parse_input(&R, &C, &ART, &ABT, argc, argv);
	printf("DEBUG: R: %d; C: %d; ART: %d; ABT: %d\n", R, C, ART, ABT);
	
	int status = 0;
	
	// Creating semaphore and opening output file
	if (init() == -1)
	{
		clean_up();
		return 1;
	}
	
	// Amount of riders to be transported
	// Condition for the bus process (cannot end until all riders have been transported)
	*sh_riders_remaining = R;

	
	// Creating bus process
	pid_t bus_id = fork();
	if (bus_id == 0)	// Child process
	{
		process_bus(C, ABT);
	}
	// Main process

	// Creating rider processes
	pid_t generate_riders_id = fork();
	if (generate_riders_id == 0)	// Child process
	{
		generate_riders(R, ART);
	}
	// Main process
	
	// Cleaning semaphores and shared memory
	printf("DEBUG: Main process waiting...\n");
	while (wait(&status) > 0);
	clean_up();
	printf("DEBUG: Exiting main process\n");
	return 0;
}
