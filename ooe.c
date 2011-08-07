#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int outpipe[2];
	pid_t fork_result, wait_result;
	int exit_status;

	if (argc < 2) {
		fprintf(stderr, "usage: %s program_to_execute [arguments ...]\n", argv[0]);
		exit(1);
	}

	if (pipe(outpipe) == -1) {
		perror("pipe creation");
		exit(254);
	}

	fork_result = fork();
	if (fork_result == -1) {
		perror("fork()");
		exit(254);
	}
	if (fork_result == 0) {
		close(outpipe[0]);
		if (dup2(outpipe[1], 1) == -1) {
			perror("dup2 for stdout");
			exit(254);
		}
		if (dup2(outpipe[1], 2) == -1) {
			perror("dup2 for stderr");
			exit(254);
		}

		execvp(argv[1], &argv[1]);
		fprintf(stderr, "%s: %s: ", argv[0], argv[1]);
		perror("");
		exit(253);
	}

	close(0);
	close(outpipe[1]);

	const int chunk_size = 65536;
	int buffer_size = chunk_size * 2;
	int buffer_length = 0;
	char *buffer = (char *)malloc(buffer_size);

	while (1) {
		if (buffer == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(254);
		}
		ssize_t n = read(outpipe[0], buffer + buffer_length, chunk_size);
		if (n == 0) break;
		if (n == -1) {
			perror("read on pipe");
			exit(254);
		}
		buffer_length += n;
		if (buffer_size <= buffer_length + chunk_size) {
			buffer_size *= 2;
			if (buffer_size <= 0) {
				fprintf(stderr, "too much data\n");
				exit(254);
			}
			buffer = (char *)realloc(buffer, buffer_size);
		}
	}

	if (wait(&wait_result) == -1) {
		perror("wait() returned error");
		exit(254);
	}

	exit_status = WEXITSTATUS(wait_result);
	if (exit_status == 0) {
		free(buffer);
		exit(0);
	}

	write(1, buffer, buffer_length);
	free(buffer);
	exit(exit_status);
}
