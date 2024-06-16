#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>
#include <stdbool.h>
#include <dirent.h> 
#include <sys/stat.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10

#define COLOR_RESET "\033[0m"
#define COLOR_WHITE "\033[37m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"

void read_command(char* command, char* args[]);
void execute_command(char* command, char* args[]);
void print_prompt(bool valid_command);
void list_directory_files(const char* dirname);

int main() {
    char command[MAX_COMMAND_LENGTH];
    char* args[MAX_ARGS];
    bool valid_command = true;

    while (1) {
        print_prompt(valid_command);
        read_command(command, args);

        if (command[0] == '\0') {
            continue;
        }

        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "cd") == 0) {
            if (args[1] != NULL) {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
        } else {
            execute_command(command, args);
            valid_command = false;
        }
    }

    return 0;
}

void read_command(char* command, char* args[]) {
    char line[MAX_COMMAND_LENGTH];
    fgets(line, sizeof(line), stdin);
    line[strcspn(line, "\n")] = '\0';

    if (line[0] == '\0') {
        command[0] = '\0';
        args[0] = NULL;
        return;
    }

    char* token = strtok(line, " ");
    int i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
    strcpy(command, args[0]);

    if (strcmp(command, "cd") == 0 && args[1] != NULL) {
        list_directory_files(args[1]);
    }
}

void execute_command(char* command, char* args[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
    } else if (pid == 0) {
        execvp(command, args);
        fprintf(stderr, "Command not found: %s\n", command);
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status)) {
            fprintf(stderr, "Command terminated abnormally: %s\n", command);
        }
    }
}

void print_prompt(bool valid_command) {
    const char* color_blue = "\033[34m";
    const char* color_magenta = "\033[35m";
    const char* color_red = "\033[31m";
    const char* color_green = "\033[32m";
    const char* color_white = "\033[37m";
    const char* color_reset = "\033[0m";

    struct passwd *pw = getpwuid(getuid());
    const char *username = pw->pw_name;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    printf("%s%s  %s%s%s  %s%s%s | ",
           color_blue, color_reset,
           color_magenta, username, color_reset,
           color_red, cwd, color_reset);
    printf("%s", color_white);

    fflush(stdout);
}

void list_directory_files(const char* dirname) {
    DIR* dir;
    struct dirent* entry;
    struct stat filestat;

    if ((dir = opendir(dirname)) == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirname, entry->d_name);

        if (stat(full_path, &filestat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(filestat.st_mode)) {
            printf("%s%s%s/%s ", COLOR_RED, entry->d_name, COLOR_WHITE, COLOR_RESET);
        } else if (filestat.st_mode & S_IXUSR) {
            printf("%s%s%s*%s ", COLOR_GREEN, entry->d_name, COLOR_WHITE, COLOR_RESET);
        } else {
            printf("%s ", entry->d_name);
        }
    }
    printf("\n");

    closedir(dir);
}
