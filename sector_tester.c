#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#define BIG 3200


void make_file(char* path, int size) {
    char big_data[BIG];

    for(int i = 0; i < size; i++)
        big_data[i] = 'a';
    
    int fd = open(path, O_CREATE | O_RDWR);
    write(fd, big_data, strlen(big_data));
    close(fd);

}

void print_sectors(char* path) {
    uint storing_addresses[13];
    int fd = open(path, O_RDONLY);
    get_file_sectors(fd, storing_addresses);

    printf(2, "%s: ", path);
    for(int i = 0; i < 13; i++)
        printf(2, " %d,", storing_addresses[i]);
    printf(1, "\n");
}

int main(int argc, char *argv[]){
    make_file("sector_file2.txt", BIG);
    make_file("sector_file3.txt", BIG - 512);

    print_sectors("sector_file.txt");
    print_sectors("sector_file1.txt");
    print_sectors("sector_file2.txt");
    print_sectors("sector_file3.txt");
    exit();
} 
