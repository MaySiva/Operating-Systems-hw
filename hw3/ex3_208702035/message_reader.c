#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "message_slot.h"

int main(int argc, char *argv[])
{
    /*argv[1] message slot file path
    argv[2] the target message channel id*/

    char buffer[MAX_MESSAGE_SIZE];
    if (argc != 3) // Validate that correct number of command line arguments is passed
    {
        fprintf(stderr, "An error has occurred , the number of commands is wrong, Error: %s\n", strerror(errno));
        exit(1);
    }
    int file_path = open(argv[1], O_RDONLY);
    if (file_path < 0)
    {

        fprintf(stderr, "An error has occurred , cant open file, Error: %s\n", strerror(errno));
        exit(1);
    }
    // 2 - set the channel id
    unsigned long channel_id = atoi(argv[2]);
    int res_of_ioctl = ioctl(file_path, MSG_SLOT_CHANNEL, channel_id);
    if (res_of_ioctl < 0)
    {
        fprintf(stderr, "An error has occurred , ioctl has failed, Error: %s\n", strerror(errno));
        exit(1);
    }

    int res = read(file_path, buffer, MAX_MESSAGE_SIZE);
    if (res < 0)
    {
        fprintf(stderr, "An error has occurred , read has failed Error: %s\n", strerror(errno));
        exit(1);
    }
    int res_write = write(1, buffer, res); // 1 for standart output
    if (res_write != res)
    {
        fprintf(stderr, "An error has occurred , write has failed Error: %s\n", strerror(errno));
        exit(1);
    }
    close(file_path);
    exit(0);
}