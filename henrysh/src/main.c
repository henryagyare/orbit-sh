#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h> // for waitpid()
#include <unistd.h>   // for fork(), execvp()

#define MAX_CMD 1024
#define MAX_ARGS 64 // Maximum number of arguments

int main()
{
    char input[MAX_CMD];
    char *args[MAX_ARGS]; // Array of pointers to strings
    int i;

    while (1)
    {
        printf("henrysh> ");
        fflush(stdout);

        if (fgets(input, MAX_CMD, stdin) == NULL)
        {
            printf("\nGoodbye! 🚀\n");
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Skip empty input
        if (input[0] == '\0')
            continue;

        // Tokenization: split input into tokens (words)
        i = 0;
        args[i] = strtok(input, " \t\r\n"); // Split by space, tab, etc.
        while (args[i] != NULL && i < MAX_ARGS - 1)
        {
            args[++i] = strtok(NULL, " \t\r\n");
        }
        args[i] = NULL; // execvp() requires NULL at end


        // Check for built-in commands
        if (strcmp(args[0], "cd") == 0)
        {
            // Handle cd
            if (args[1] == NULL)
            {
                fprintf(stderr, "henrysh: cd: expected argument\n");
            }
            else
            {
                if (chdir(args[1]) != 0)
                {
                    perror("henrysh");
                }
            }
            continue; // Skip fork/exec — we're done
        }

        if (strcmp(args[0], "exit") == 0)
        {
            printf("Leaving... 🚀\n");
            exit(0);
        }

        // Execute external command
        pid_t pid = fork();

        if (pid == -1)
        {
            // Fork failed
            perror("henrysh: fork failed");
            continue;
        }
        else if (pid == 0)
        {
            // Child process: run the command
            execvp(args[0], args);

            // If execvp returns, it failed
            perror("henrysh");
            exit(1);
        }
        else
        {
            // Parent process: wait for child to finish
            int status;
            do
            {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }

    return 0;
}