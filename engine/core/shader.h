#ifndef SHADER_H
#define SHADER_H

#include <cglm/cglm.h>
#include <cglm/struct.h>

typedef struct
{
    uint32_t* data;
    size_t    size;
} SPVBuffer;

char* readFileToString(const char* filename, uint32_t* file_size);
char* appendFileExt(const char* fileName, const char* ext);

SPVBuffer loadSPVFile(const char* filename);

#endif
