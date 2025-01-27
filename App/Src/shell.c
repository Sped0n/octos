/**
 * @file shell.c
 * @brief Implementation of shell interface
 */

#include "shell.h"
#include <stdbool.h>
#include <string.h>

/* Private function prototypes */
static void shell_parse_command(char *cmdline, int *argc, char *argv[]);
static void shell_backspace(Shell_t *shell);
static void shell_process_command(Shell_t *shell);

/**
 * @brief Initialize shell interface
 * @param shell: Pointer to shell structure
 * @param commands: Array of command definitions
 * @param num_commands: Number of commands in array
 * @param print_func: Function pointer for output
 * @return None
 */
void shell_init(Shell_t *shell, const ShellCommand_t *commands,
                size_t num_commands, void (*print_func)(const char *str)) {
    shell->commands = commands;
    shell->num_commands = num_commands;
    shell->print = print_func;
    shell->input_pos = 0;
    memset(shell->input_buffer, 0, SHELL_MAX_INPUT_LEN);

    /* Print initial prompt */
    shell_prompt(shell);
}

/**
 * @brief Process received character
 * @param shell: Pointer to shell structure
 * @param c: Received character
 * @return None
 */
void shell_process_char(Shell_t *shell, char c) {
    char tmp[2];
    tmp[1] = '\0';
    switch (c) {
        case '\r': /* Enter key */
        case '\n':
            shell->print("\r\n");
            if (shell->input_pos > 0) { shell_process_command(shell); }
            shell_prompt(shell);
            break;

        case '\b': /* Backspace */
        case 0x7F: /* Delete */
            shell_backspace(shell);
            break;

        default:
            /* Only process printable characters */
            if (c >= 32 && c <= 126) {
                if (shell->input_pos < SHELL_MAX_INPUT_LEN - 1) {
                    shell->input_buffer[shell->input_pos++] = c;
                    tmp[0] = c;
                    shell->print(tmp);
                }
            }
            break;
    }
}

/**
 * @brief Execute command line
 * @param shell: Pointer to shell structure
 * @param cmdline: Command line string to execute
 * @return Command execution result
 */
int shell_execute(Shell_t *shell, char *cmdline) {
    int argc = 0;
    char *argv[SHELL_MAX_ARGS];

    /* Parse command line into arguments */
    shell_parse_command(cmdline, &argc, argv);

    if (argc > 0) {
        /* Search for command */
        for (size_t i = 0; i < shell->num_commands; i++) {
            if (strcmp(argv[0], shell->commands[i].name) == 0) {
                /* Execute command handler */
                return shell->commands[i].handler(argc, argv);
            }
        }
        /* Command not found */
        shell->print("Unknown command\r\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Print shell prompt
 * @param shell: Pointer to shell structure
 * @return None
 */
void shell_prompt(Shell_t *shell) {
    shell->print("> ");
    shell->input_pos = 0;
    memset(shell->input_buffer, 0, SHELL_MAX_INPUT_LEN);
}

/**
 * @brief Parse command line into arguments
 * @param cmdline: Command line string to parse
 * @param argc: Pointer to store argument count
 * @param argv: Array to store argument strings
 * @return None
 */
static void shell_parse_command(char *cmdline, int *argc, char *argv[]) {
    char *token;
    *argc = 0;

    /* Get first token */
    token = strtok(cmdline, " ");

    /* Walk through other tokens */
    while (token != NULL && *argc < SHELL_MAX_ARGS) {
        argv[(*argc)++] = token;
        token = strtok(NULL, " ");
    }
}

/**
 * @brief Handle backspace character
 * @param shell: Pointer to shell structure
 * @return None
 */
static void shell_backspace(Shell_t *shell) {
    if (shell->input_pos > 0) {
        shell->print("\b \b"); /* Erase character on terminal */
        shell->input_pos--;
        shell->input_buffer[shell->input_pos] = '\0';
    }
}

/**
 * @brief Process command in input buffer
 * @param shell: Pointer to shell structure
 * @return None
 */
static void shell_process_command(Shell_t *shell) {
    shell->input_buffer[shell->input_pos] = '\0';
    shell_execute(shell, shell->input_buffer);
}
