#pragma once

void PrintLog(const char* _Format, ...);
char* GetName(char* name);
char* GetFileExtansion(char* name);
size_t GetFileSize(FILE* stream);
unsigned char* GetFile(FILE* stream, unsigned char* destination);
int SignCheck(const char sign[], unsigned char* pointer);
