#pragma once
#include <stdlib.h>
#include <malloc.h>

#include "fileproc.h"

enum infoFieldBMP
{
	inf_size,
	inf_img_start_addr,
	inf_width,
	inf_height,
	inf_color_depth,
	inf_image_size,
	inf_resolution_horizontal,
	inf_resolution_vertical,
};

int checkBMP(unsigned char* pointer);
unsigned int infoBMP(unsigned char* pointer, int infoField);
void packBMP(char* containerName, char* objectName, int mode);
void unpackBMP(char* containerName);



int checkBMP(unsigned char* pointer)
{
	if (pointer[0] == 0x42 && pointer[1] == 0x4D)
	{
		return 0;
	}
	return -1;
}
unsigned int infoBMP(unsigned char* pointer, int infoField)
{
	unsigned int start = 0, finish = 0;

	switch (infoField)
	{
	case inf_size:					// size
		start = 0x02;
		finish = 0x05;
		break;
	case inf_img_start_addr:		// image start address
		start = 0x0A;
		finish = 0x0D;
		break;
	case inf_width:					// width
		start = 0x12;
		finish = 0x15;
		break;
	case inf_height:				// height
		start = 0x16;
		finish = 0x19;
		break;
	case inf_color_depth:			// color depth
		start = 0x1C;
		finish = 0x1D;
		break;
	case inf_image_size:			// image size (without header)
		start = 0x22;
		finish = 0x25;
		break;
	case inf_resolution_horizontal:	// horizontal resolution / meter
		start = 0x26;
		finish = 0x29;
		break;
	case inf_resolution_vertical:	// vertical resolution / meter
		start = 0x2A;
		finish = 0x2D;
		break;
	}

	unsigned int value = 0;
	for (unsigned int i = finish; i >= start; i--)
	{
		value = pointer[i] + (value * 256);
	}
	return value;
}
void packBMP(char* containerName, char* objectName, int mode)
{
	// Progress bar
	printf("0%% [                                                  ] 100%% \r0%% [");	// 100% = 2% + 2% + ((94%) // (44% + 44% + 2%)) + 2%. (2% = 1 клетка)

	// Открытие файла контейнера и запись его в массив
	FILE* containerFile;
	if (fopen_s(&containerFile, containerName, "rb") != 0)
	{
		int errorNumber = __LINE__ - 2;
		printLog("Error 3-%d. Unable to open file \"%s\".", errorNumber, containerName);
		exit(errorNumber);
	}
	size_t containerSize = fileSize(containerFile);
	unsigned char* container = (unsigned char*)malloc(containerSize * sizeof(unsigned char));
	if (container == NULL)
	{
		int errorNumber = __LINE__ - 2;
		fclose(containerFile);
		printLog("Error 3-%d. Allocate error.", errorNumber);
		exit(errorNumber);
	}
	container = getFile(containerFile, container);
	fclose(containerFile);
	if (checkBMP(container) != 0)
	{
		int errorNumber = __LINE__ - 2;
		free(container);
		printLog("Error 3-%d. \"%s\" is not a BMP image.", errorNumber, containerName);
		exit(errorNumber);
	}

	// Progress bar
	printf("\xFE");

	// Открытие файла объекта и запись его в массив
	FILE* objectFile;
	if (fopen_s(&objectFile, objectName, "rb") != 0)
	{
		int errorNumber = __LINE__ - 2;
		free(container);
		printLog("Error 3-%d. Unable to open file \"%s\".", errorNumber, objectName);
		exit(errorNumber);
	}
	size_t objectSize = fileSize(objectFile);
	unsigned char* object = (unsigned char*)malloc(objectSize * sizeof(unsigned char) + 1);
	if (object == NULL)
	{
		int errorNumber = __LINE__ - 2;
		fclose(objectFile);
		free(container);
		printLog("Error 3-%d. Allocate error.", errorNumber);
		exit(errorNumber);
	}
	object = getFile(objectFile, object);
	fclose(objectFile);
	object[objectSize] = '\0';

	// Progress bar
	printf("\xFE");

	// Открытие выходного файла и запись в него
	FILE* outputFile;
	if (fopen_s(&outputFile, "container.bmp", "wb") != 0)
	{
		int errorNumber = __LINE__ - 2;
		free(container);
		free(object);
		printLog("Error 3-%d. Unable to save file (\"container.bmp\").", errorNumber);
		exit(errorNumber);
	}
	switch (mode)
	{
	case 0:	// Текст в BMP
	{
		// Обработка текста (помещение одного символа в 4 байта)
		size_t tmpSize = objectSize * 4;
		if (tmpSize > containerSize)
		{
			int errorNumber = __LINE__ - 2;
			fclose(outputFile);
			free(container);
			free(object);
			printLog("Error 3-%d. Text is too big.", errorNumber);
			exit(errorNumber);
		}
		unsigned char* tmp = (unsigned char*)malloc(tmpSize);
		if (tmp == NULL)
		{
			int errorNumber = __LINE__ - 2;
			fclose(outputFile);
			free(container);
			free(object);
			printLog("Error 3-%d. Allocate error.", errorNumber);
			exit(errorNumber);
		}

		for (size_t i = 0; i < objectSize; i++)
		{
			tmp[i * 4 + 0] = (object[i] & 0b11000000) >> 6;
			tmp[i * 4 + 1] = (object[i] & 0b00110000) >> 4;
			tmp[i * 4 + 2] = (object[i] & 0b00001100) >> 2;
			tmp[i * 4 + 3] = (object[i] & 0b00000011) >> 0;
		}
		for (size_t i = 0; i < (size_t)infoBMP(container, inf_img_start_addr); i++)	// Запись заголовка изображения "контейнер"
		{
			fprintf(outputFile, "%c", container[i]);
		}
		for (size_t i = infoBMP(container, inf_img_start_addr), j = 0; i < containerSize; i++, j++)	// Запись "текста" в "контейнер"
		{
			fprintf(outputFile, "%c", (j < tmpSize ? ((container[i] & 0b11111100) | (tmp[j])) : (container[i] & 0b11111100)));

			// Progress bar
			if (i % (containerSize / 47) == 0 && i != 0)
			{
				printf("\xFE");
			}
		}
		free(tmp);

		// Запись размера текстового файла
		fprintf(outputFile, "%c", (objectSize & 0b00000000000000000000000011111111));
		fprintf(outputFile, "%c", (objectSize & 0b00000000000000001111111100000000) >> 8);
		fprintf(outputFile, "%c", (objectSize & 0b00000000111111110000000000000000) >> 16);
		fprintf(outputFile, "%c", (objectSize & 0b11111111000000000000000000000000) >> 24);
		
		fprintf(outputFile, "%c", 0x01);	// Метка выбранного режима
		break;
	}
	case 1:	// С потерей качества (Размер файла = больший файл + заголовок BMP + 1(режим))
	{
		if (checkBMP(object) != 0)
		{
			int errorNumber = __LINE__ - 2;
			fclose(objectFile);
			free(container);
			free(object);
			printLog("Error 3-%d. \"%s\" is not a BMP image.", errorNumber, objectName);
			exit(errorNumber);
		}
		for (size_t i = 0; i < (size_t)infoBMP(container, inf_img_start_addr); i++)	// Запись заголовка изображения "контейнер"
		{
			fprintf(outputFile, "%c", container[i]);
		}
		size_t size = (containerSize > objectSize ? containerSize : objectSize);	// Размер большего файла
		for (size_t i = infoBMP(container, inf_img_start_addr); i < size; i++)	// Если изображение "контейнер" закончилось, а изображение "объект" не закончилось - продолжить записывать "объект". Если закончился "объект", то удалить два бита в "контейнере" (для однородности изображения)
		{
			fprintf(outputFile, "%c", (i > containerSize ? ((object[i] & 0b11000000) >> 6) : ((container[i] & 0b11111100) | (i > objectSize ? 0b00000000 : ((object[i] & 0b11000000) >> 6)))));

			// Progress bar
			if (i % (size / 47) == 0 && i != 0)
			{
				printf("\xFE");
			}
		}
		for (size_t i = 0; i < 54; i++)	// Запись заголовка "объекта" в конец файла
		{
			fprintf(outputFile, "%c", object[i]);
		}

		fprintf(outputFile, "%c", 0x11);	// Метка выбранного режима
		break;
	}
	case 2:	// Без потерь качества (Размер файла = размер контейнера + размер объекта + 1(режим))
	{
		if (checkBMP(object) != 0)
		{
			int errorNumber = __LINE__ - 2;
			fclose(objectFile);
			free(container);
			free(object);
			printLog("Error 3-%d. \"%s\" is not a BMP image.", errorNumber, objectName);
			exit(errorNumber);
		}
		for (size_t i = 0; i < containerSize; i++)	// Запись изображения "контейнер"
		{
			fprintf(outputFile, "%c", container[i]);

			// Progress bar
			if ((i % (containerSize / 22) == 0 || i == containerSize - 1) && i != 0)
			{
				printf("\xFE");
			}
		}
		for (size_t i = 0; i < objectSize; i++)		// Запись изображения "объект"
		{
			fprintf(outputFile, "%c", object[i]);

			// Progress bar
			if ((i % (objectSize / 22) == 0 || i == objectSize - 1) && i != 0)
			{
				printf("\xFE");
			}
		}

		// Progress bar
		printf("\xFE");

		fprintf(outputFile, "%c", 0x12);	// Метка выбранного режима
		break;
	}
	}
	fclose(outputFile);
	free(container);
	free(object);

	// Progress bar
	printf("\xFE\n");
}
void unpackBMP(char* containerName)
{
	// Progress bar
	printf("0%% [                                                  ] 100%% \r0%% [");	// 100% = 2% + 96% + 2%. (2% = 1 клетка)

	// Открытие файла и запись его в массив
	FILE* containerFile;
	if (fopen_s(&containerFile, containerName, "rb") != 0)
	{
		int errorNumber = __LINE__ - 2;
		printLog("Error 2-%d. Unable to open file \"%s\".", errorNumber, containerName);
		exit(errorNumber);
	}
	size_t containerSize = fileSize(containerFile);
	unsigned char* container = (unsigned char*)malloc(containerSize * sizeof(unsigned char));
	if (container == NULL)
	{
		int errorNumber = __LINE__ - 2;
		fclose(containerFile);
		printLog("Error 2-%d. Allocate error.", errorNumber);
		exit(errorNumber);
	}
	container = getFile(containerFile, container);
	fclose(containerFile);
	if (checkBMP(container) != 0)
	{
		int errorNumber = __LINE__ - 2;
		free(container);
		printLog("Error 2-%d. \"%s\" is not a BMP image.", errorNumber, containerName);
		exit(errorNumber);
	}

	// Progress bar
	printf("\xFE");

	FILE* objectFile;
	switch (container[containerSize - 1])
	{
	case 0x01:	// Режим распаковки текста
	{
		if (fopen_s(&objectFile, "object.txt", "wb") != 0)
		{
			int errorNumber = __LINE__ - 2;
			free(container);
			printLog("Error 2-%d. Unable to save file (\"object.txt\").", errorNumber);
			exit(errorNumber);
		}
		unsigned char* object = (unsigned char*)malloc(containerSize);
		if (object == NULL)
		{
			int errorNumber = __LINE__ - 2;
			fclose(objectFile);
			free(container);
			printLog("Error 3-%d. Allocate error.", errorNumber);
			exit(errorNumber);
		}
		size_t objectSize = 0;
		for (unsigned int i = containerSize - 2; i >= containerSize - 5; i--)	//Размер текста
		{
			objectSize = container[i] + (objectSize * 256);
		}
		for (size_t i = 0, j = infoBMP(container, inf_img_start_addr) + 3; i < objectSize; i++, j += 4)	// Разбор
		{
			object[i] = ((container[j - 3] & 0b00000011) << 6) | ((container[j - 2] & 0b00000011) << 4) | ((container[j - 1] & 0b00000011) << 2) | (container[j] & 0b00000011);
		}
		for (size_t i = 0; i < objectSize; i++)	// Запись сообщения
		{
			fprintf(objectFile, "%c", object[i]);

			// Progress bar
			if (i % (objectSize / 48) == 0 && i != 0)
			{
				printf("\xFE");
			}
		}
		fclose(objectFile);
		free(object);
		break;
	}
	case 0x11:	// 1-ый режим распаковки (BMP)
	{
		if (fopen_s(&objectFile, "object.bmp", "wb") != 0)
		{
			int errorNumber = __LINE__ - 2;
			free(container);
			printLog("Error 2-%d. Unable to save file (\"object.bmp\").", errorNumber);
			exit(errorNumber);
		}
		for (size_t i = containerSize - 55; i < containerSize - 1; i++)	// Запись заголовка изображения "объект"
		{
			fprintf(objectFile, "%c", container[i]);
		}
		size_t size = 0;	// Определение размера изображения "объект"
		for (size_t i = containerSize - 49; i >= containerSize - 53; i--)
		{
			size = container[i] + (size * 256);
		}
		for (size_t i = infoBMP(container, inf_img_start_addr); i < size; i++)	// Запись изображения "объект"
		{
			fprintf(objectFile, "%c", ((container[i] & 0b00000011) << 6) | 0b00001111);

			// Progress bar
			if (i % (size / 48) == 0 && i != 0)
			{
				printf("\xFE");
			}
		}
		fclose(objectFile);
		break;
	}
	case 0x12:	// 2-ой режим распаковки (BMP)
	{
		if (fopen_s(&objectFile, "object.bmp", "wb") != 0)
		{
			int errorNumber = __LINE__ - 2;
			free(container);
			printLog("Error 2-%d. Unable to save file (\"object.bmp\").", errorNumber);
			exit(errorNumber);
		}
		size_t objectSize = containerSize - infoBMP(container, inf_size);
		for (size_t i = infoBMP(container, inf_size); i < containerSize; i++)	// Запись изображения "объект", начиная с конца изображения "контейнер"
		{
			fprintf(objectFile, "%c", container[i]);

			// Progress bar
			if (i % (objectSize / 48) == 0)
			{
				printf("\xFE");
			}
		}
		fclose(objectFile);
		break;
	}
	}

	free(container);

	// Progress bar
	printf("\xFE\n");
}
