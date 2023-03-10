
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024 * 1024 // 1MB
int main(int argc, char *argv[])
{
    /*argv[1] server's IP address
    argv[2] server's port
    argv[3] path of the file to send */

    if (argc != 4) // Validates that correct number of command line arguments is passed.
    {
        fprintf(stderr, "An error has occurred , the number of commands is wrong, Error: %s\n", strerror(errno));
        exit(1);
    }
    FILE *file;
    // Opens the specified file for reading.
    file = fopen(argv[3], "rb");
    if (file == NULL)
    {
        fprintf(stderr, "An error has occurred in fopen function. Error: %s\n", strerror(errno));
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    uint32_t file_size = ftell(file);
    fseek(file, 0, SEEK_END);

    // Creates a TCP connection to the specified server port on the specified server IP.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "An error has occurred in socket function, Error: %s\n", strerror(errno));
        exit(1);
    }
    /*Server code*/
    struct sockaddr_in serv_addr; // Where we want to get to.

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "An error has occurred , in connect function, Error: %s\n", strerror(errno));
        exit(1);
    }

    uint32_t num_of_bytes = htonl(file_size); // N = num_of_bytes ,  N is a 32- bit unsigned integer in network byte order.

    size_t num_of_bytes_sent = 0;

    num_of_bytes_sent = (write(sockfd, &num_of_bytes, sizeof(num_of_bytes))); // Writes N to server.

    if (num_of_bytes_sent < 0)
    {
        fprintf(stderr, "An error has occurred in write function , Error: %s\n", strerror(errno));
        exit(1);
    }
    if (num_of_bytes_sent < sizeof(num_of_bytes))
    {
        fprintf(stderr, "An error has occurred. Number of sent bytes is wrong, Error: %s\n", strerror(errno));
        exit(1);
    }

    size_t content = 0;
    size_t sum_bytes = 0;
    if (file_size < BUFFER_SIZE)
    {
        char *contents_of_file = malloc(file_size);
        rewind(file);
        sum_bytes = fread(contents_of_file, sizeof(char), file_size, file); // Reads the content of the file.

        if (sum_bytes != file_size)
        {
            fprintf(stderr, "An error has occurred in fread function, Error: %s\n", strerror(errno));
            exit(1);
        }

        content = write(sockfd, contents_of_file, file_size); // Writes the content of the file to the server.

        if (content < 0)
        {
            free(contents_of_file);
            fprintf(stderr, "An error has occurred in write function, Error: %s\n", strerror(errno));
            exit(1);
        }
        free(contents_of_file);
    }

    else // file_size > BUFFER_SIZE
    {
        char contents_of_file[BUFFER_SIZE];
        rewind(file);

        while ((sum_bytes = fread(contents_of_file, sizeof(char), BUFFER_SIZE, file)) > 0) // Reads 1MB.

        {

            content = write(sockfd, contents_of_file, sum_bytes); // Writes the current content to the server.

            if (content < 0)
            {
                fclose(file);
                fprintf(stderr, "An error has occurred in write function, Error: %s\n", strerror(errno));
                exit(1);
            }
        }
        fclose(file);
    }


      u_int32_t buffer_of_C={0};
 

    u_int32_t num_of_recv_bytes = read(sockfd, &buffer_of_C, sizeof(buffer_of_C)); // Reads the C from the server.

    if (num_of_recv_bytes < 0)
    {
        fprintf(stderr, "An error has occurred in read function. Error: %s\n", strerror(errno));
        exit(1);
    }

    if (num_of_recv_bytes != sizeof(num_of_bytes))
    {
        fprintf(stderr, "An error has occurred . Number of read bytes is wrong, Error: %s\n", strerror(errno));
        exit(1);
    }

    uint32_t C= ntohl(buffer_of_C); // C is a 32- bit unsigned integer in network byte order.
    printf("# of printable characters: %u\n", C);
    close(sockfd);
    exit(0);
}