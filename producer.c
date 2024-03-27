#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main()
{

    const int SHM_SIZE = 4096;
    const char *SHM_NAME = "OS";
    ssize_t bytes_read;
    char buffer[5000];

    int shm_fd;
    int file_fd;

    file_fd = open("commands.txt", O_RDONLY);

    if(file_fd ==-1)
    {
        perror("Unable to open the file");
        exit(1);
    }

    char *ptr;

    shm_fd = shm_open(SHM_NAME,O_CREAT | O_RDWR,0666);

    ftruncate(shm_fd, SHM_SIZE);

    ptr = (char *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0)
    // {
    //     printf("%s", buffer);
    // }

    read(file_fd, buffer, sizeof(buffer));
    sprintf(ptr,"%s \n", buffer);
    
    close(shm_fd);
    
    if(close(file_fd) == -1)
    {
        perror("Unable to close file");
        exit(1);
    }
    return 0;
}