#include "shader.h"
#include <stddef.h>
#include <stdio.h>
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