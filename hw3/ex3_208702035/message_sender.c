#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include "message_slot.h"

int main(int argc, char *argv[])
{
    /*argv[1] message slot file path
    argv[2] the target message channel id
    argv[3] the message to pass*/

    if (argc != 4) // Validate that correct number of command line arguments is passed
    {
        fprintf(stderr, "An error has occurred , the number of commands is wrong, Error: %s\n", strerror(errno));
        exit(1);
    }
    int file_path = open(argv[1], O_WRONLY);
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
        fprintf(stderr, "An error has occurred , ioctl failed, Error: %s\n", strerror(errno));
        exit(1);
    }
    int res = write(file_path, argv[3], strlen(argv[3]));
    if (res != strlen(argv[3])) // Checks that we write the correct number of bytes (strlen - without \0)
    {
        fprintf(stderr, "An error has occurred , Error: %s\n", strerror(errno));
        exit(1);
    }
    close(file_path);
    exit(0);
}