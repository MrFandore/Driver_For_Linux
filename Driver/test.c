#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/mydriver"
#define BUFFER_SIZE 100

int main() {
    int fd;
    char buffer[BUFFER_SIZE];
    
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    
    printf("1. Reading from device:\n");
    int bytes = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes < 0) {
        perror("Read failed");
    } else {
        buffer[bytes] = '\0';
        printf("Read %d bytes: %s\n", bytes, buffer);
    }
    
    printf("\n2. Writing to device:\n");
    char *message = "Hello from user program!";
    bytes = write(fd, message, strlen(message));
    printf("Wrote %d bytes\n", bytes);
    
    printf("\n3. Reading again:\n");
    lseek(fd, 0, SEEK_SET);
    bytes = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes < 0) {
        perror("Read failed");
    } else {
        buffer[bytes] = '\0';
        printf("Read %d bytes: %s\n", bytes, buffer);
    }
    
    close(fd);
    return 0;
}