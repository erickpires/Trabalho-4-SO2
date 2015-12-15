#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FALSE 0
#define TRUE 1
#define N_SIG 31

static int is_running = TRUE;
static int terminated = FALSE;

void stop_running(int signal) {
	is_running = FALSE;
	terminated = TRUE;
}

FILE* execute_command(char* command, int arg_count, ...) {
	// Varargs
	va_list args_list;
	char** args = (char**) alloca((arg_count + 2) * sizeof(char*));
	va_start(args_list, arg_count);

	args[0] = command;
	int i;
	for(i = 1; i < arg_count + 1; i++)
		args[i] = va_arg(args_list, char*);

	args[i] = NULL;

	va_end(args_list);

	// Pipe
	int file_descriptors[2];
	if(pipe(file_descriptors) == -1) {
		fprintf(stderr, "Could not pipe\n");
		exit(2);
	}

	int pid = fork();
	if(pid < 0) {
		fprintf(stderr, "Could not fork\n");
		exit(3);
	}

	if(pid == 0) { // Child process
		// Closes the stdout file descriptor. Will be replaced with the pipe fd.
		close(1);
		close(file_descriptors[0]);
		dup2(file_descriptors[1], 1);

		// TODO: This is hardcoded
		execvp(command, args);

		return NULL; // This line is never reached
	}
	else { // Parent process
		close(file_descriptors[1]);
		return fdopen(file_descriptors[0], "r");
	}
}

int main(int argc, char** argv){

	pid_t pid, sid;

   	/* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
            exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */
	const char * filename = "zombies.log";
	FILE * logfile = NULL;
	if (!(logfile = fopen(filename, "w"))) {
		fprintf (stderr, "Could not open log file\n");
		return -1;
	}

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
            /* Log any failures here */
            exit(EXIT_FAILURE);
    }

	// Let the child processes die and not become a zombie.
	//signal(SIGCHLD, SIG_IGN);


	// Ignoring all signals except SIG_TERM and SIG_KILL
	// SIGCHLD is also ignore
	for(int signal_num = 0; signal_num < N_SIG; signal_num++) {
		if(signal_num != SIGTERM && signal_num != SIGKILL)
			signal(signal_num, SIG_IGN);
	}

	signal(SIGTERM, stop_running);

	int time_to_sleep;
	if (argc != 2 ||  (time_to_sleep = atoi (argv[1])) <= 0) {
        fprintf (stderr, "Use: %s [<n>]\n", argv[0]);
        exit(1);
    }


	const char separator[2] = " ";
	char *token;
	char state[2] = ".";
	char buffer[100];

	// Time related stuff
	time_t raw_time;
	struct tm* time_info;



	fprintf(logfile, "Log:\n\n");
	fprintf(logfile, "\tPID\t\tPPID\t\tNome do Programa\n");
	fprintf(logfile, "===========================================================\n");

	//Main loop
	while(is_running) {
		FILE* process_input_stream = execute_command("ps", 1, "-el");

		// Getting time now
		time(&raw_time);
		time_info = localtime(&raw_time);
		fprintf(logfile, "Time: %s\n", asctime(time_info));

		int first = TRUE;
		while(!feof(process_input_stream)) {
			if (first == TRUE) {
				fgets(buffer, sizeof(buffer), process_input_stream);
				first = FALSE;
			}

			fgets(buffer, sizeof(buffer), process_input_stream);

			token = strtok(buffer, separator);
		    int i = 0;
		    while( token != NULL )
		    {
		        //i = 1 -> state
		        if (i == 1) {
	            	strcpy(state, token);
		        }
				// se estado for
				if (!strcmp(state, "Z")) {
					//i = 3 -> PID
			        if (i == 3) {
			            fprintf(logfile, "\t%s", token);
			        }
			        //i = 4 -> PPID
			        if (i == 4) {
			            fprintf(logfile, "\t\t%s", token);
			        }
			        //i = 13 -> CMD
			        if (i == 13) {
			            fprintf(logfile, "\t\t%s\n", token);
			        }
				}
		        ++i;
		        token = strtok(NULL, separator);
		    }
		}

		fprintf(logfile, "\n===========================================================\n");
		fclose(process_input_stream);

		sleep(time_to_sleep);
	}

	if(terminated) {
		fprintf(logfile, "The deamon has been closed by a SIG_TERM\n");
	}

	fclose(logfile);

    return 0;
}
