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
const int SHM_SIZE = 10000;
const char *SHM_NAME = "OS";

FILE *fptr;
int shm_fd;
char *ptr;
int lines = 0;

//Function to open the text file (commands.txt)
void openFile()
{
    fptr = fopen("commands.txt", "r");
    if(fptr == NULL)
    {
        printf("Error in opening file (commands.txt)\n");
    }
}

//Function to open the Shared Memory
void openSHM()
{
    shm_fd = shm_open(SHM_NAME,O_CREAT | O_RDWR,0666);
    ftruncate(shm_fd, SHM_SIZE);
    ptr = (char *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
}

//Function to calculate the number of lines in the text file (commands.txt)
void NoOfLinesCalculator()
{   
    int ch;
    while(EOF != (ch = getc(fptr)))
    {
        if(ch == '\n')
            lines++;
    }
    fseek(fptr, 0, SEEK_SET);
    printf("Lines: %d\n", lines);
}

//Function to read data from the text file using 4 threads
void *readFromFile(void *args)
{
    int thread_id = *((int *) args);
    int start = thread_id * (lines / num_threads);
    int end = start + (lines / num_threads);

    char line[max_line_size];
    
    for(int i=start; i<end; i++)
    {
        fgets(line, max_line_size, fptr);
        sprintf(ptr, "%s\n", line);
        ptr += strlen(line);
    }
}

int main()
{
    openFile();
    openSHM();
    NoOfLinesCalculator();
    pthread_t threads[num_threads];       
    int thread_args[num_threads];
    
    for(int i=0; i<num_threads; i++)
    {
        thread_args[i] = i;
        pthread_create(&threads[i], NULL, readFromFile, (void *)&thread_args[i]);    //Creating Threads
     }

    for(int i=0; i<num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // printing out the remainder lines (in case the number of commands are not divisible by num_threads)
    char line[max_line_size];
    while(fgets(line, max_line_size, fptr)){
        sprintf(ptr, "%s\n", line);
        ptr += strlen(line);
    }

    ptr++;
    *ptr = '\0';

    close(shm_fd);        //closing the shared memory
    fclose(fptr);          //closing the file pointer
    return 0;
}
