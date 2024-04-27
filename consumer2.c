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
#include<stdbool.h>
// #define NUM_OF_PROCESSES 4
// #define DELAY 2

// #define SIZE 4096
pthread_mutex_t *shared_mutex = NULL;
pthread_mutex_t *shared_mutex2 = NULL;

const int SHARED_MEM_SIZE = sizeof(pthread_mutex_t);
char *ptr;
int * ptr2;
const char *name = "OS";
const char *name3 = "mutex1";
const char *name4 = "mutex2";
const char * name5 = "num";

void mutexWrite()
{
	int shm_fd;
	shm_fd = shm_open(name3, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
	ftruncate(shm_fd, SHARED_MEM_SIZE);
	shared_mutex = (pthread_mutex_t *)mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(shared_mutex, &attr);

	int fd = shm_open(name4, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
	ftruncate(fd, SHARED_MEM_SIZE);
	shared_mutex2 = (pthread_mutex_t *)mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	pthread_mutexattr_t attr2;
	pthread_mutexattr_init(&attr2);
	pthread_mutexattr_setpshared(&attr2, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(shared_mutex2, &attr2);
}



void *keepCount(void * arg){
	
	char * check;
	int numCommands=-1;
	while(1){
		int count=0;
		pthread_mutex_lock(shared_mutex);
		check=ptr;
		
		while(*check!='\0')
		{
			if(*check=='\n'){
				count++;
			}
		
			
		
		}
		pthread_mutex_unlock(shared_mutex);
		if(numCommands==-1){
			numCommands=count;
		}
		else if(numCommands-count>=5){
			printf("Number of remaining commands: %d\n", numCommands);
		
		}
	
	
	
	
	
	
	
	
	}
	
		



}

void *read_shm(void *c)
{
	int *arr = (int*)c;
	//int arr = (int)c;
	char* command;
	char* end = ptr;
	
	// loop till end
	while (*end != '\0')
	{
		pthread_mutex_lock(shared_mutex2);
		//printf("Process %d is Retrieving\n", arr);
		printf("Thread %d from process %d is Retrieving\n", arr[1], arr[0]);
		
		while (*end != '\n')
		{
			end++;
		}
		
		command = (char*)malloc(end - ptr + 1);
		strncpy(command, ptr, end - ptr);
		command[end - ptr] = '\0';
		printf("Running: %s\n", command);
		
		end++;
		sprintf(ptr, "%s", end);
		end = ptr;
		//printf("Process %d is Saving\n", arr);
		printf("Thread %d from process %d is Saving\n", arr[1], arr[0]);
		//from here
		
		
		//to here
		pthread_mutex_unlock(shared_mutex2);
		//system(command);
		free(command);

		sleep(0.5);
	}

	printf("EOF reached by process %d thread %d\n", arr[0], arr[1]);
	//printf("EOF reached by process %d\n", arr);
	pthread_exit(0);
}

int main()
{

	mutexWrite();

	const int SIZE = 4096;
	int fd;
	fd = shm_open(name, O_RDWR, 0666);
	ptr = (char *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //close(fd);
	int i = 0;
	pid_t pid;
	pthread_t tid[4];	//change to array
	pthread_t tid2;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	bool flag = false;
	for (i = 0; i < 4; i++)
	{
		pid = fork();
		// printf("\n%d\n", pid);
		if (pid < 0)
		{
			fprintf(stderr, "Fork failed\n");
			return 1;
		}
		else if (pid == 0)
		{
			int j;
			int arr[2];
			for(j=0; j<4; j++){
				arr[0]=i;
				arr[1]=j;
				pthread_create(&tid[i], &attr, read_shm, (void*)arr);
				//pthread_create(&tid, &attr, read_shm, (void*)i);
			}
			
			
			for(j=0; j<4; j++)
			pthread_join(tid[i], NULL);

			exit(0);	// change to break;
		}
		else
		{
			/*if(!flag){
				pthread_create(&tid2, NULL, keepCount, NULL);
				flag=true;
				pthread_join(tid2, NULL);
			
			}
			*/
			printf("\nParent process with pid %d created child process with pid %d\n", getpid(), pid);
		}
	}

	for (int i = 0; i < 4; i++)
		wait(NULL);

	shm_unlink(name);
    
	if (shared_mutex2)
	{
		pthread_mutex_destroy(shared_mutex2);
	}

	printf("\nFinally\n");

	return 0;
}
