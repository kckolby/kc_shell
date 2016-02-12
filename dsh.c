#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>

#define BUFSZ 1024

struct cmd
  {
    int redirect_in;     /* Any stdin redirection?         */
    int redirect_out;    /* Any stdout redirection?        */
    int redirect_append; /* Append stdout redirection?     */
    int background;      /* Put process in background?     */
    int piping;          /* Pipe prog1 into prog2?         */
    char *infile;        /* Name of stdin redirect file    */
    char *outfile;       /* Name of stdout redirect file   */
    char *argv1[10];     /* First program to execute       */
    char *argv2[10];     /* Second program in pipe         */
  };

int cmdscan(char *cmdbuf, struct cmd *com);
void dupSTDOUT(int fd[]);
void dupSTDIN(int fd[]);
void dup_fileout(int fd[], char *fileout);
void dup_filein(int fd[], char *filein);
void dup_append(int fd[], char *fileout);

int main(int argc, char *argv[]) {
	char buf[BUFSZ];
	//char *sargv[16];
	struct cmd command;
	int fd[2], fdio[2];
	pid_t pid;
	
	
	printf("\n>> ");

	while( strcmp(gets(buf), "exit") != 0)  {
		if (cmdscan(buf, &command)) {
			//printf("Illegal Format: \n");
			printf(">> ");
			continue;
		}
		
	
		switch(pid = fork()) {
			case -1:
				fprintf(stderr, "fork error\n");
				exit(-1);

			case 0:
				/*If no piping execute in child*/
				if (command.piping == 0) {
					if (command.background) {
						switch(fork()) {
							case -1:
								perror("fork error");
								exit(-1);
							case 0:
								break;
							default:
								exit(0);
						}
					}
					/*If there is redirection of any kind open a pipe*/
					if (command.redirect_out | command.redirect_in | command.redirect_append) {
						if (pipe(fd)) {
							perror("pipe error");
							exit(-1);
						}
						
			
					/*Directing the plumbing to the appropriate file descriptors*/	
					if (command.redirect_out) {
						if (command.redirect_append) {
							dup_append(fd, command.outfile);
						}
						else {	
							dup_fileout(fd, command.outfile);
						}
					}
					
					else {
						dupSTDOUT(fd);
					}

					if (command.redirect_in) {
						dup_filein(fd, command.infile);
					}
					else {
						dupSTDIN(fd);
					}
					}
					
					/*Executing the program*/
					execvp(command.argv1[0], command.argv1);
					fprintf(stderr, "%s:command not found\n", command.argv1[0]);
					exit(-1);
					
				}
			
				else if (command.piping) {
					if (command.background) {
						switch(fork()) {
							case -1:
								perror("fork error");
								exit(-1);
							case 0:
								break;
							default:
								exit(0);
						}
					}
					if (pipe(fd)) {
						perror("pipe error");
						exit(-1);
					}
					if (command.redirect_out | command.redirect_in | command.redirect_append) {
						if (pipe(fdio)) {
							perror("pipe error");
							exit(-1);
						}
					}	
					switch(fork()) {
						case -1:
							fprintf(stderr, "fork error\n");
							exit(-1);
						case 0:
							if (command.redirect_in) {
								dup_filein(fdio, command.infile);
							}
							else {
								dupSTDIN(fdio);
							}	
							dupSTDOUT(fd);	
							
							execvp(command.argv1[0], command.argv1);
							fprintf(stderr, "%s:command not found\n", command.argv1[0]);
							exit(-1);
							

						default:
							if (command.redirect_out) {
								if (command.redirect_append) {
									dup_append(fdio, command.outfile);
								}
								else {	
									dup_fileout(fdio, command.outfile);
								}
							}
							
							else {
								dupSTDOUT(fdio);
							}
							dupSTDIN(fd);
						   	
							execvp(command.argv2[0], command.argv2);
							fprintf(stderr, "%s:command not found\n", command.argv2[0]);
							exit(-1);
					}
				}
			default:
				wait(NULL);	
		}

		printf("\n>> ");
	}		
	exit(0);
}

void dupSTDOUT(int fd[]) {
	close(fd[0]);
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);
}

void dupSTDIN(int fd[]) {
	close(fd[1]);
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);
}

void dup_fileout(int fd[], char *fileout) {
	close(fd[0]);
	fd[1] = open(fileout, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0600);
	if (fd[1] < 0) {
		perror(fileout);
		exit(-1);
	}
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);
}

void dup_append(int fd[], char *fileout) {
	close(fd[0]);
	fd[1] = open(fileout, O_WRONLY | O_CREAT | O_APPEND, 0600);
	if (fd[1] < 0) {
		perror(fileout);
		exit(-1);
	}
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);
}

void dup_filein(int fd[], char *filein) {
	close(fd[1]);
	fd[0] = open(filein, O_RDONLY);
	if (fd[0] < 0) {
		perror(filein);
		exit(-1);
	}
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);
}
