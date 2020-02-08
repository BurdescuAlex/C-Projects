#ifndef SHELLY_ACTION_H
#define SHELLY_ACTION_H

extern int action_exit_requested;

enum {
    ACTION_NOP,                 // Nullary
    ACTION_TRUE,                // Nullary
    ACTION_FALSE,               // Nullary
    ACTION_EXEC_AND,            // Binary
    ACTION_EXEC_OR,             // Binary
    ACTION_EXEC_PIPE,           // Binary or more
    ACTION_EXEC,                // Unary
    ACTION_EXEC_FILE_REDIRECT,  // Binary or ternary
    ACTION_COUNT,
};
typedef unsigned short action_type_t;

typedef struct action_ {
    // Action type
    action_type_t action_type;

    // Parameters
    int action_count;
    struct action_ ** actions;

    // Arguments
    int argc;
    char ** argv;

    // Custom file descriptors
    char * stdin_path;
    int stdin_mode;

    char * stdout_path;
    int stdout_mode;
} action_t;

int wait_handler();
int execute_process(int argc, char **argv, int stdin_fd, int stdout_fd, int * pipe_in, int * pipe_out);
int execute_action(action_t * action_graph);

#endif //SHELLY_ACTION_H
