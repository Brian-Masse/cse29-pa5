#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "pish.h"

// MARK: Global Variables
const char* ERROR_FOPEN_FAILURE = "open";
const char* ERROR_CD_FAILURE = "cd";
const char* ERROR_EXECVP_FAILURE = "pish";
const char* ERROR_FORK_FAILURE = "fork";

FILE *fp = NULL;

/*
 * Batch mode flag. If set to 0, the shell reads from stdin. If set to 1,
 * the shell reads from a file from argv[1].
 */
// MARK: script_mode
static int script_mode = 0;

/*
 * Prints a prompt IF NOT in batch mode (see script_mode global flag),
 */ 
// MARK: Prompt
int prompt(void) {
    static const char prompt[] = {0xe2, 0x96, 0xb6, ' ', ' ', '\0'};
    if (!script_mode) {
        printf("%s", prompt);
        fflush(stdout);
    }
    return 1;
}

/*
 * Print usage error for built-in commands.
 */
// MARK: Usage Error
void usage_error(void) {
    fprintf(stderr, "pish: Usage error\n");
    fflush(stdout);
}

// MARK: Exit
void exit_successfully(int should_exit) {
	fclose(history_file);
	fclose(fp);

	if (should_exit) { exit(EXIT_SUCCESS); }
}

void exit_failure() {
	fclose(history_file);

	exit(EXIT_FAILURE);
}

/*
 * Break down a line of input by whitespace, and put the results into
 * a struct pish_arg to be used by other functions.
 *
 * @param command   A char buffer containing the input command
 * @param arg       Broken down args will be stored here.
 */
// MARK: parse_command
void parse_command(char *command, struct pish_arg *arg)
{
	char* delim = " \t\n|";
	char* command_token = strtok( command, delim );

	if ( command_token == NULL ) { return; }

	int argc = 0;

	while( command_token ) { 
		arg->argv[argc] = command_token;	
		argc += 1;
		command_token = strtok( NULL, delim );
	 }
	
	arg->argv[argc] = NULL;
	arg->argc = argc;
}

// MARK: run_cd
// This is gaurenteed to have at least one argument
void run_cd(struct pish_arg* arg) {
	if ( arg->argc != 2 ) { 
		usage_error();
		return;
	}

	int result = chdir(arg->argv[1]);
	if ( result != 0) {
		perror( ERROR_CD_FAILURE );
		return;
	}
}

// MARK: run_history
void run_history( struct pish_arg* arg ) {
	if (arg->argc != 1) {
		usage_error();
		return;
	}

	print_history();
}

// MARK: run_general_command
// This is gaurenteed to have at least one argument
// This is for any non-built in shell function
void run_general_command( struct pish_arg* arg ) {
	int child_status;

	pid_t pid = fork();
	if (pid == -1) {
		perror( ERROR_FORK_FAILURE );
		return;
	}

	if ( pid == 0 ) {		
		// as the child, execute the command as a new program
		execvp( arg->argv[0], arg->argv );
		perror( ERROR_EXECVP_FAILURE );
		return;

	} else {
		// as the parent, wait for the child to finish executing
		waitpid( pid, &child_status, 0 );
		return;
	}
}

/*
 * Run a command.
 *
 * Built-in commands are handled internally by the pish program.
 * Otherwise, use fork/exec to create child process to run the program.
 *
 * If the command is empty, do nothing.
 * If NOT in batch mode, add the command to history file.
 */
// MARK: run_command
void run(struct pish_arg *arg)
{
	char* command = arg->argv[0];

	if ( strcmp(command, "exit") == 0 ) {
		exit_successfully(1);
	} else if ( strcmp(command, "cd") == 0 ) {
		run_cd(arg);
	} else if ( strcmp(command, "history") == 0 ) {
		run_history(arg);
	} else {
		run_general_command(arg);
	}
}


// MARK: PISH
int pish(FILE *fp)
{
    // assume input does not exceed buffer size
    char buf[1024];
    struct pish_arg arg;

	prompt();

	while(fgets( buf, sizeof(buf), script_mode ? fp : stdin ) ) {
		parse_command( buf, &arg );

		if ( arg.argc != 0 ) { 
			add_history( &arg );
			run( &arg );
		}

		prompt();

		//reset the arg struct
		arg.argc = 0;
		arg.argv[0] = NULL;
	}

    return 0;
}

// MARK: Open file
void open_file( char* path ) {
	fp = fopen( path, "r" );

	if (fp == NULL) {
        perror( ERROR_FOPEN_FAILURE );
		exit_failure();
    }
}

// MARK: main
int main(int argc, char *argv[]) {

	if (argc == 2) {
		open_file( argv[1] );
		script_mode = 1;

	} else if (argc == 1) {
		script_mode = 0;

	} else {
		usage_error();
		exit_failure();
	}

    pish(fp);
	exit_successfully(0);
	return EXIT_SUCCESS;
}
