#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/signal.h>

#define FALSE 0
#define TRUE 1

FILE* execute_command(const char* command) {
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

		execlp("ps", "ps", "aux", NULL);

		return NULL; // This line is never reached
	}
	else { // Parent process
		close(file_descriptors[1]);
		return fdopen(file_descriptors[0], "r");
	}
}

int main(int argc, char** argv){

	// Let the child processes die and not become a zombie.
	signal(SIGCHLD, SIG_IGN);

	int time_to_sleep;
	if (argc != 2 ||  (time_to_sleep = atoi (argv[1])) <= 0) {
        fprintf (stderr, "Use: %s [<n>]\n", argv[0]);
        exit(1);
    }


	char buffer[100];
	//Main loop
	while(TRUE) {
		FILE* process_input_stream = execute_command("ps aux");

		while(!feof(process_input_stream)) {
			fgets(buffer, 100, process_input_stream);
			printf("%s", buffer);
		}

		sleep(time_to_sleep);
	}

    return 0;
}
