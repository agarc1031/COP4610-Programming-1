#include <stdio.h>     
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>     
#include <sys/wait.h>   
#include <fcntl.h>      
#include <signal.h>    

#define MAX_LINE 80     
#define MAX_ALIASES 10

char history[MAX_LINE]; 

// Structure for alias mapping
struct alias {
    char name[50];
    char command[100];
} aliases[MAX_ALIASES];
int alias_count = 0;

void handle_sigint(int sig) {
    write(STDOUT_FILENO, "\nosh> ", 7);
    fflush(stdout);
}

// Check and replace aliases
void check_alias(char **args) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(args[0], aliases[i].name) == 0) {
            strcpy(args[0], aliases[i].command);
            break;
        }
    }
}

// Handle alias creation
int handle_alias(char *input) {
    if (strncmp(input, "alias ", 6) == 0) {
        char *name = strtok(input + 6, "=");
        char *command = strtok(NULL, "\n");
        if (name && command && alias_count < MAX_ALIASES) {
            name = strtok(name, " \t\n");
            command = strtok(command, "\"\n");
            strcpy(aliases[alias_count].name, name);
            strcpy(aliases[alias_count].command, command);
            alias_count++;
            return 1;
        }
    }
    return 0;
}

void parse_input(char *input, char **args, int *is_background) {
    int i = 0;
    *is_background = 0;
    char *token = malloc(strlen(input) + 1);
    int in_token = 0, j = 0;
    int in_quotes = 0;

    for (int k = 0; input[k] != '\0'; ++k) {
        if (input[k] == '"') {
            in_quotes = !in_quotes;  // Toggle quote mode
        } else if (!in_quotes && input[k] == '\\' && input[k + 1] == ' ') {
            token[j++] = ' ';
            k++;
            in_token = 1;
        } else if (!in_quotes && (input[k] == ' ' || input[k] == '\n')) {
            if (in_token) {
                token[j] = '\0';
                args[i++] = strdup(token);
                j = 0;
                in_token = 0;
            }
        } else {
            token[j++] = input[k];
            in_token = 1;
        }
    }

    if (in_token) {
        token[j] = '\0';
        args[i++] = strdup(token);
    }

    args[i] = NULL;

    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        *is_background = 1;
        args[i - 1] = NULL;
    }

    free(token);
}

// Execute piped commands
void execute_pipe(char **args, int pipe_pos) {
    args[pipe_pos] = NULL;
    int pipefd[2];
    pipe(pipefd);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(args[0], args);
        perror("Exec failed");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        execvp(args[pipe_pos + 1], &args[pipe_pos + 1]);
        perror("Exec failed");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Handle input/output redirection
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            printf("[Input redirected to %s. Press Ctrl+D to finish]\n", args[i + 1]);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

// Execute command with support for background, redirection, and alias
void execute_command(char **args, int is_background) {
    check_alias(args);

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            execute_pipe(args, i);
            return;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        handle_redirection(args);
        execvp(args[0], args);
        perror("Exec failed");
        exit(1);
    } else {
        if (is_background) {
            printf("[Background] PID: %d\n", pid);
        } else {
            waitpid(pid, NULL, 0);
        }
    }
}

int main(void) {
    signal(SIGINT, handle_sigint);
    char *args[MAX_LINE / 2 + 1];
    int should_run = 1;

    while (should_run) {
        printf("osh> ");
        fflush(stdout);

        char input[MAX_LINE];
        if (!fgets(input, MAX_LINE, stdin)) {
            continue;
        }

        if (strcmp(input, "exit\n") == 0) {
            should_run = 0;
            continue;
        }

        if (handle_alias(input)) {
            continue;
        }

        if (strcmp(input, "!!\n") == 0) {
            if (strlen(history) == 0) {
                printf("No commands in history.\n");
                continue;
            } else {
                printf("%s", history);
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
