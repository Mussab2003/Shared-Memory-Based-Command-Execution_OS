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

#define SIZE 4096
#define NUM_OF_PROCESSES 4

int old_status[NUM_OF_PROCESSES];
int status[NUM_OF_PROCESSES];
pid_t children[NUM_OF_PROCESSES];
pid_t parent;
pthread_mutex_t lock;
pthread_mutex_t *lock2;
char* ptr;
int fd;

char name[] = "IPC with Standby";
char name2[] = "mutex with Standby";


void* fault_tolerance(void* args){

    int i = args;
    
    while(fd != -1){

        pthread_mutex_lock(lock2);
        children[i] = *((pid_t*)(ptr + (i+1)*sizeof(pid_t)));
        pthread_mutex_unlock(lock2);

        if (children[i] == 0) {  // error occurred
        
            pthread_mutex_lock(&lock);

            printf("\nSTANDBY: A Process has been terminated.\n\n");
            fflush(stdout);
            old_status[i] = 1;

            if (!kill(parent, 0) && (kill(parent, SIGUSR1) == -1)) {
                perror("Error in signal.\n");
            }
            
            
            *((pid_t*)(ptr + (i+1)*sizeof(pid_t))) = 1;
            
            pthread_mutex_unlock(&lock);
        }

        
        close(fd);

        fd = shm_open(name, O_RDWR, 0666);
    }

    pthread_exit(0);
}

void mutexWrite()
{
	int fd = shm_open(name2, O_CREAT | O_TRUNC | O_RDWR , S_IRUSR | S_IWUSR);
	ftruncate(fd, sizeof(pthread_mutex_t));
	lock2 = (pthread_mutex_t *)mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	pthread_mutexattr_t attr2;
	pthread_mutexattr_init(&attr2);
	pthread_mutexattr_setpshared(&attr2, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(lock2, &attr2);
}


int main(){

    pthread_mutex_init(&lock, NULL);

    mutexWrite();
    
    pthread_mutex_lock(lock2);

    fd = shm_open(name,  O_CREAT | O_TRUNC | O_RDWR, 0666);
	ftruncate(fd, 4096);
	void* mem = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    ptr = (char*)mem;
	
    pid_t* ptr_parent = ((pid_t*)ptr);
    *ptr_parent = 0;

    for(int i = 0; i<NUM_OF_PROCESSES; i++){
        *((pid_t*)(ptr + (i+1)*sizeof(pid_t))) = (pid_t)0;
    }

    pthread_mutex_unlock(lock2);

    for(int i = 0; i<NUM_OF_PROCESSES; i++){
        old_status[i] = 0;
    }

    pthread_t tid[NUM_OF_PROCESSES];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    bool flag = false;
    while(!flag){
        
        flag = true;

        pthread_mutex_lock(lock2);
        for(int i = 0; i<NUM_OF_PROCESSES; i++){
            children[i] = *((pid_t*)(ptr + (i+1)*sizeof(pid_t)));

            if(children[i] == 0 || *ptr_parent == 0){
                flag = false;
            }
        }
        pthread_mutex_unlock(lock2);

    }

    // After all children are loaded, continue
    parent = *ptr_parent;

    for(int i = 0; i<NUM_OF_PROCESSES; i++){
        pthread_create(&(tid[i]), &attr, fault_tolerance, (void*)i);
    }

    for(int i=0; i<NUM_OF_PROCESSES; i++){
        pthread_join(tid[i], NULL);
    }

	shm_unlink(name);
    shm_unlink(name2);
    
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(lock2);

}
