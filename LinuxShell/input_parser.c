#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "input_parser.h"

const char * operators[] = {
        "&&",   // AND
        "||",   // OR

        "|",    // PIPE
        ">>",   // REDIRECT_APPEND
        ">",    // REDIRECT
        "<",    // INPUT
};
const int operator_count = sizeof(operators) / sizeof(operators[0]);

int is_whitespace(const char c) {
    return (c == ' ' || c == '\t');
}

// Does this character mark the start of a skip sequence?
int is_skip_seq(const char c) {
    return (c == '(' || c == '"' || c == '\'');
}

int starts_with(const char * haystack, const char * needle) {
    if(strlen(haystack) < strlen(needle))
        return 0;

    return strncmp(haystack, needle, strlen(needle)) == 0;
}

int starts_with_operator(const char * str) {
    for(int i = 0; i < operator_count; i++)
        if(starts_with(str, operators[i]))
            return 1;

    return 0;
}

// Is this string bracketed?
int is_bracketed(const char * str) {
    size_t length = strlen(str);
    if(length < 2)
        return 0;

    return (str[0] == '(' && str[length - 1] == ')');
}

// Is this string simple quoted?
int is_simple_quoted(const char * str) {
    size_t length = strlen(str);
    if(length < 2)
        return 0;

    return (str[0] == '\'' && str[length - 1] == '\'');
}

// Is this string double quoted?
int is_double_quoted(const char * str) {
    size_t length = strlen(str);
    if(length < 2)
        return 0;

    return (str[0] == '"' && str[length - 1] == '"');
}

void unquote(char * dest, const char * src) {
    assert(dest != NULL);

    if(!is_simple_quoted(src) && !is_double_quoted(src)) {
        strcpy(dest, src);
        return;
    }

    size_t length = strlen(src);
    memcpy(dest, src + 1, length - 2);
    dest[length - 2] = '\0';
}

int split_string(char *** dest, size_t * dest_count, char * input, const size_t len) {
    size_t current_count = 0;
    char ** current_substrings = NULL;

    size_t current_pos = 0;

    while(current_pos < len) {
        // Skip any whitespace characters
        while(current_pos < len && is_whitespace(input[current_pos]))
            current_pos++;

        char * this_substring_start = input + current_pos;
        char this_char = input[current_pos];

        size_t substring_size = 0;

        // Deal with brackets
        if(this_char == '(') {
            int unclosed_brackets = 1;
            size_t end_pos = current_pos + 1;
            while(end_pos < len && unclosed_brackets > 0) {
                if(input[end_pos] == '(')
                    unclosed_brackets++;

                else if(input[end_pos] == ')')
                    unclosed_brackets--;

                if(unclosed_brackets > 0)
                    end_pos++;
            }

            if(end_pos == len && unclosed_brackets > 0) {
                fprintf(stderr, "Syntax error: Bad parenthesization!\n");
                return -1;
            }

            substring_size = end_pos - current_pos + 1;
        }
        // Deal with double quotes
        else if(this_char == '"') {
            size_t end_pos = current_pos + 1;
            while(end_pos < len && input[end_pos] != '"')
                end_pos++;

            if(end_pos == len) {
                fprintf(stderr, "Syntax error: Bad quoting!\n");
                return -1;
            }

            substring_size = end_pos - current_pos + 1;
        }
        // Deal with single quotes
        else if(this_char == '\'') {
            size_t end_pos = current_pos + 1;
            while(end_pos < len && input[end_pos] != '\'')
                end_pos++;

            if(end_pos == len) {
                fprintf(stderr, "Syntax error: Bad quoting!\n");
                return -1;
            }

            substring_size = end_pos - current_pos + 1;
        }
        // Deal with operators
        else if(starts_with_operator(this_substring_start)) {
            for(int i = 0; i < operator_count; i++)
                if(starts_with(this_substring_start, operators[i])) {
                    substring_size = strlen(operators[i]);
                    break;
                }
        }
        // Deal with actual words and operators
        else {
            size_t end_pos = current_pos;
            while(end_pos < len && !is_whitespace(input[end_pos]) && !is_skip_seq(input[end_pos]) && !starts_with_operator(input + end_pos))
                end_pos++;
            end_pos--;

            substring_size = end_pos - current_pos + 1;
        }

        if (substring_size > 0) {
            // Extract the substring from the input string
            char *new_substring = malloc(substring_size + 1);
            strncpy(new_substring, this_substring_start, substring_size);
            new_substring[substring_size] = '\0';

            // Save the substring into the destination array
            current_count++;
            current_substrings = realloc(current_substrings, current_count * sizeof(char *));
            current_substrings[current_count - 1] = new_substring;

            // Update the current position
            current_pos += substring_size;

            continue;
        }

        current_pos++;
    }

    *dest = current_substrings;
    *dest_count = current_count;

    return 0;
}

int build_action_graph(action_t * dest, char ** substrings, const size_t substring_count) {
    // Treat null input as a NOP
    if(substrings == NULL || substring_count == 0) {
        dest->action_type = ACTION_NOP;
        dest->action_count = 0;
        dest->actions = NULL;
        dest->argc = 0;
        dest->argv = NULL;
        dest->stdin_path = NULL;
        dest->stdin_mode = 0;
        dest->stdout_path = NULL;
        dest->stdout_mode = 0;

        return 0;
    }

    // Eliminate brackets
    if(substring_count == 1 && is_bracketed(substrings[0])) {
        size_t substring_length = strlen(substrings[0]);
        int parse_result = parse_input(dest, substrings[0] + 1, substring_length - 2);
        if(parse_result < 0)
            return parse_result;

        return 0;
    }

    // Look for AND and OR operators
    for(int i = (int)substring_count - 1; i >= 0; i--) {
        int found_and = (strcmp(substrings[i], "&&") == 0);
        int found_or = (strcmp(substrings[i], "||") == 0);

        if(!found_and && !found_or)
            continue;

        size_t left_substring_count = i;
        size_t right_substring_count = substring_count - (i + 1);

        // Ensure the correctness of the AND / OR statement
        if(left_substring_count < 1)
            return -1;

        if(right_substring_count < 1)
            return -1;

        // Populate the left/right substring arrays
        char ** left_substrings = malloc(left_substring_count * sizeof(char *));
        char ** right_substrings = malloc(right_substring_count * sizeof(char *));

        for(int j = 0; j < left_substring_count; j++)
            left_substrings[j] = substrings[j];

        for(int j = 0; j < right_substring_count; j++)
            right_substrings[j] = substrings[i + j + 1];

        // Parse the left side of the statement
        action_t * left_action_ptr = malloc(sizeof(action_t));
        int left_parse_result = build_action_graph(left_action_ptr, left_substrings, left_substring_count);
        if(left_parse_result < 0)
            return left_parse_result;

        // Parse the right side of the statement
        action_t * right_action_ptr = malloc(sizeof(action_t));
        int right_parse_result = build_action_graph(right_action_ptr, right_substrings, right_substring_count);
        if(right_parse_result < 0)
            return right_parse_result;

        // Save the result
        if(found_and)
            dest->action_type = ACTION_EXEC_AND;
        else if (found_or)
            dest->action_type = ACTION_EXEC_OR;
        else {
            fprintf(stderr, "Boolean algebra is a lie!\n");
            exit(-1);
        }

        dest->action_count = 2;
        dest->actions = malloc(dest->action_count * sizeof(action_t *));
        dest->actions[0] = left_action_ptr;
        dest->actions[1] = right_action_ptr;
        dest->argc = 0;
        dest->argv = NULL;
        dest->stdin_path = NULL;
        dest->stdin_mode = 0;
        dest->stdout_path = NULL;
        dest->stdout_mode = 0;

        return 0;
    }

    // We shouldn't have any brackets by now
    for(int i = 0; i < substring_count; i++)
        if(is_bracketed(substrings[i]))
            return -1;

    // Look for one or more PIPE operators
    for(int i = 0; i < substring_count; i++) {
        int found_first_pipe = (strcmp(substrings[i], "|") == 0);

        if(!found_first_pipe)
            continue;


        int pipe_count = 1;
        int * pipe_positions = malloc(pipe_count * sizeof(int *));

        // Save the first PIPE operator
        pipe_positions[0] = i;

        // Find all the PIPE operators
        for(int j = i + 1; j < substring_count; j++) {
            int found_next_pipe = (strcmp(substrings[j], "|") == 0);

            if(!found_next_pipe)
                continue;

            pipe_count++;
            pipe_positions = realloc(pipe_positions, pipe_count * sizeof(int *));
            pipe_positions[pipe_count - 1] = j;
        }

        // Ensure the correctness of the PIPE statement
        if(pipe_positions[0] == 0)
            return -1;

        if(pipe_positions[pipe_count - 1] == substring_count)
            return -1;

        // We can't have two pipes next to each other
        for(int j = 0; j < pipe_count - 1; j++)
            if(pipe_positions[j + 1] - pipe_positions[j] <= 1)
                return -1;

        // Start creating the action
        dest->action_type = ACTION_EXEC_PIPE;
        dest->argc = 0;
        dest->argv = NULL;
        dest->stdin_path = NULL;
        dest->stdin_mode = 0;
        dest->stdout_path = NULL;
        dest->stdout_mode = 0;

        dest->action_count = pipe_count + 1;
        dest->actions = malloc(dest->action_count * sizeof(action_t *));

        // Save the first argument of the PIPE
        dest->actions[0] = malloc(sizeof(action_t));
        size_t first_substring_count = pipe_positions[0];
        char ** first_substrings = malloc(first_substring_count * sizeof(char *));
        for(int j = 0; j < first_substring_count; j++)
            first_substrings[j] = substrings[j];

        int first_parse_result = build_action_graph(dest->actions[0], first_substrings, first_substring_count);
        if(first_parse_result < 0)
            return first_parse_result;

        // Save the next argument(s) of the PIPE
        for(int j = 0; j < pipe_count; j++) {
            size_t next_substring_count;

            // Is this the last argument chunk?
            if(j == pipe_count - 1)
                next_substring_count = substring_count - pipe_positions[j] - 1;
            else
                next_substring_count = pipe_positions[j + 1] - pipe_positions[j] - 1;

            // Copy the next substrings into a separate array
            char ** next_substrings = malloc(next_substring_count * sizeof(char *));
            for(int k = 0; k < next_substring_count; k++)
                next_substrings[k] = substrings[pipe_positions[j] + k + 1];

            // Build the graph
            dest->actions[j + 1] = malloc(sizeof(action_t));
            int next_parse_result = build_action_graph(dest->actions[j + 1], next_substrings, next_substring_count);
            if(next_parse_result < 0)
                return next_parse_result;
        }

        return 0;
    }

    // Do we have a TRUE operator?
    if(substring_count == 1 && strcmp(substrings[0], "true") == 0) {
        // Save the result
        dest->action_type = ACTION_TRUE;
        dest->action_count = 0;
        dest->actions = NULL;
        dest->argc = 0;
        dest->argv = NULL;
        dest->stdin_path = NULL;
        dest->stdin_mode = 0;
        dest->stdout_path = NULL;
        dest->stdout_mode = 0;

        return 0;
    }

    // Do we have a FALSE operator?
    if(substring_count == 1 && strcmp(substrings[0], "false") == 0) {
        // Save the result
        dest->action_type = ACTION_FALSE;
        dest->action_count = 0;
        dest->actions = NULL;
        dest->argc = 0;
        dest->argv = NULL;
        dest->stdin_path = NULL;
        dest->stdin_mode = 0;
        dest->stdout_path = NULL;
        dest->stdout_mode = 0;

        return 0;
    }


    // Do we have any redirect operators? (redirected execution statement)
    {
        int output_append_pos = -1;
        int output_pos = -1;
        int input_pos = -1;

        for (int i = 0; i < substring_count; i++) {
            int found_output_append = (strcmp(substrings[i], ">>") == 0);
            int found_output = (strcmp(substrings[i], ">") == 0);
            int found_input = (strcmp(substrings[i], "<") == 0);

            if(found_output_append) {
                if(output_append_pos != -1) {
                    fprintf(stderr, "Syntax error: only one '>>' allowed per process!\n");
                    return -1;
                }
                output_append_pos = i;
                continue;
            }

            if(found_output) {
                if(output_pos != -1) {
                    fprintf(stderr, "Syntax error: only one '>' allowed per process!\n");
                    return -1;
                }
                output_pos = i;
                continue;
            }

            if(found_input) {
                if(input_pos != -1) {
                    fprintf(stderr, "Syntax error: only one '<' allowed per process!\n");
                    return -1;
                }
                input_pos = i;
                continue;
            }
        }

        if(output_append_pos != -1 || output_pos != -1 || input_pos != -1) {
            if (output_append_pos != -1 && output_pos != -1) {
                fprintf(stderr, "Syntax error: '>' incompatible with '>>'!\n");
                return -1;
            }

            int last_operator = input_pos;
            if(output_pos > last_operator)
                last_operator = output_pos;
            if(output_append_pos > last_operator)
                last_operator = output_append_pos;

            if(last_operator < substring_count - 2) {
                fprintf(stderr, "Syntax error: only one parameter allowed after redirect operators!\n");
                return -1;
            }

            action_t left_action;
            int left_parse_result = build_action_graph(&left_action, substrings, substring_count - 2);
            if(left_parse_result < 0)
                return left_parse_result;

            if(left_action.action_type != ACTION_EXEC && left_action.action_type != ACTION_EXEC_FILE_REDIRECT) {
                fprintf(stderr, "Syntax error: left side of redirect operator is not an execution instruction!\n");
                return -1;
            }

            memcpy(dest, &left_action, sizeof(action_t));
            dest->action_type = ACTION_EXEC_FILE_REDIRECT;

            if(last_operator == input_pos) {
                dest->stdin_path = malloc(strlen(substrings[last_operator + 1]) * sizeof(char));
                unquote(dest->stdin_path, substrings[last_operator + 1]);
                dest->stdin_mode = O_RDONLY;
            } else if (last_operator == output_pos) {
                dest->stdout_path = malloc(strlen(substrings[last_operator + 1]) * sizeof(char));
                unquote(dest->stdout_path, substrings[last_operator + 1]);
                dest->stdout_mode = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (last_operator == output_append_pos) {
                dest->stdout_path = malloc(strlen(substrings[last_operator + 1]) * sizeof(char));
                unquote(dest->stdout_path, substrings[last_operator + 1]);
                dest->stdout_mode = O_WRONLY | O_CREAT | O_APPEND;
            } else {
                fprintf(stderr, "Unknown living being error: how did you get here?\n");
                return -1;
            }

            return 0;
        }
    }

    // We're left with a simple execution statement
    dest->action_type = ACTION_EXEC;
    dest->action_count = 0;
    dest->actions = NULL;
    dest->argc = substring_count;
    dest->argv = malloc(dest->argc * sizeof(char *));
    dest->stdin_path = NULL;
    dest->stdin_mode = 0;
    dest->stdout_path = NULL;
    dest->stdout_mode = 0;

    for(int i = 0; i < dest->argc; i++) {
        char * arg = malloc(strlen(substrings[i]) * sizeof(char));
        unquote(arg, substrings[i]);
        (dest->argv)[i] = arg;
    }

    return 0;
}

int parse_input(action_t * dest, char * input, const size_t len) {
    // Treat null input as a NOP
    if(input == NULL || len == 0) {
        dest->action_type = ACTION_NOP;
        dest->action_count = 0;
        dest->actions = NULL;
        dest->argc = 0;
        dest->argv = NULL;
        dest->stdin_path = NULL;
        dest->stdin_mode = 0;
        dest->stdout_path = NULL;
        dest->stdout_mode = 0;

        return 0;
    }

    // Split the string into substrings
    char ** substrings = NULL;
    size_t substring_count = 0;

    int split_status = split_string(&substrings, &substring_count, input, len);
    if(split_status < 0)
        return split_status;

    // Pass the substrings to the action graph generator
    int build_status = build_action_graph(dest, substrings, substring_count);
    if(build_status < 0)
        return build_status;

    return 0;
}