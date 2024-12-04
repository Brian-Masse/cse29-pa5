#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "pish.h"

static char pish_history_path[1024] = {'\0'};
FILE *history_file;

/*
 * Set history file path to ~/.pish_history.
 */
// MARK: set_history_path
static void set_history_path()
{
    const char *home = getpwuid(getuid())->pw_dir;
    strncpy(pish_history_path, home, 1024);
    strcat(pish_history_path, "/.pish_history");
}

// MARK: set_history_file
void set_history_file() {
    // set history path if needed
    if (!(*pish_history_path)) {
        set_history_path();
    }

    // open the history file if needed
    // if the file does not exist, create it
    if (history_file == NULL) {
        history_file = fopen( pish_history_path, "a" );

        if (history_file == NULL) {
            perror( "open" );
            return;
        }
    }
}

// MARK: add_history
void add_history(const struct pish_arg *arg) {
    
    set_history_file();

    char buf[1024];
    char index = 0;

    for (int i = 0; i < arg->argc; i += 1) {
        sprintf( buf + index, "%s ", arg->argv[i] );
        index += strlen(arg->argv[i]) + 1;
    }

    fprintf(history_file, "%s\n", buf);
    fflush(history_file);
}

// MARK: print_history
void print_history() {
    // set history path if needed
    if (!(*pish_history_path)) {
        set_history_path();
    }

    //open the file in read-only mode
    FILE* read_history_file = fopen( pish_history_path, "r" );
    if (read_history_file == NULL) {
        perror( "open" );
        return;
    }

    char buf[1024];
    size_t count = 1;

    while(fgets( buf, sizeof(buf), read_history_file ) ) {
        printf( "%lu %s", count, buf );
        count += 1;
    }

    fclose( read_history_file );
}
