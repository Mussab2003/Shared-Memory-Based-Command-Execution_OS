#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#define SIZE 10000
#define NUM_OF_PROCESSES 4

pthread_mutex_t *shared_mutex = NULL;
pthread_mutex_t *shared_mutex2 = NULL;

const int SHARED_MEM_SIZE = sizeof(pthread_mutex_t);
char *ptr;
int * ptr2;
const char *name = "OS";
const char *name2 = "mutex1";
const char *name3 = "IPC with Standby";
const char *name4 = "mutex with Standby";
int processes = 0;
int shm_fd;
char* shm_ptr;

void sig_handler(int signal) {
    if (signal == SIGUSR1) {
		
		pthread_mutex_lock(shared_mutex);

		if(*ptr != '\0'){
			printf("\n----------------------------------------PROCESS REGENERATED!----------------------------------------\n\n");
			processes--;
		}

		pthread_mutex_unlock(shared_mutex);

    }
}


void mutexWrite()
{
	int fd = shm_open(name2, O_CREAT | O_TRUNC | O_RDWR , S_IRUSR | S_IWUSR);
	ftruncate(fd, SHARED_MEM_SIZE);
	shared_mutex = (pthread_mutex_t *)mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(shared_mutex, &attr);

	int fd2;
	do{
		fd2 = shm_open(name4, O_RDWR , S_IRUSR | S_IWUSR);
	}
	while(fd2 < 0);

	shared_mutex2 = (pthread_mutex_t *)mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);

}

void *keepCount(void * arg){

	char * check;
	int numCommands = -1;
	
	while(*ptr != '\0'){

		int count=0;
		pthread_mutex_lock(shared_mutex);
		check=ptr;
		
		while(*check != '\0')
		{
			if(*check == '\n'){
				count++;
			}
			check++;
		}

		if(numCommands == -1){
			numCommands = count;
		}
		else if(numCommands - count >= 10){
			numCommands = count;

			fflush(stdout);
			printf("\n-------------------------------- Number of remaining commands: %d --------------------------------\n\n", numCommands);
			fflush(stdout);

		}

		pthread_mutex_unlock(shared_mutex);
		
	}

	pthread_mutex_lock(shared_mutex2);
	close(shm_fd);
	shm_unlink(name3);
	pthread_mutex_unlock(shared_mutex2);
	
	pthread_exit(0);
}

void *read_shm(void *c)
{
	int *arr = (int*)c;
	
	char* command;
	char* end = ptr;
	
	// loop till end
	while (*end != '\0')
	{
		pthread_mutex_lock(shared_mutex);
        
        if(*end == '\0'){		//after entering, the process realises that the commands are finished
			pthread_mutex_unlock(shared_mutex);
			break;
        }

		// printf("Process %d, Thread %d is Retrieving\n", arr[0], arr[1]);
		
		while (*end != '\n'){
			end++;
		}
		
		command = (char*)malloc(end - ptr + 1);
		strncpy(command, ptr, end - ptr);
		command[end - ptr] = '\0';
		printf("Process %d, Thread %d is Running: %s\n\n", arr[0], arr[1], command);
		
		end++;
		sprintf(ptr, "%s", end);
		end = ptr;

		// printf("Process %d, Thread %d is Saving\n\n", arr[0], arr[1]);

		if(!strcmp(command, "throw error\0")){	//custom erorr to showcase Fault Tolerance
			printf("\n----------------------------------------ERROR THROWN at %d, pid: %d----------------------------------------\n\n", arr[0], getpid());
			free(command);

			pthread_mutex_lock(shared_mutex2);

			pid_t* temp = (pid_t*)shm_ptr;
	
			for(int i=1; i<5; i++){

				if(*(temp+i) == getpid()){
					*(temp+i) = (pid_t)0;
				}
			}

			pthread_mutex_unlock(shared_mutex2);

			pthread_mutex_unlock(shared_mutex);
			kill(getpid(), SIGTERM);
		}

		pthread_mutex_unlock(shared_mutex);


		//system(command);

		free(command);

		sleep(1);

	}

	printf("EOF reached by Process %d, Thread %d.\n", arr[0], arr[1]);

	pthread_exit(0);
}


int main(){

	mutexWrite();

	do{
		shm_fd = shm_open(name3, O_RDWR, 0666);	
	}while(shm_fd < 0);
	
	pthread_mutex_lock(shared_mutex2);
	
	void* mem = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	pthread_mutex_unlock(shared_mutex2);

	shm_ptr = (char*)mem;
	
	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

	pthread_mutex_lock(shared_mutex2);

	pid_t *parent_ptr = (pid_t*) (shm_ptr);
	*parent_ptr = getpid();
	
	pthread_mutex_unlock(shared_mutex2);
	

	pid_t pid;

	int fd;
	fd = shm_open(name, O_RDWR, 0666);
	ptr = (char *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	
	pthread_t tid[4];	//change to array
	pthread_t tid2;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	bool flag = false;
	int total_processes_created = 0;

	for(int i=0; i<4; i++){
	
		pid = fork();

		if (pid < 0)
		{
			fprintf(stderr, "Fork failed\n");
			return 1;
		}
		else if (pid == 0)
		{
			
			pthread_mutex_lock(shared_mutex2);

			printf("PID%d %d\n", processes+1, getpid());
			fflush(stdout);
			pid_t *pidptr = (pid_t*) (shm_ptr + (processes+1)*sizeof(pid_t));
			*pidptr = getpid();

			pthread_mutex_unlock(shared_mutex2);

			int j;
			int arr[4][2];
			
			sleep(1);

			for(j=0; j<4; j++){
				arr[j][0] = total_processes_created+1;
				arr[j][1] = j+1;

				pthread_create(&tid[j], &attr, read_shm, (void*)arr[j]);
			}
			
			
			for(j=0; j<4; j++)
				pthread_join(tid[j], NULL);

			exit(0);	// change to break;
		}
		else{
			processes++;
			total_processes_created++;

			if(!flag){		//only one thread is needed to keep count of number of remaining commands
				flag = true;
				
				pthread_create(&tid2, &attr, keepCount, NULL);
			}
			printf("\nParent process with pid %d created child process with pid %d\n", getpid(), pid);
		}
		
	}

	// waiting for processes
	for (int i = 0; i < total_processes_created; i++){
		wait(NULL);

		// examine if a child exhibited strange behavior
		if(*ptr == '\0'){
			break;
		}

		while(processes >= 4 && *ptr != '\0');

		for(; processes < 4; ){

			pid = fork();

			if (pid < 0)
			{
				fprintf(stderr, "Fork failed\n");
				return 1;
			}
			else if (pid == 0)
			{
				
				pthread_mutex_lock(shared_mutex2);

				printf("----------------------------------------PID %d HAS BEEN CREATED TO REPLACE THE OLD PROCESS----------------------------------------\n\n", getpid());
				fflush(stdout);

				pid_t* temp = (pid_t*)shm_ptr;
				for(int i=1; i<5; i++){
					if(*(temp+i) == 1){
						*(temp+i) = getpid();
						break;
					}
				}

				// pid_t *pidptr = (pid_t*) (shm_ptr + (processes+1)*sizeof(pid_t));
				// *pidptr = getpid();

				pthread_mutex_unlock(shared_mutex2);

				int j;
				int arr[4][2];

				for(j=0; j<4; j++){
					arr[j][0] = total_processes_created+1;
					arr[j][1] = j+1;

					pthread_create(&tid[j], &attr, read_shm, (void*)arr[j]);
				}
				
				
				for(j=0; j<4; j++)
					pthread_join(tid[j], NULL);

				exit(0);
			}
			else{
				processes++;
				total_processes_created++;
			}
		}
	}
		

	pthread_join(tid2, NULL);

	close(fd);
	shm_unlink(name);

	pthread_mutex_destroy(shared_mutex);

	sleep(1);
	printf("\nPARENT Terminated\n");
	fflush(stdout);
	
}