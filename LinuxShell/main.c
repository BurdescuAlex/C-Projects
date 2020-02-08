#include <stdio.h>
#include <string.h>

#include "console_input.h"
#include "input_parser.h"
#include "action.h"

#define clear() printf("\033[H\033[J")

int action_exit_requested = 0;

void init_shell() {
    clear();
    printf(
        "\033[96;1mShelly version 1.0 - The basic command shell\033[0m\n"
        "(C) 20 20 Burdescu Alexandru & Danciu Paul-Tiberiu\n"
        "\n"
    );
}

void uninit_shell() {
    printf("Bye!\n");
}

int main() {
    init_shell();

    while (!action_exit_requested) {
        char user_input[1024];

        // Read the user's input
        if (read_console_input(user_input))
            continue;

        // Parse the input into the action graph
        action_t action_graph;
        int parse_result = parse_input(&action_graph, user_input, strlen(user_input));
        if(parse_result < 0)
            continue;

        // Execute the requested action
        int retval = execute_action(&action_graph);
        if(retval != 0)
            fprintf(stderr,"Job returned non-zero value: %d\n", retval);
    }

    uninit_shell();
    return 0;
}
