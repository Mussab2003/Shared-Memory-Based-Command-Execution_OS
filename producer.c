#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>


#define num_threads 4

const int SHM_SIZE = 4096;
const char *SHM_NAME = "OS";

FILE *fptr;
char buffer[5000];
int shm_fd;
char *ptr;

void openFile()
{
    fptr = fopen("commands.txt", "r");
    if(fptr == NULL)
    {
        printf("Error in opening file (commands.txt)\n");
    }
}
    
void openSHM()
{
    shm_fd = shm_open(SHM_NAME,O_CREAT | O_RDWR,0666);
    ftruncate(shm_fd, SHM_SIZE);
    ptr = (char *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
}

void *readFromFile(void *args)
{
    int thread_id = *((int *) args);
    int start = thread_id * (100 / num_threads);
    int end = start + (100 / num_threads);

    for(int i=start; i<end; i++)
    {
        fgets(buffer, sizeof(buffer), fptr);
        printf("%s", buffer);
        sprintf(ptr, "%s\n", buffer);
        ptr += strlen(buffer);
    }

}

int main()
{
    openFile();
    openSHM();
    
    pthread_t threads[num_threads];
    int thread_args[num_threads];

    for(int i=0; i<num_threads; i++)
    {
        thread_args[i] = i;
        pthread_create(&threads[i], NULL, readFromFile, (void *)&thread_args[i]);
    }

    for(int i=0; i<num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    close(shm_fd);
    fclose(fptr);

    return 0;
}