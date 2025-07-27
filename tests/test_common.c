/**
 * @file test_common.c
 * @brievoid cleanup_temp_files(void) {
    // Clean up temporary test files
    int result = system("rm -f " TEMP_PATH "test_temp_*");
    (void)result; // Suppress unused result warning
}plementation of common test utilities
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "test_common.h"
#include <sys/stat.h>
#include <time.h>

static char temp_filename[256];
static int temp_file_counter = 0;

/**
 * @brief Create a temporary file with given content
 */
char* create_temp_file(const char* content) {
    snprintf(temp_filename, sizeof(temp_filename), 
             TEMP_PATH "test_temp_%d_%ld.c", 
             temp_file_counter++, time(NULL));
    
    FILE* fp = fopen(temp_filename, "w");
    if (fp) {
        fputs(content, fp);
        fclose(fp);
        return temp_filename;
    }
    return NULL;
}

/**
 * @brief Clean up temporary files
 */
void cleanup_temp_files(void) {
    // Remove temporary files created during tests
    int result = system("rm -f " TEMP_PATH "test_temp_*");
    (void)result; // Suppress unused result warning
}

/**
 * @brief Set up test environment
 */
void setup_test_environment(void) {
    // Create temp directory if it doesn't exist
    mkdir("tests", 0755);
    mkdir("tests/temp", 0755);
    
    // Clean up any existing temp files
    cleanup_temp_files();
}

/**
 * @brief Run a compiler stage and return exit code
 */
int run_compiler_stage(const char* stage, const char* input_file, char** output_files) {
    char command[512];
    
    if (strcmp(stage, "cc0") == 0) {
        snprintf(command, sizeof(command), 
                "./bin/cc0 %s %s %s", 
                input_file, output_files[0], output_files[1]);
    } else if (strcmp(stage, "cc1") == 0) {
        snprintf(command, sizeof(command), 
                "./bin/cc1 %s %s %s %s", 
                output_files[0], output_files[1], output_files[2], output_files[3]);
    } else if (strcmp(stage, "cc2") == 0) {
        snprintf(command, sizeof(command), 
                "./bin/cc2 %s %s %s %s %s %s", 
                output_files[0], output_files[1], output_files[2], 
                output_files[3], output_files[4], output_files[5]);
    } else {
        return -1;
    }
    
    return system(command);
}

/**
 * @brief Read entire file content into a string
 */
char* read_file_content(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* content = malloc(length + 1);
    if (content) {
        size_t bytes_read = fread(content, 1, length, fp);
        content[bytes_read] = '\0'; // Handle partial reads gracefully
        (void)bytes_read; // Suppress unused variable warning
    }
    
    fclose(fp);
    return content;
}

/**
 * @brief Compare two files for equality
 */
int compare_files(const char* file1, const char* file2) {
    char* content1 = read_file_content(file1);
    char* content2 = read_file_content(file2);
    
    if (!content1 || !content2) {
        free(content1);
        free(content2);
        return -1;
    }
    
    int result = strcmp(content1, content2);
    free(content1);
    free(content2);
    return result;
}
