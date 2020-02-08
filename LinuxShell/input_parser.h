#ifndef SHELLY_INPUT_PARSER_H
#define SHELLY_INPUT_PARSER_H

#include <stddef.h>
#include "action.h"

int parse_input(action_t * dest, char * input, size_t len);
int split_string(char *** dest, size_t * dest_count, char * input, size_t len);
#endif //SHELLY_INPUT_PARSER_H
