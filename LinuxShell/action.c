#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "action.h"
#include "meta_action.h"

char ** convert_argv_to_null_terminated(int argc, char **argv) {
    char ** arguments = malloc((argc + 1) * sizeof(char *));
    for(int i = 0; i < argc; i++) {
        arguments[i] = malloc(strlen(argv[i]) * sizeof(char));
        strcpy(arguments[i], argv[i]);
    }
    arguments[argc] = NULL;

    return arguments;
}

void signal_handler(int signum) {
    switch(signum) {
        case SIGINT:
            printf("SIGINT caught!\n");
            break;
        case SIGTSTP:
            printf("SIGTSTP caught!\n");
            break;
        default:
            printf("Unknown signal?");
    }
}

int wait_handler() {
    // Handle SIGINT and SIGTSTP
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));

    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTSTP, &action, NULL);

    int status = 0;

    // waiting for child to terminate
    pid_t res = wait(&status);

    // Caught an interrupt, we have to wait for the process to end
    while(res == -1) {
        res = wait(&status);
    }

    // Revert the signal handler to the default state
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    return status;
}

int execute_process(int argc, char **argv, int stdin_fd, int stdout_fd, int * pipe_in, int * pipe_out) {
    // Consistency checks
    if(pipe_in != NULL) {
        if(pipe_in[0] != stdin_fd && pipe_in[1] != stdin_fd) {
            fprintf(stderr, "Consistency check failure, stdin_fd couldn't be found in pipe_in!\n");
            return -1;
        }
    }
    if(pipe_out != NULL) {
        if(pipe_out[0] != stdout_fd && pipe_out[1] != stdout_fd) {
            fprintf(stderr, "Consistency check failure, stdout_fd couldn't be found in pipe_out!\n");
            return -1;
        }
    }

    // Create the null-terminated argv
    char ** arguments = convert_argv_to_null_terminated(argc, argv);

    // Fork this process
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "Failed forking child..\n");
        return -1;
    }

    if (pid == 0) {
        // Duplicate the custom input file descriptor
        if(stdin_fd != STDIN_FILENO) {
            dup2(stdin_fd, STDIN_FILENO);

            // Close the pipe if we're using one
            if(pipe_in != NULL) {
                close(pipe_in[0]);
                close(pipe_in[1]);
            }
        }

        // Duplicate the custom output file descriptor
        if(stdout_fd != STDOUT_FILENO) {
            dup2(stdout_fd, STDOUT_FILENO);

            // Close the pipe if we're using one
            if(pipe_out != NULL) {
                close(pipe_out[0]);
                close(pipe_out[1]);
            }
        }

        // Check whether both the pipes are closed before calling execvp
        if(pipe_in != NULL && (fcntl(pipe_in[0], F_GETFD) == 0 || fcntl(pipe_in[1], F_GETFD) == 0)) {
            fprintf(stderr, "The input pipe is not properly closed, cannot execute child process!\n");
            exit(EXIT_FAILURE);
        }
        if(pipe_out != NULL && (fcntl(pipe_out[0], F_GETFD) == 0 || fcntl(pipe_out[1], F_GETFD) == 0)) {
            fprintf(stderr, "The output pipe is not properly closed, cannot execute child process!\n");
            exit(EXIT_FAILURE);
        }

        // Execute the process
        if (execvp(arguments[0], arguments) < 0) {
            fprintf(stderr, "Could not execute command!\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int execute_action(action_t * action_graph) {
    if(action_graph == NULL)
        return -1;

    if(action_graph->action_type >= ACTION_COUNT)
        return -1;

    // The NOP action is considered to return true
    if(action_graph->action_type == ACTION_NOP)
        return 0;

    // TRUE action
    if(action_graph->action_type == ACTION_TRUE)
        return 0;

    // FALSE action
    if(action_graph->action_type == ACTION_FALSE)
        return -1;

    // EXEC_AND action
    if(action_graph->action_type == ACTION_EXEC_AND) {
        if(action_graph->action_count != 2)
            return -1;

        int left_retval = execute_action(action_graph->actions[0]);
        if(left_retval < 0)
            return -1;

        int right_retval = execute_action(action_graph->actions[1]);
        if(right_retval < 0)
            return -1;

        return 0;
    }

    // EXEC_OR action
    if(action_graph->action_type == ACTION_EXEC_OR) {
        if(action_graph->action_count != 2)
            return -1;

        int left_retval = execute_action(action_graph->actions[0]);
        if(left_retval == 0)
            return 0;

        int right_retval = execute_action(action_graph->actions[1]);
        if(right_retval == 0)
            return 0;

        return -1;
    }

    // EXEC_PIPE action
    if(action_graph->action_type == ACTION_EXEC_PIPE) {
        // We're only going to use a max of 2 pipes at any given point.
        // Having only the number of needed pipes open at any time is a bit
        // harder to manage, but given that we're fork()-ing the parent process,
        // and that execute_process() only closes the pipes it uses, we could
        // very easily end up with unclosed pipes, and therefore, hanging children processes.

        // pipes[i][0] -> read; pipes[i][1] -> write
        int pipes[2][2];
        int pipe_count = 0;

        // Open the first pipe
        if (pipe(pipes[pipe_count]) < 0) {
            fprintf(stderr,"\nPipe could not be initialized");
            return -1;
        }
        pipe_count++;

        // Start the first process
        int first_process_status = execute_process(
                action_graph->actions[0]->argc,
                action_graph->actions[0]->argv,
                STDIN_FILENO,
                pipes[0][1],
                NULL,
                pipes[0]
                );
        if(first_process_status < 0) {
            // Close all the open pipes
            while(pipe_count > 0) {
                close(pipes[pipe_count - 1][0]);
                close(pipes[pipe_count - 1][1]);
                pipe_count--;
            }

            return first_process_status;
        }

        for(int i = 1; i < action_graph->action_count - 1; i++) {
            // Open a new pipe
            if (pipe(pipes[pipe_count]) < 0) {
                fprintf(stderr,"\nPipe could not be initialized");
                return -1;
            }
            pipe_count++;

            int this_process_status = execute_process(
                    action_graph->actions[i]->argc,
                    action_graph->actions[i]->argv,
                    pipes[0][0],
                    pipes[1][1],
                    pipes[0],
                    pipes[1]
            );
            if(this_process_status < 0) {
                // Close all the open pipes
                while(pipe_count > 0) {
                    close(pipes[pipe_count - 1][0]);
                    close(pipes[pipe_count - 1][1]);
                    pipe_count--;
                }

                return this_process_status;
            }

            // Close the older pipe of the two currently opened ones
            close(pipes[0][0]);
            close(pipes[0][1]);
            pipes[0][0] = pipes[1][0];
            pipes[0][1] = pipes[1][1];
            pipe_count--;
        }

        int last_process_status = execute_process(
                action_graph->actions[action_graph->action_count - 1]->argc,
                action_graph->actions[action_graph->action_count - 1]->argv,
                pipes[0][0],
                STDOUT_FILENO,
                pipes[0],
                NULL
        );
        if(last_process_status < 0) {
            // Close all the open pipes
            while(pipe_count > 0) {
                close(pipes[pipe_count - 1][0]);
                close(pipes[pipe_count - 1][1]);
                pipe_count--;
            }

            return last_process_status;
        }

        // Close all the remaining pipes (should only be one at this point)
        while(pipe_count > 0) {
            close(pipes[pipe_count - 1][0]);
            close(pipes[pipe_count - 1][1]);
            pipe_count--;
        }

        // Wait for all the processes
        int status = 0;
        for(int i = 0; i < action_graph->action_count; i++) {
            int exit_status = wait_handler();
            if(exit_status != 0)
                status = -1;
        }

        return status;
    }

    // EXEC_FILE_REDIRECT action
    if(action_graph->action_type == ACTION_EXEC_FILE_REDIRECT) {
        int stdin_fd = STDIN_FILENO;
        int stdout_fd = STDOUT_FILENO;

        if(action_graph->stdin_path != NULL) {
            stdin_fd = open(action_graph->stdin_path, action_graph->stdin_mode);
            if(stdin_fd == -1) {
                fprintf(stderr, "Could not open file '%s' for reading: %s\n", action_graph->stdin_path, strerror(errno));
                return -1;
            }
        }

        if(action_graph->stdout_path != NULL) {
            stdout_fd = open(action_graph->stdout_path, action_graph->stdout_mode);
            if(stdout_fd == -1) {
                fprintf(stderr, "Could not open file '%s' for writing: %s\n", action_graph->stdout_path, strerror(errno));
                return -1;
            }
        }

        int init_status = execute_process(action_graph->argc, action_graph->argv, stdin_fd, stdout_fd, NULL, NULL);
        int retval = wait_handler();

        if(stdin_fd != STDIN_FILENO)
            close(stdin_fd);

        if(stdout_fd != STDOUT_FILENO)
            close(stdout_fd);

        return retval;
    }

    // EXEC action
    if(action_graph->action_type == ACTION_EXEC) {
        if(is_meta_action(action_graph))
            return execute_meta_action(action_graph);

        int init_status = execute_process(action_graph->argc, action_graph->argv, STDIN_FILENO, STDOUT_FILENO, NULL, NULL);
        int retval = wait_handler();

        return retval;
    }

    return -1;
}
