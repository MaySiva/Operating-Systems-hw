#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>

/* This function is the signal handler. It determines the behaviour of SIGINT to all processes*/
void signal_handler(int signum)
{
    if (signum == 0) // We want the default SIGINT
    {
        struct sigaction new_action = {.sa_handler = SIG_DFL};
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &new_action, 0) == -1)
        {
            fprintf(stderr, "An error has occurred : signal handle faild, Error: %s\n", strerror(errno));
            exit(1);
        }
    }
    if (signum == 1) // We want to ignore SIGINT
    {
        struct sigaction new_action = {.sa_handler = SIG_IGN};
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &new_action, 0) == -1)
        {
            fprintf(stderr, "An error has occurred : signal handle faild, Error: %s\n", strerror(errno));
            exit(1);
        }
    }

    if (signum == 3) // Avoid zombies when gets SIGCHLD
    {
        struct sigaction new_action = {.sa_handler = SIG_DFL};
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_RESTART | SA_NOCLDWAIT; // ERAN'S TRICK, We want to prevent zombies
        if (sigaction(SIGCHLD, &new_action, 0) == -1)
        {
            fprintf(stderr, "An error has occurred : signal handle faild, Error: %s\n", strerror(errno));
            exit(1);
        }
    }
}

/* The prepare function determines the behaviour of the processes to SIGINT*/
int prepare(void)
{
    signal_handler(1);
    signal_handler(3);
    return 0;
}
/* This function gets the arglist, it's length(-1 because arglist[count+1]=NULL)
 and returns the index of the symbol "|" or ">" in arglist . If the symbol doesnt found in arglist, returns -1 */

int check_case(int count, char **arglist, char *symbol)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(arglist[i], symbol) == 0)
        {
            return i;
        }
    }
    return -1;
}

/*This function executes a process with pipe*/

int process_needs_pipe(int count, char **arglist, int index_of_pipe)
{
    int pid_first, pid_second, pipefd[2], status, res_of_waitpid;
    arglist[index_of_pipe] = NULL;
    if (pipe(pipefd) == -1)
    {
        fprintf(stderr, "An error has occurred : pipe() faild, Error: %s\n", strerror(errno));
        return 0;
    }

    pid_first = fork();
    if (pid_first < 0)
    {
        fprintf(stderr, "An error has occurred : fork() faild, Error: %s\n", strerror(errno));
        return 0;
    }
    if (pid_first == 0) // First child
    {
        signal_handler(0);
        close(pipefd[0]); // Close unused side
        if (dup2(pipefd[1], 1) == -1)
        {
            fprintf(stderr, "An error has occurred : dup2() faild, Error: %s\n", strerror(errno));
            exit(1);
        }

        close(pipefd[1]);            // Close unused side
        execvp(arglist[0], arglist); // First command
        fprintf(stderr, "An error has occurred : execvp() faild, Error: %s\n", strerror(errno));
        exit(1);
    }

    pid_second = fork(); // The Father does this fork()
    if (pid_second < 0)
    {
        fprintf(stderr, "An error has occurred : fork() faild, Error: %s\n", strerror(errno));
        return 0;
    }

    if (pid_second == 0) // Second child
    {
        signal_handler(0);
        close(pipefd[1]); // Close unused side
        if (dup2(pipefd[0], 0) == -1)
        {
            fprintf(stderr, "An error has occurred : dup2() faild, Error: %s\n", strerror(errno));
            exit(1);
        }

        close(pipefd[0]);                                 // Close unused side
        char **arglist_sec = arglist + index_of_pipe + 1; // The indexes of the second command after the pipe |
        execvp(arglist_sec[0], arglist_sec);              // Second Command

        fprintf(stderr, "An error has occurred : execvp() faild, Error: %s\n", strerror(errno));
        exit(1);
    }

    else // The Father
    {
        close(pipefd[0]); // We don't need this anymore
        close(pipefd[1]); // We don't need this anymore

        res_of_waitpid = waitpid(pid_first, &status, 0);
        if (errno != ECHILD && errno != EINTR && res_of_waitpid == -1)
        {

            fprintf(stderr, "An error has occurred : waitpid() faild, Error: %s\n", strerror(errno));
            return 0;
        }

        res_of_waitpid = waitpid(pid_second, &status, 0);              // Waits the child
        if (errno != ECHILD && errno != EINTR && res_of_waitpid == -1) // Ignore those errors
        {

            fprintf(stderr, "An error has occurred : waitpid() faild, Error: %s\n", strerror(errno));
            return 0;
        }
    }

    return 1;
}
/*This function executes a process with the > command, it redirectes the output to a given file*/
int process_open_file(int count, char **arglist)
{

    int pid, res_of_waitpid, status;
    pid = fork();
    if (pid < 0) // fork() failed
    {
        fprintf(stderr, "An error has occurred : fork() faild, Error: %s\n", strerror(errno));
        return 0;
    }

    if (pid == 0)
    {
        signal_handler(0);
        const char *path_name = arglist[count - 1]; // The index of the file name

        int fd = open(path_name, O_RDWR | O_CREAT | O_TRUNC, 0777);
        if (fd == -1)
        {
            fprintf(stderr, "An error has occurred : open file faild, Error: %s\n", strerror(errno));
            exit(1);
        }
        arglist[count - 2] = NULL;
        if (dup2(fd, 1) == -1) // Redirectes the output to the given file
        {
            fprintf(stderr, "An error has occurred : dup2() faild, Error: %s\n", strerror(errno));
            exit(1);
        }

        close(fd); /// We don't need this anymore

        execvp(arglist[0], arglist); // The command (input)
        fprintf(stderr, "An error has occurred : execvp faild, Error: %s\n", strerror(errno));
        exit(1);
    }
    else // The Father
    {
        res_of_waitpid = waitpid(pid, &status, 0);                     // Waits the child
        if (errno != ECHILD && errno != EINTR && res_of_waitpid == -1) // Ignore those errors
        {

            fprintf(stderr, "An error has occurred : waitpid() faild, Error: %s\n", strerror(errno));
            return 0;
        }
    }
    return 1;
}

/*This function is the main function which checks the cases (&,|,>,regular) and executes the appropriate case*/
int process_arglist(int count, char **arglist)
{

    int pid;

    if (strcmp(arglist[count - 1], "&") == 0) // Case 1: run the child process in the backround
    {

        pid = fork();
        if (pid < 0) // fork() failed
        {
            fprintf(stderr, "An error has occurred : fork() faild, Error: %s\n", strerror(errno));
            return 0;
        }

        if (pid == 0)
        {
            signal_handler(1);         // We want to ignore SIGINT
            arglist[count - 1] = NULL; // We dont pass the & argument to execvp()
            execvp(arglist[0], arglist);
            fprintf(stderr, "An error has occurred : execvp faild, Error: %s\n", strerror(errno));
            exit(1);
        }
        return 1;
    }
    int index_pipe = check_case(count, arglist, "|");

    if (index_pipe != -1) // Case 2: Pipe is needed
    {
        return process_needs_pipe(count, arglist, index_pipe);
    }

    if (check_case(count, arglist, ">") != -1) // Case 3: Open file
    {

        return process_open_file(count, arglist);
    }

    else // Case 4: Regular case
    {

        int res_of_waitpid, status;
        pid = fork();
        if (pid < 0) // fork() failed
        {
            fprintf(stderr, "An error has occurred : fork() faild, Error: %s\n", strerror(errno));
            return 0;
        }

        if (pid == 0) // Child
        {

            signal_handler(0);
            execvp(arglist[0], arglist);
            fprintf(stderr, "An error has occurred : execvp faild, Error: %s\n", strerror(errno));
            exit(1);
        }
        else // Father
        {
            res_of_waitpid = waitpid(pid, &status, 0);                     // Waits the child
            if (errno != ECHILD && errno != EINTR && res_of_waitpid == -1) // Ignore those errors
            {

                fprintf(stderr, "An error has occurred : waitpid() faild, Error: %s\n", strerror(errno));
                return 0;
            }
        }
        return 1;
    }
}

/*This function returns 0 on sucess*/
int finalize(void)
{
    return 0;
}
