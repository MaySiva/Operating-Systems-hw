#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <wait.h>

#define BUFFER_SIZE 1024 * 1024 // 1MB

/* In this file I used Eran's example of server*/

int connect_fd = -1;
int connect_flag = 0;
uint32_t pcc_total[95]; // 95 values for b 32<=b<=126

/*The signal handler - defines the behaviour after SIGINT*/
void signal_handler()
{
    if (connect_fd < 0)
    {
        for (int i = 0; i < 95; i++)
        {
            printf("char '%c' : %u times\n", i + 32, pcc_total[i]);
        }
        exit(0);
    }
    connect_flag = 1;
}
/*This function updates for every character the number of times it was observed in the current content (in curr_pcc array).
This function returns the number of printable character in the current content.
*/

u_int32_t update_curr_pcc(int content, uint32_t *pcc_curr, char *contents_of_file)
{
    u_int32_t C_temp = 0;
    for (int i = 0; i < content; i++)
    {

        if ((contents_of_file[i]) >= 32 && (contents_of_file[i]) <= 126)
        {

            C_temp++;
            int j = (int)contents_of_file[i] - 32;
            pcc_curr[j]++;
        }
    }

    return C_temp;
}

/*The main function*/

int main(int argc, char *argv[])
{
    /*argv[1] server's port*/

    int listenfd = -1;
    // Initializes the pcc_total that will count how many times each printable character was observed in all client connections
    for (int i = 0; i < 95; i++)
    {
        pcc_total[i] = 0;
    }

    if (argc != 2) // Validate that correct number of command line arguments is passed
    {

        fprintf(stderr, "An error has occurred , the number of commands is wrong, Error: %s\n", strerror(errno));
        exit(1);
    }

    /*Defines signal handler for SIGINT*/

    struct sigaction new_action;
    new_action.sa_handler = &signal_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &new_action, 0) != 0)
    {
        fprintf(stderr, "An error has occurred in signal handler, Error: %s\n", strerror(errno));
        exit(1);
    }

    /*Creates socket*/

    struct sockaddr_in serv_addr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        fprintf(stderr, "An error has occurred , in socket function, Error: %s\n", strerror(errno));
        exit(1);
    }
    int opt_val = 1;
    int res = -1;
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
    if (res < 0)

    {
        fprintf(stderr, "An error has occurred in setsockopt function , Error: %s\n", strerror(errno));
        exit(1);
    }

    /*Server code*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY - any local machine addres.
    if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
    {
        fprintf(stderr, "An error has occurred in bind function , Error: %s\n", strerror(errno));
        exit(1);
    }

    if (0 != listen(listenfd, 10)) // Listen to incoming TCP connections.
    {
        fprintf(stderr, "An error has occurred in listen function , Error: %s\n", strerror(errno));
        exit(1);
    }



    while (1)
    {
        u_int32_t C = 0;
        uint32_t pcc_curr[95] = {0};

        if (connect_flag)
        {
            for (int i = 0; i < 95; i++)
            {
                printf("char '%c' : %u times\n", i + 32, pcc_total[i]); // Prints the count of each character.
            }
            exit(0);
        }

        connect_fd = accept(listenfd, NULL, NULL); // Accept a connection

        if (connect_fd < 0)
        {

            fprintf(stderr, "An error has occurred in accept function , Error: %s\n", strerror(errno));
            exit(1);
        }

        uint32_t N = 0, num_of_bytes = 0;

        uint32_t num_of_recv_bytes = read(connect_fd, &num_of_bytes, sizeof(num_of_bytes)); // Reads the N from the client

        if (num_of_recv_bytes < 0)
        {

            if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE)
            {
                fprintf(stderr, "An error has occurred in read function. Error: %s\n", strerror(errno));
                exit(1);
            }
            else // A client connection terminates due to TCP errors
            {
                fprintf(stderr, "An TCP error has occurred in read function . Error: %s\n", strerror(errno));
                close(connect_fd);
                connect_fd = -1;
                continue; // We dont need to exit
            }
        }

        if ((num_of_recv_bytes) != 4)
        {
            fprintf(stderr, "An error has occurred . Number of recv bytes is wrong, Error: %s\n", strerror(errno));
            close(connect_fd);
            connect_fd = -1;
            continue;
        }

        N = ntohl(num_of_bytes); // N is a 32- bit unsigned integer in network byte order

        char contents_of_file[BUFFER_SIZE] = {0};
        u_int32_t content = 0;
        u_int32_t sum_bytes = 0;
        if (N < BUFFER_SIZE)
        {

            content = (read(connect_fd, &contents_of_file, N)); // Reads the content of the file from the client.
            if (content < 0)
            {

                if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE)
                {
                    fprintf(stderr, "An error has occurred in sent function, Error: %s\n", strerror(errno));
                    exit(1);
                }

                else // A client connection terminates due to TCP errors
                {
                    fprintf(stderr, "An TCP error has occurred in read function . Error: %s\n", strerror(errno));
                    free(contents_of_file);
                    close(connect_fd);
                    connect_fd = -1;
                    continue;
                }
            }
            else if (content != N)
            {

                fprintf(stderr, "An error has occurred in read function . Error: %s\n", strerror(errno));
                close(connect_fd);
                connect_fd = -1;
                continue; // We dont need to exit
            }
            u_int32_t curr_C = update_curr_pcc(content, pcc_curr, contents_of_file);
            C += curr_C;
            sum_bytes += content;
        }
        else // N > BUFFER_SIZE , The file size is bigger than 1MB
        {

            while (sum_bytes < N && ((content = read(connect_fd, &contents_of_file, sizeof(contents_of_file))) > 0)) // While we can read more bytes from the file
            {

                u_int32_t curr_C = update_curr_pcc(content, pcc_curr, contents_of_file);
                C += curr_C;
                sum_bytes += content;
            }

            if (content < 0 && sum_bytes != N)
            {

                if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE)
                {
                    fprintf(stderr, "An error has occurred in read function, Error: %s\n", strerror(errno));
                    exit(1);
                }
                else // A client connection terminates due to TCP errors
                {
                    fprintf(stderr, "An TCP error has occurred in read function . Error: %s\n", strerror(errno));
                    free(contents_of_file);
                    close(connect_fd);
                    connect_fd = -1;
                    continue; // We dont need to exit
                }
            }
        }

        if (sum_bytes != N)
        {
            fprintf(stderr, "An error has occurred. Number of read bytes is wrong, Error: %s\n", strerror(errno));
            close(connect_fd);
            connect_fd = -1;
            continue;
        }

        uint32_t C_conv = htonl(C); // C is a 32- bit unsigned integer in network byte order

        int num_of_send_bytes = write(connect_fd, &C_conv, sizeof(C_conv)); // Writes C to the client.

        if (num_of_send_bytes < 0)
        {
            if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE)
            {
                fprintf(stderr, "An error has occurred in write function, Error: %s\n", strerror(errno));
                exit(1);
            }
            else // A client connection terminates due to TCP errors
            {

                close(connect_fd);
                connect_fd = -1;
                continue; // We dont need to exit
            }
        }

        if (num_of_send_bytes != sizeof(C_conv))
        {
            fprintf(stderr, "An error has occurred in write function, Error: %s\n", strerror(errno));
            close(connect_fd);
            connect_fd = -1;
            continue;
        }

        for (int i = 0; i < 95; i++)
        {
            pcc_total[i] += pcc_curr[i]; // Updates the pcc_total
        }
        close(connect_fd);
        connect_fd = -1;
    }
    exit(0);
}
