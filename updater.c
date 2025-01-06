#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>

#define TEMP_FILE_PATH "/%s/.applist"  // User-specific temporary file path
#define MAX_AGE 86400  // 24 hours in seconds

// Function to get the current user's home directory
char* get_home_directory() {
    char* home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Failed to get HOME environment variable\n");
        exit(EXIT_FAILURE);
    }
    return home;
}

void get_commands(FILE *temp_file) {
    const char *dirs[] = { "/usr/bin", "/bin", "/usr/local/bin", "/sbin", "/usr/sbin" };
    struct dirent *entry;
    DIR *dp;

    // Iterate through each directory to collect commands
    for (int i = 0; i < 5; i++) {
        dp = opendir(dirs[i]);
        if (dp == NULL) {
            perror("opendir");
            continue;
        }

        while ((entry = readdir(dp))) {
            if (entry->d_type == DT_REG) {  // Regular file (executable)
                char command_path[512];
                snprintf(command_path, sizeof(command_path), "%s/%s", dirs[i], entry->d_name);
                fprintf(temp_file, "%s=%s\n", entry->d_name, command_path);
            }
        }
        closedir(dp);
    }

    // Use `compgen -c` to get additional commands
    FILE *compgen_output = popen("compgen -c", "r");
    if (compgen_output == NULL) {
        perror("popen");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), compgen_output)) {
        line[strcspn(line, "\n")] = '\0';  // Remove newline character
        fprintf(temp_file, "%s=%s\n", line, line);  // Command = Path (same for now)
    }

    fclose(compgen_output);

    // Add applications from .desktop files in ~/.local/share/applications
    const char *desktop_dir = "/home/%s/.local/share/applications";  // Assuming user's home dir
    char desktop_path[512];
    snprintf(desktop_path, sizeof(desktop_path), desktop_dir, getenv("USER"));

    dp = opendir(desktop_path);
    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".desktop")) {
            // Read the .desktop file
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", desktop_path, entry->d_name);
            FILE *desktop_file = fopen(file_path, "r");
            if (desktop_file == NULL) {
                perror("fopen");
                continue;
            }

            char line[512];
            char name[256] = {0};
            char exec[512] = {0};

            // Read the .desktop file content to extract Name and Exec
            while (fgets(line, sizeof(line), desktop_file)) {
                if (strncmp(line, "Name=", 5) == 0) {
                    strncpy(name, line + 5, sizeof(name) - 1);
                    name[strcspn(name, "\n")] = '\0';  // Remove newline
                } else if (strncmp(line, "Exec=", 5) == 0) {
                    strncpy(exec, line + 5, sizeof(exec) - 1);
                    exec[strcspn(exec, "\n")] = '\0';  // Remove newline
                }
            }

            fclose(desktop_file);

            // If both Name and Exec are found, write them to the file
            if (strlen(name) > 0 && strlen(exec) > 0) {
                fprintf(temp_file, "%s=%s\n", name, exec);
            }
        }
    }

    closedir(dp);
}

// Function to check if the temporary file is older than 24 hours
int is_temp_file_old(const char *temp_file_path) {
    struct stat file_stat;
    if (stat(temp_file_path, &file_stat) == -1) {
        return 1;  // File does not exist, consider it old
    }

    time_t current_time = time(NULL);
    double age = difftime(current_time, file_stat.st_mtime);
    return age > MAX_AGE;
}

// Function to create or update the temporary file with the application commands
void update_temp_file(const char *temp_file_path) {
    FILE *temp_file = fopen(temp_file_path, "w");
    if (temp_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    get_commands(temp_file);
    fclose(temp_file);
}

int main() {
    char *home_dir = get_home_directory();
    char temp_file_path[512];
    snprintf(temp_file_path, sizeof(temp_file_path), TEMP_FILE_PATH, home_dir);
    if (is_temp_file_old(temp_file_path)) {
        printf("AppIndex file is old or missing. Recreating it...\n");
        update_temp_file(temp_file_path);
    } else {
        printf("AppIndex file is up-to-date.\n");
    }
    return 0;
}

