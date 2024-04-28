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
#define max_line_size 256
pthread_mutex_t mutex;
const int SHM_SIZE = 10000;
const char *SHM_NAME = "OS";

FILE *fptr;
int shm_fd;
char *ptr;
int lines = 0;
int j=0;

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

void NoOfLinesCalculator()
{   
    int ch;
    while(EOF != (ch = getc(fptr)))
    {
        if(ch == '\n')
            lines++;
    }
    fseek(fptr, 0, SEEK_SET);
}

void *readFromFile(void *args)
{
    printf("%d", lines);
    int thread_id = *((int *) args);
    int start = thread_id * (lines / num_threads);
    int end = start + (lines / num_threads);

    char line[max_line_size];

    pthread_mutex_lock(&mutex);
    for(int i=start; i<end; i++)
    {
        j++;
        fgets(line, max_line_size, fptr);
        printf("thread %d : %d: %s", thread_id, j, line);
        sprintf(ptr, "%s\n", line);
        ptr += strlen(line);
    }
    pthread_mutex_unlock(&mutex);

}

int main()
{
    openFile();
    openSHM();
    NoOfLinesCalculator();
    pthread_t threads[num_threads];
    int thread_args[num_threads];
    pthread_mutex_init(&mutex, NULL);

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

    pthread_mutex_destroy(&mutex);
    return 0;
}