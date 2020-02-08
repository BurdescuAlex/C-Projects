#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void get_current_directory(char* str) {
    char * user_home = getenv("HOME");
    char * cwd = malloc(1024);
    getcwd(cwd, 1024);

    // Show ~ instead of the home directory if possible
    char * search_result = strstr(cwd, user_home);

    if(search_result == cwd) {
        strcpy(str, "~");
        strcat(str, cwd + strlen(user_home));
    } else {
        strcpy(str, cwd);
    }

    free(cwd);
}

int read_console_input(char* str)
{
    char * username;
    char cwd[1024];

    username = getenv("USER");
    get_current_directory(cwd);

    printf("\033[31;1m%s\033[92m:\033[32m%s\033[0m", username, cwd);

    char* buf;

    buf = readline("\033[93;1m€\033[0m ");
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}