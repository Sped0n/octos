/**
 * @file shell.h
 * @brief Shell interface API definitions
 */

#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Maximum length of command line input
 */
#ifndef SHELL_MAX_INPUT_LEN
#define SHELL_MAX_INPUT_LEN 64
#endif

/**
 * @brief Maximum number of command arguments
 */
#ifndef SHELL_MAX_ARGS
#define SHELL_MAX_ARGS 8
#endif

/**
 * @brief Shell command handler function type
 */
typedef int (*ShellCmdHandler_t)(int argc, char *argv[]);

/**
 * @brief Shell command structure
 */
typedef struct {
    const char *name;          /*!< Command name string */
    ShellCmdHandler_t handler; /*!< Command handler function */
} ShellCommand_t;

/**
 * @brief Shell structure definition
 */
typedef struct {
    const ShellCommand_t *commands;         /*!< Pointer to command table */
    size_t num_commands;                    /*!< Number of commands in table */
    char input_buffer[SHELL_MAX_INPUT_LEN]; /*!< Input buffer */
    size_t input_pos;               /*!< Current position in input buffer */
    void (*print)(const char *str); /*!< Print function pointer */
} Shell_t;

/**
 * @brief Initialize shell interface
 * @param shell: Pointer to shell structure
 * @param commands: Array of command definitions
 * @param num_commands: Number of commands in array
 * @param print_func: Function pointer for output
 * @return None
 */
void shell_init(Shell_t *shell, const ShellCommand_t *commands,
                size_t num_commands, void (*print_func)(const char *str));

/**
 * @brief Process received character
 * @param shell: Pointer to shell structure
 * @param c: Received character
 * @return None
 */
void shell_process_char(Shell_t *shell, char c);

/**
 * @brief Execute command line
 * @param shell: Pointer to shell structure
 * @param cmdline: Command line string to execute
 * @return Command execution result
 */
int shell_execute(Shell_t *shell, char *cmdline);

/**
 * @brief Print shell prompt
 * @param shell: Pointer to shell structure
 * @return None
 */
void shell_prompt(Shell_t *shell);

#endif /* __SHELL_H__ */
