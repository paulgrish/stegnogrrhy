// Программа "помещает" файл в BMP изображение
//
// Контейнер - файл в который помещается объект
// Объект - файл который помещается в контейнер

#include "bmpproc.h"
#include "errorcodes.h"

int main(int argc, char* argv[])
{
	if (argc == 1)	// Справка по программе
	{
		printf("HOW TO USE:\n\n");

		printf("If you want to pack your file into BMP image:\n");
		printf("\t1. Drag and drop both files over the program\n");
		printf("\t2. Wait for \"Done!\". New file will be created (container.bmp)\n\n");

		printf("If you want to unpack your BMP container:\n");
		printf("\t1. Drag and drop your BMP file (container) over the program\n");
		printf("\t2. Wait for \"Done!\". New file will be created (object.*)\n\n");

		printf("Errors will be written to \"errors.log\"\n\n");
		system("pause");
	}
	else if (argc == 2)	// Распаковать файл
	{
		if (_stricmp(GetFileExtansion(argv[1]), "bmp") == 0)
		{
			UnpackBMP(argv[1]);
		}
		else
		{
			PrintLog("Error %d. \"%s\" is not a BMP container", EC_INVALID_FILE, GetName(argv[1]));
			exit(EC_INVALID_FILE);
		}
		printf("Done!\n");
		system("pause");
	}
	else if (argc == 3)	// Упаковать файл в BMP-изображение
	{
		if (_stricmp(GetFileExtansion(argv[1]), "bmp") == 0)
		{
			PackBMP(argv[1], argv[2]);
		}
		else if (_stricmp(GetFileExtansion(argv[2]), "bmp") == 0)
		{
			PackBMP(argv[2], argv[1]);
		}
		else
		{
			PrintLog("Error %d. There is no BMP file (%s, %s)\n", EC_INVALID_FILE, GetName(argv[1]), GetName(argv[2]));
			exit(EC_INVALID_FILE);
		}
		printf("Done!\n");
		system("pause");
	}
	else	// Непонятно что делать
	{
		PrintLog("Error %d. Too many files (%d).", EC_EXIT, argc - 1);
		exit(EC_EXIT);
	}
	return EC_SUCCESS;
}
