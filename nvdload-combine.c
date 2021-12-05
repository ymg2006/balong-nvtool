// 
//   Утилита для сборки раздела прошивки nvdload из компонентов nvimg и xml
//  
//
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include "printf.h"
#endif

void main(int argc, char* argv[]) {

FILE* in;  
FILE* out;

uint32_t cs=0;

char* bufimg; // буфер под NVIMG
char* bufxml; // буфер под XML

struct {
  uint32_t sig1;
  uint32_t start1;
  uint32_t len1;
  uint32_t sig2;
  uint32_t start2;
  uint32_t len2;
  uint8_t zero[60];
} hdr;  


if (argc != 3) {
  printf("\n! Не указаны имена файлов для объединения\n\
Формат командной строки:\n\n\
  %s nvimg-file xml-file\n\n\
Выходной файл - nvdload.nvd\n\n",argv[0]);
  exit(0);
}

// заполняем заголовок
#ifndef WIN32
bzero(&hdr,sizeof(hdr));
#else
memset(&hdr, 0, sizeof(hdr));
#endif
hdr.sig1 = 0x766e;
hdr.sig2 = 0x766e;


// читаем раздел NVIMG
in=fopen(argv[1],"rb");
if (in == 0) {
  printf("\n Error opening NVIMG file %s\n",argv[1]);
  exit(0);
}

fseek(in,0,SEEK_END);
hdr.len1=ftell(in);
rewind(in);
bufimg=malloc(hdr.len1);
fread(bufimg,1,hdr.len1,in);
fclose(in);

// читаем раздел XML
in=fopen(argv[2],"rb");
if (in == 0) {
  printf("\n Error opening XML file %s\n",argv[1]);
  exit(0);
}

fseek(in,0,SEEK_END);
hdr.len2=ftell(in);
rewind(in);
bufxml=malloc(hdr.len2);
fread(bufxml,1,hdr.len2,in);
fclose(in);

// вычисляем позиции компонентов
hdr.start1=0x54;
hdr.start2=hdr.start1+hdr.len1;

// записываем компоненты
out=fopen("nvdload.nvd","wb");
fwrite(&hdr,1,0x54,out);

printf("\n Type start size\n--------------------------");

printf("\nNVIMG  %08x  %08x",hdr.start1,hdr.len1);
fwrite(bufimg,1,hdr.len1,out);

printf("\nXML    %08x  %08x",hdr.start2,hdr.len2);
fwrite(bufxml,1,hdr.len2,out);

// хвост файла
fwrite(&cs,1,4,out);

printf("\n");  

}
