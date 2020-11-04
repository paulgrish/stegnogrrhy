#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmpproc.h"
#include "fileproc.h"
#include "errorcodes.h"

#define BMPSIGN "BM"

enum infoFieldBMP
{
	inf_size = 0x02,
	inf_img_start_addr = 0x0A,
	inf_header_size = 0x0E,
	inf_width = 0x12,
	inf_height = 0x16,
	inf_color_depth = 0x1C,
	inf_image_size = 0x22,
	inf_resolution_horizontal = 0x26,
	inf_resolution_vertical = 0x2A,
	inf_color = 0x2E,
	inf_imprtnt_color = 0x32,
};

unsigned int InfoBMP(unsigned char* pointer, enum infoFieldBMP infoField)
{
	unsigned int bytes = (infoField == inf_color_depth ? 2 : 4);
	unsigned int value = 0;
	for (unsigned int i = infoField + bytes; i >= (unsigned int)infoField; i--)
	{
		value = pointer[i] + (value * 256);
	}
	return value;
}
void PackBMP(char* containerName, char* objectName)
{
	// Progress bar
	printf("0%% [                                                  ] 100%% \r0%% [");	// 100% = 2% + 48% + 48% + 2%. (2% = 1 клетка)

	// Открытие файла контейнера и запись его в массив
	FILE* containerFile;
	if (fopen_s(&containerFile, containerName, "rb"))
	{
		PrintLog("Error %d. Unable to open file \"%s\".", EC_OPEN_FAILED, GetName(containerName));
		exit(EC_OPEN_FAILED);
	}
	size_t containerSize = GetFileSize(containerFile);
	unsigned char* container = (unsigned char*)malloc(containerSize);
	if (container == NULL)
	{
		fclose(containerFile);
		PrintLog("Error %d. Allocate error.", EC_ALLOC_FAILED);
		exit(EC_ALLOC_FAILED);
	}
	container = GetFile(containerFile, container);
	fclose(containerFile);
	if (!SignCheck(BMPSIGN, &container[0]))
	{
		printf("");
		free(container);
		PrintLog("Error %d. \"%s\" is not a BMP image.", EC_INVALID_FILE, GetName(containerName));
		exit(EC_INVALID_FILE);
	}

	// Progress bar
	printf("\xFE");

	// Открытие файла объекта и запись его в массив
	FILE* objectFile;
	if (fopen_s(&objectFile, objectName, "rb"))
	{
		int errorNumber = __LINE__ - 2;
		free(container);
		PrintLog("Error %d. Unable to open file \"%s\".", EC_OPEN_FAILED, GetName(objectName));
		exit(EC_OPEN_FAILED);
	}
	size_t objectSize = GetFileSize(objectFile);
	unsigned char* object = (unsigned char*)malloc(objectSize);
	if (object == NULL)
	{
		fclose(objectFile);
		free(container);
		PrintLog("Error %d. Allocate error.", EC_ALLOC_FAILED);
		exit(EC_ALLOC_FAILED);
	}
	object = GetFile(objectFile, object);
	fclose(objectFile);

	// Обработка текста (помещение одного символа в 4 байта)
	size_t tmpSize = objectSize * 4;
	unsigned char* tmp = (unsigned char*)malloc(tmpSize);
	if (tmp == NULL)
	{
		free(container);
		free(object);
		PrintLog("Error %d. Allocate error.", EC_ALLOC_FAILED);
		exit(EC_ALLOC_FAILED);
	}
	for (size_t i = 0, j = 0; i < objectSize; i++, j += 4)
	{
		tmp[j + 0] = (object[i] & 0b11000000) >> 6;
		tmp[j + 1] = (object[i] & 0b00110000) >> 4;
		tmp[j + 2] = (object[i] & 0b00001100) >> 2;
		tmp[j + 3] = (object[i] & 0b00000011) >> 0;

		// Progress bar
		if (i % (objectSize / 24) == 0 && i != 0)
		{
			printf("\xFE");
		}
	}
	free(object);

	// Запись размера
	container[0x2E] = (unsigned char)(objectSize & 0b00000000000000000000000011111111) ^ 0b00101110;
	container[0x2F] = (unsigned char)((objectSize & 0b00000000000000001111111100000000) >> 8) ^ 0b11100110;
	container[0x30] = (unsigned char)((objectSize & 0b00000000111111110000000000000000) >> 16) ^ 0b11010011;
	container[0x31] = (unsigned char)((objectSize & 0b11111111000000000000000000000000) >> 24) ^ 0b11010110;

	// Запись расширения
	unsigned char* ext = GetFileExtansion(objectName);
	container[0x32] = ext[0] ^ 0b01010101;
	container[0x33] = ext[1] ^ 0b01110101;
	container[0x34] = ext[2] ^ 0b01011101;
	container[0x35] = ext[3] ^ 0b00010111;

	// Метка контейнера
	container[0x09] = 0x01;

	// Открытие выходного файла и запись в него
	FILE* outputFile;
	if (fopen_s(&outputFile, "container.bmp", "wb"))
	{
		free(container);
		free(tmp);
		PrintLog("Error %d. Unable to save file (\"container.bmp\").", EC_OPEN_FAILED);
		exit(EC_OPEN_FAILED);
	}
	for (size_t i = 0; i < InfoBMP(container, inf_img_start_addr); i++)	// Запись заголовка изображения "контейнер"
	{
		fprintf(outputFile, "%c", container[i]);
	}
	size_t size = containerSize > tmpSize ? containerSize : tmpSize + 54;
	for (size_t i = InfoBMP(container, inf_img_start_addr), j = 0; i < size; i++, j++)	// Запись "объекта" в "контейнер"
	{
		if (i < containerSize)	// Контейнер не закончился
		{
			if (j < tmpSize)	// Объект не закончился - записать 2 бита объекта в последние 2 бита контейнера
			{
				fprintf(outputFile, "%c", container[i] & 0b11111100 | tmp[j]);
			}
			else	// Объект закончился - записать контейнер без потерь
			{
				fprintf(outputFile, "%c", container[i]);
			}
		}
		else	// Контейнер закончился - записать объект
		{
			fprintf(outputFile, "%c", tmp[j]);
		}

		// Progress bar
		if (i % (size / 24) == 0 && i != 0)
		{
			printf("\xFE");
		}
	}
	free(container);
	free(tmp);

	fclose(outputFile);

	// Progress bar
	printf("\xFE\n");
}
void UnpackBMP(char* containerName)
{
	// Progress bar
	printf("0%% [                                                  ] 100%% \r0%% [");	// 100% = 2% + 48% + 48% + 2%. (2% = 1 клетка)

	// Открытие файла и запись его в массив
	FILE* containerFile;
	if (fopen_s(&containerFile, containerName, "rb"))
	{
		PrintLog("Error %d. Unable to open file \"%s\".", EC_OPEN_FAILED, GetName(containerName));
		exit(EC_OPEN_FAILED);
	}
	size_t containerSize = GetFileSize(containerFile);
	unsigned char* container = (unsigned char*)malloc(containerSize);
	if (container == NULL)
	{
		fclose(containerFile);
		PrintLog("Error %d. Allocate error.", EC_ALLOC_FAILED);
		exit(EC_ALLOC_FAILED);
	}
	container = GetFile(containerFile, container);
	fclose(containerFile);
	if (!SignCheck(BMPSIGN, &container[0]))
	{
		free(container);
		PrintLog("Error %d. \"%s\" is not a BMP image.", EC_INVALID_FILE, GetName(containerName));
		exit(EC_INVALID_FILE);
	}
	if (container[0x09] != 0x01)
	{
		free(container);
		PrintLog("Error %d. \"%s\" is not a container.", EC_INVALID_CONTAINER, GetName(containerName));
		exit(EC_INVALID_CONTAINER);
	}

	// Progress bar
	printf("\xFE");

	// Расширение и имя нового файла
	char ext[5] = { "\0" };
	ext[0] = container[0x32] ^ 0b01010101;
	ext[1] = container[0x33] ^ 0b01110101;
	ext[2] = container[0x34] ^ 0b01011101;
	ext[3] = container[0x35] ^ 0b00010111;
	char objectName[256];
	strcpy_s(objectName, 256, "object.");
	strcat_s(objectName, 256, ext);

	// Распаковка и запись файла
	FILE* objectFile;
	if (fopen_s(&objectFile, objectName, "wb"))
	{
		free(container);
		PrintLog("Error %d. Unable to save file (\"object.txt\").", EC_OPEN_FAILED);
		exit(EC_OPEN_FAILED);
	}
	size_t objectSize = InfoBMP(container, inf_color) ^ 0b11010110110100111110011000101110;
	unsigned char* object = (unsigned char*)malloc(objectSize);
	if (object == NULL)
	{
		fclose(objectFile);
		free(container);
		PrintLog("Error %d. Allocate error.", EC_ALLOC_FAILED);
		exit(EC_ALLOC_FAILED);
	}
	for (size_t i = 0, j = InfoBMP(container, inf_img_start_addr) + 3; i < objectSize; i++, j += 4)	// Разбор
	{
		object[i] = ((container[j - 3] & 0b00000011) << 6) | ((container[j - 2] & 0b00000011) << 4) | ((container[j - 1] & 0b00000011) << 2) | (container[j] & 0b00000011);

		// Progress bar
		if (i % (objectSize / 24) == 0 && i != 0)
		{
			printf("\xFE");
		}
	}
	for (size_t i = 0; i < objectSize; i++)	// Запись
	{
		fprintf(objectFile, "%c", object[i]);

		// Progress bar
		if (i % (objectSize / 24) == 0 && i != 0)
		{
			printf("\xFE");
		}
	}
	fclose(objectFile);
	free(object);
	free(container);

	// Progress bar
	printf("\xFE\n");
}
