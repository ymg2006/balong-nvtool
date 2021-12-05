// 
//   Утилита для разделения раздела прошивки nvdload на отдельные компоненты
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

#ifndef WIN32
char buf[1024*1024*5]; // буфер 5М под компоненты
#else
static char buf[1024*1024*5]; // буфер 5М под компоненты
#endif

struct {
  uint32_t sig1;
  uint32_t start1;
  uint32_t len1;
  uint32_t sig2;
  uint32_t start2;
  uint32_t len2;
} hdr;  


if (argc != 2) {
  printf("\n File name not specified - section NVRAM\n");
  exit(0);
}

in=fopen(argv[1],"rb");
if (in == 0) {
  printf("\n File open error %s\n",argv[1]);
  exit(0);
}
  
fread(&hdr,1,sizeof(hdr),in);
printf("\n Тип    старт    размер\n--------------------------");
if ((hdr.sig1 != 0x766e) || (hdr.sig2 != 0x766e)) {
  printf("\n File header format error - file is not a firmware section NVDLOAD\n");
  exit(0);
}  

printf("\nNVIMG  %08x  %08x",hdr.start1,hdr.len1);
out=fopen("nvimg.nvm","wb");
fseek(in,hdr.start1,SEEK_SET);
fread(buf,1,hdr.len1,in);
fwrite(buf,1,hdr.len1,out);
fclose(out);

printf("\nXML    %08x  %08x",hdr.start2,hdr.len2);
out=fopen("nvimg.xml","wb");
fseek(in,hdr.start2,SEEK_SET);
fread(buf,1,hdr.len2,in);
fwrite(buf,1,hdr.len2,out);
fclose(out);

// хвост файла
printf("\n Tail: ");
while (fread(buf,1,1,in) == 1) printf("%02x ",(uint32_t)buf[0]&0xff);

printf("\n");  

}
