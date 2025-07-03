#include "shader.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "./logging/log.h"

// -- -- -- -- -- -- Shader functions -- -- -- -- -- --- --
//
char* readFileToString(const char* filename, uint32_t* file_size)
{
    // Open the file in read mode ("r")
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("\nCould not open file %s\n", filename);
        fflush(stdout);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    char* content = (char*) malloc((*file_size) * sizeof(char));
    if (!content) {
        printf("\nMemory allocation failed\n");
        fclose(file);
        return NULL;
    }
    //for (size_t i = 0; i < *file_size + 1; i++) {
    //    content[i] = '\0';
    //}
    fread(content, sizeof(char), *file_size, file);

    fclose(file);
    return content;
}

char* appendFileExt(const char* fileName, const char* ext)
{
    size_t utf8_len  = strlen(fileName);
    size_t total_len = utf8_len + 4 + 1;
    char*  full_path = malloc(total_len);
    if (!full_path) {
        LOG_ERROR("Memory allocation failed for shader path");
        return NULL;
    }
    snprintf(full_path, total_len, "%s.%s", fileName, ext);
    return full_path;
}

SPVBuffer loadSPVFile(const char* filename)
{
    SPVBuffer buffer = {NULL, 0};
    FILE*     file   = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return buffer;
    }

    fseek(file, 0, SEEK_END);
    unsigned long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        fprintf(stderr, "Invalid SPV file size.\n");
        fclose(file);
        return buffer;
    }

    buffer.size = fileSize;
    buffer.data = (uint32_t*) malloc(fileSize);
    if (!buffer.data) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(file);
        return buffer;
    }

    if (fread(buffer.data, 1, fileSize, file) != fileSize) {
        fprintf(stderr, "Failed to read the file.\n");
        free(buffer.data);
        buffer.data = NULL;
        buffer.size = 0;
        fclose(file);
        return buffer;
    }

    fclose(file);
    return buffer;
}