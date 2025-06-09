#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

char history[MAX_LINE];

void parse_input(char *input, char **args, int *is_background) {
    int i = 0;
    *is_background = 0;

    args[i] = strtok(input, " \n");
    while (args[i] != NULL) {
        if (strcmp(args[i], "&") == 0) {
            *is_background = 1;
            args[i] = NULL;
            break;
        }
        i++;
        args[i] = strtok(NULL, " \n");
    }
    args[i] = NULL;
}

void execute_command(char **args, int is_background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // Handle redirection
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], ">") == 0) {
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = NULL;
                break;
            } else if (strcmp(args[i], "<") == 0) {
                int fd = open(args[i + 1], O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
                args[i] = NULL;
                break;
            } else if (strcmp(args[i], "|") == 0) {
                args[i] = NULL;
                int pipefd[2];
                pipe(pipefd);
                pid_t pid2 = fork();
                if (pid2 == 0) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]);
                    close(pipefd[1]);
                    execvp(args[0], args);
                    perror("Exec failed");
                    exit(1);
                } else {
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[1]);
                    close(pipefd[0]);
                    execvp(args[i + 1], &args[i + 1]);
                    perror("Exec failed");
                    exit(1);
                }
            }
        }

        execvp(args[0], args);
        perror("Exec failed");
        exit(1);
    } else {
        if (!is_background) {
            wait(NULL);
        }
    }
}

int main(void) {
    char *args[MAX_LINE / 2 + 1];
    int should_run = 1;

    while (should_run) {
        printf("osh>&#x003E;");
        fflush(stdout);

        char input[MAX_LINE];
        if (!fgets(input, MAX_LINE, stdin)) {
            continue;
        }

        if (strcmp(input, "exit\n") == 0) {
            should_run = 0;
            continue;
        }

        if (strcmp(input, "!!\n") == 0) {
            if (strlen(history) == 0) {
                printf("No commands in history.\n");
                continue;
            } else {
                printf("%s\n", history);
                strcpy(input, history);
            }
        } else {
            strcpy(history, input);
        }

        int is_background = 0;
        parse_input(input, args, &is_background);
        if (args[0] != NULL) {
            execute_command(args, is_background);
        }
    }
    return 0;
}
