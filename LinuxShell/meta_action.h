#ifndef SHELLY_META_ACTION_H
#define SHELLY_META_ACTION_H

#include "action.h"

typedef int (*meta_action_fn_t)(int, char **);
typedef struct {
    char * command;
    meta_action_fn_t function;
} meta_action_t;

int is_meta_action(action_t * action);
int execute_meta_action(action_t * action);

#endif //SHELLY_META_ACTION_H
