#pragma once

unsigned int InfoBMP(unsigned char* pointer, enum infoFieldBMP infoField);
void PackBMP(char* containerName, char* objectName);
void UnpackBMP(char* containerName);
