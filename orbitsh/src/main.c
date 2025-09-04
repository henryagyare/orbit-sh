#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CMD 1024
#define MAX_ARGS 64  // Maximum number of arguments

int main() {
    char input[MAX_CMD];
    char *args[MAX_ARGS];  // Array of pointers to strings
    int i;

    while (1) {
        printf("orbitsh> ");
        fflush(stdout);

        if (fgets(input, MAX_CMD, stdin) == NULL) {
            printf("\nGoodbye, astronaut! 🚀\n");
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Skip empty input
        if (input[0] == '\0') continue;

        // Tokenization: split input into tokens (words)
        i = 0;
        args[i] = strtok(input, " \t\r\n");  // Split by space, tab, etc.
        while (args[i] != NULL && i < MAX_ARGS - 1) {
            args[++i] = strtok(NULL, " \t\r\n");
        }
        args[i] = NULL;  // execvp() requires NULL at end

        // For now: print all arguments
        printf("Parsed args:\n");
        for (int j = 0; args[j] != NULL; j++) {
            printf("  args[%d] = '%s'\n", j, args[j]);
        }
    }

    return 0;
}