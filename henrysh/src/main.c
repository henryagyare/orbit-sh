#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD 1024
#define MAX_ARGS 64
#define MAX_BG 64

// Function declarations
void handle_sigchld(int sig);

pid_t bg_pids[MAX_BG];  // Track background job PIDs
int bg_count = 0;       // Number of active background jobs

int main() {
    char input[MAX_CMD];
    char *args[MAX_ARGS];

    // Set up SIGCHLD handler to reap zombies
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("henrysh: sigaction failed");
        exit(1);
    }

    while (1) {
        printf("henrysh> ");
        fflush(stdout);

        if (fgets(input, MAX_CMD, stdin) == NULL) {
            printf("\nGoodbye! 🚀\n");
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Skip empty lines
        if (input[0] == '\0') continue;

        // Tokenize: split by whitespace
        int i = 0;
        args[i] = strtok(input, " \t\r\n");
        while (args[i] != NULL && i < MAX_ARGS - 1) {
            args[++i] = strtok(NULL, " \t\r\n");
        }
        args[i] = NULL;

        // Check for pipe: cmd1 | cmd2
        int pipe_pos = -1;
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "|") == 0) {
                pipe_pos = j;
                break;
            }
        }

        if (pipe_pos != -1) {
            // Handle pipeline

            // Split into two commands
            args[pipe_pos] = NULL;
            char **cmd1 = args;
            char **cmd2 = &args[pipe_pos + 1];

            // Create pipe
            int fd[2];
            if (pipe(fd) == -1) {
                perror("henrysh: pipe");
                continue;
            }

            // Fork first child (left side of pipe)
            pid_t pid1 = fork();
            if (pid1 == -1) {
                perror("henrysh: fork");
                close(fd[0]); close(fd[1]);
                continue;
            }

            if (pid1 == 0) {
                // Child 1: cmd1 (e.g., ls)
                dup2(fd[1], STDOUT_FILENO);  // stdout → write end
                close(fd[0]); close(fd[1]);
                execvp(cmd1[0], cmd1);
                perror("henrysh: cmd1");
                exit(1);
            }

            // Fork second child (right side of pipe)
            pid_t pid2 = fork();
            if (pid2 == -1) {
                perror("henrysh: fork");
                close(fd[0]); close(fd[1]);
                waitpid(pid1, NULL, 0);  // Clean up first child
                continue;
            }

            if (pid2 == 0) {
                // Child 2: cmd2 (e.g., wc)
                dup2(fd[0], STDIN_FILENO);   // stdin ← read end
                close(fd[0]); close(fd[1]);
                execvp(cmd2[0], cmd2);
                perror("henrysh: cmd2");
                exit(1);
            }

            // Parent: close both ends
            close(fd[0]); close(fd[1]);

            // Wait for both children
            int status;
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);

            continue;  // Done with pipeline — skip rest
        }

        // Check for background job (&)
        int background = 0;
        if (i > 1 && strcmp(args[i-1], "&") == 0) {
            background = 1;
            args[i-1] = NULL;  // Remove & from args
        }

        // Built-in commands
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "henrysh: cd: expected argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("henrysh");
                }
            }
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            printf("Leaving... 🚀\n");
            exit(0);
        }

        // External command
        pid_t pid = fork();
        if (pid == -1) {
            perror("henrysh: fork failed");
            continue;
        } else if (pid == 0) {
            // Child: execute command
            execvp(args[0], args);
            perror("henrysh");
            exit(1);
        } else {
            // Parent
            if (!background) {
                // Foreground: wait
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            } else {
                // Background: track PID and don't wait
                if (bg_count < MAX_BG) {
                    bg_pids[bg_count++] = pid;
                }
                printf("Background job [%d] started\n", pid);
            }
        }
    }

    return 0;
}

// Signal handler: only report true background jobs
void handle_sigchld(int sig) {
    (void)sig;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Only report if this PID was a background job
        int is_bg = 0;
        for (int i = 0; i < bg_count; i++) {
            if (bg_pids[i] == pid) {
                is_bg = 1;
                // Remove from list
                for (int j = i; j < bg_count - 1; j++) {
                    bg_pids[j] = bg_pids[j + 1];
                }
                bg_count--;
                break;
            }
        }
        if (is_bg) {
            printf("Background job [%d] completed\n", pid);
        }
    }
}