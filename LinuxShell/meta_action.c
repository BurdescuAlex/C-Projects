#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "meta_action.h"

int action_exit(int argc, char ** argv) {
    action_exit_requested = 1;
    return 0;
}

int action_cd(int argc, char ** argv) {
    chdir(argv[1]);
    return 0;
}

int action_help(int argc, char ** argv) {
    printf("Welcome to Shelly!\n"
           "\n"
           "Available shell commands:\n"
           "\t- cd <dir>    Change directory\n"
           "\t- ls          List directory\n"
           "\t- exit        Exit the shell\n"
           "\t- tank        *Sunt ca un tanc!*\n");

    return 0;
}

int action_hello(int argc, char ** argv) {
    char * username = getenv("USER");
    printf("Hi, %s! Nice to meet you! Please use 'help' to see the available commands.\n", username);

    return 0;
}

int action_tank(int argc, char ** argv) {
    printf("       ___\n"
           " _____/_o_\\_____\n"
           "(==(/_______\\)==)\n"
           " \\==\\/     \\/==/\n");

    char * command[128] = {"xdg-open", "https://www.youtube.com/watch?v=qZAIfEp0AoQ"};
    execute_process(2, command, STDIN_FILENO, STDOUT_FILENO, NULL, NULL);
    wait_handler();

    return 0;
}

const meta_action_t meta_actions[] = {
        {"exit", action_exit},
        {"cd", action_cd},
        {"help", action_help},
        {"hello", action_hello},
        {"tank", action_tank},
};
const size_t meta_action_count = sizeof(meta_actions) / sizeof(meta_actions[0]);


int is_meta_action(action_t * action) {
    if(action == NULL)
        return 0;

    if(action->action_type != ACTION_EXEC)
        return 0;

    for(int i = 0; i < meta_action_count; i++)
        if(strcmp(meta_actions[i].command, action->argv[0]) == 0)
            return 1;

    return 0;
}

int execute_meta_action(action_t * action) {
    if(!is_meta_action(action))
        return -1;

    for (int i = 0; i < meta_action_count; i++)
        if (strcmp(meta_actions[i].command, action->argv[0]) == 0)
            return meta_actions[i].function(action->argc, action->argv);

    return -1;
}
