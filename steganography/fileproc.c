#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "fileproc.h"

void PrintLog(const char* _Format, ...)
{
	FILE* logFile;
	if (fopen_s(&logFile, "error.log", "ab") == 0)
	{
		const time_t sysTime = time(NULL);
		char strSysTime[26];
		ctime_s(strSysTime, sizeof(strSysTime), &sysTime);
		strSysTime[24] = '\0';
		fprintf(logFile, "[%s]: ", strSysTime);

		va_list vl;
		va_start(vl, _Format);
		for (const char* p = _Format; *p; p++)
		{
			if (*p != '%')	// Напечатать символ
			{
				fputc(*p, logFile);
				continue;
			}
			switch (*(++p))
			{
			case 'd':	// Int
			{
				fprintf(logFile, "%d", va_arg(vl, int));
				break;
			}
			case 'f':	// Float
			{
				fprintf(logFile, "%f", va_arg(vl, float));
				break;
			}
			case 's':	// String
			{
				for (char* sval = va_arg(vl, char*); *sval; sval++)
				{
					fputc(*sval, logFile);
				}
				break;
			}
			default:	// Все остальное
			{
				fputc(*p, logFile);
				break;
			}
			}
		}
		va_end(vl);
		fputc('\n', logFile);
		fclose(logFile);
	}
	else
	{
		printf("Unable to create a log file/n");
	}
}
char* GetName(char* name)
{
	return strrchr(name, (strrchr(name, '\\') > strrchr(name, '/') ? '\\' : '/')) + 1;
}
char* GetFileExtansion(char* name)
{
	return (strrchr(name, '.') + 1);
}
size_t GetFileSize(FILE* stream)
{
	fseek(stream, 0, SEEK_END);
	size_t fileSize = ftell(stream);
	fseek(stream, 0, SEEK_SET);
	return fileSize;
}
unsigned char* GetFile(FILE* stream, unsigned char* destination)
{
	size_t size = GetFileSize(stream);
	for (size_t i = 0; i < size; i++)
	{
		destination[i] = fgetc(stream);
	}
	return destination;
}
int SignCheck(const char sign[], unsigned char* pointer)
{
	size_t length = strlen(sign);
	for (size_t i = 0; i < length; i++)
	{
		if (sign[i] != pointer[i])
		{
			return 0;
		}
	}
	return 1;
}
