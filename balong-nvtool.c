#include <stdio.h>
#include <stdint.h>
#ifndef WIN32
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <windows.h>
#include "getopt.h"
#include "printf.h"
#include "buildno.h"
#endif

#include "nvfile.h"
#include "nvio.h"
#include "nvid.h"
#include "nvcrc.h"

// Open Image File nvram
FILE* nvf=0;

int mflag=-1;
int moff=0;
int mdata[128];
int midx;

int aflag=-1;
int aoff=0;
char adata[128];

// Offset to CRC field in file
uint32_t crcoff;

#ifdef MODEM 
// flag of direct work with nvram-file instead of kernel interface
int32_t kernelflag=0;
int ecall(char* name);
#endif

int test_crc();
void recalc_crc();

//****************************************************************
//* Checking the admissibility of work с CRC
//****************************************************************
void check_crcmode() {
  
if (crcmode == -1) {
  printf("\n Unsupported CRC type - nvram is read-only\n");
  exit(0);
}
}

//****************************************************************
//*   Parsing key parameters -m
//****************************************************************
void parse_mflag(char* arg) {
  
char buf[1024];
char* sptr, *offptr;
int endflag=0;

if (strlen(arg)>1024) {
    printf("\nThe argument to the -m switch is too long\n");
    exit(0);
}    
strcpy(buf,arg);

// проверяем на наличие байта +
sptr=strchr(buf,'+');

if (sptr != 0) {
  // есть смещение
  offptr=sptr+1;
  *sptr=0;
  sscanf(buf,"%i",&mflag);
  // looking for the first colon
  sptr=strchr(sptr+1,':');
  if (sptr == 0) {
    printf("\n - no replacement bytes are specified in the -m switch\n");
    exit(1);
  }
  *sptr=0;
  sscanf(offptr,"%x",&moff);
}

else {
  // no bias
  // looking for the first colon
  sptr=strchr(buf,':');
  if (sptr == 0) {
    printf("\n - no replacement bytes are specified in the -m switch\n");
    exit(1);
  }
  *sptr=0;
  sscanf(buf,"%i",&mflag);
}
sptr++; // on the first byte;
do {
  offptr=sptr;
  sptr=strchr(sptr,':'); // looking for another colon
  if (sptr != 0) *sptr=0;
  else endflag=1;
  sscanf(offptr,"%x",&mdata[midx]);
  if (mdata[midx] > 0xff) {
    printf("\n Wrong byte 0X%x in the key-m",mdata[midx]);
    exit(1);
  }
  midx++;
  sptr++;
} while (!endflag);
}


//****************************************************************
//*   Parsing key parameters -a
//****************************************************************
void parse_aflag(char* arg) {
  
char buf[1024];
char* sptr, *offptr;

if (strlen(arg)>1024) {
    printf("\nThe argument to the -a switch is too long\n");
    exit(0);
}    
strcpy(buf,arg);

// check for a byte +
sptr=strchr(buf,'+');

if (sptr != 0) {
  // there is an offset
  offptr=sptr+1;
  *sptr=0;
  sscanf(buf,"%i",&aflag);
  // looking for the first colon
  sptr=strchr(sptr+1,':');
  if (sptr == 0) {
    printf("\n - no replacement bytes are specified in the -a switch\n");
    exit(1);
  }
  *sptr=0;
  sscanf(offptr,"%x",&aoff);
}

else {
  // no bias
  // looking for the first colon
  sptr=strchr(buf,':');
  if (sptr == 0) {
    printf("\n - no replacement bytes are specified in the -a switch\n");
    exit(1);
  }
  *sptr=0;
  sscanf(buf,"%i",&aflag);
}
sptr++; // on the first byte;
strncpy(adata,sptr,128);
}

//****************************************************************
//*  Печать заголовка утилиты
//****************************************************************
void utilheader() {
  
printf("\n Utility for editing images of NVRAM devices on a chipset\n"
       " Hisilicon Balong, V1.0.%i (c) forth32, 2016, GNU GPLv3",BUILDNO);
#ifdef WIN32
printf("\n Windows 32bit port (c) rust3028, 2017");
#endif
printf("\n-------------------------------------------------------------------\n");
}


//****************************************************************
//* Печать help-блока
//****************************************************************
void utilhelp(char* utilname) {
 
utilheader();  
printf("\n Command line format:\n\n\
%s [the keys] <image file name NVRAM>\n\n\
 The following keys are valid:\n\n\
-l       - display image map NVRAM\n\
-u       - printing unique identifiers and settings\n\
-e       - extract all cells \n\
-x item  - extract cell item to file\n\
-d item  - dump cell item (d* - all cells)\n\
-r item:file - replace item cell with file content file\n\n\
-m item[+off]:nn[:nn...] - replace bytes in item with bytes specified in the command\n\
-a item[+off]:text - write a text string в item\n\
        *  если +off not specified - replacement starts at byte zero. The offset is specified in hex\n\n\
-i imei    write a new one IMEI\n\
-s serial- write down the new serial number\n\
-c       - extract all component files \n\
-k n     - extract all cells related to component n to directory COMPn\n\
-w dir   - import cell contents from catalog files dir/\n\
-b oem|simlock|all - select OEM, SIMLOCK or both codes\n"
#ifdef MODEM
"-f      - reload the modified nvram into the modem memory\n"
#endif
"\n",utilname);
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
       
void main(int argc,char* argv[]) {

int i;
int opt;
int res;
uint32_t pos;
char buf[10000];
int len;
char rfilename[200];
char* sptr;
FILE* in;
char imei[16];
char serial[20];
char nvfilename[100];

int xflag=-1;
int lflag=0;
int cflag=0;
int eflag=0;
int dflag=-1;
int rflag=-1;
int bflag=0;
int uflag=0;
int iflag=0;
int sflag=0;
int kflag=-1;
char wflag[200]={0};

while ((opt = getopt(argc, argv, "hlucex:d:r:m:b:i:s:a:k:w:f")) != -1) {
  switch (opt) {
   case 'h': 
    utilhelp(argv[0]);
    return;

   case 'f':
#ifndef MODEM
     printf("\n The -f switch is invalid on this platform\n");
     return;
#else
     kernelflag=1;
     break;
#endif     
    
   case 'b':
     switch (optarg[0]) {
       case 'o':
       case 'O':
	  bflag=1;
	  break;
	  
       case 's':
       case 'S':
	  bflag=2;
	  break;
	  
       case 'a':
       case 'A':
	  bflag=3;
	  break;

       default:  
	  printf("\n - incorrect value for the -b key\n");
          return;
     }
     break;
    
   case 'l':
     lflag=1;
     break;
     
   case 'u':
     uflag=1;
     break;
     
   case 'e':
     eflag=1;
     break;
     
   case 'c':
     cflag=1;
     break;
     
   case 'w':
     strcpy(wflag,optarg);
     break;
     
   case 'i':
     iflag=1;
     memset(imei,0,16);
     strncpy(imei,optarg,15);
     break;
     
   case 's':
     sflag=1;
     memset(serial,0xFF,20);
     strncpy(serial,optarg,strlen(optarg) < 16 ? strlen(optarg) : 16);
     break;
     
   case 'x':
     sscanf(optarg,"%i",&xflag);
     break;
     
   case 'k':
     sscanf(optarg,"%i",&kflag);
     break;
     
   case 'd':
     if (*optarg == '*') dflag=-2;  
     else sscanf(optarg,"%i",&dflag);
     break;
     
   case 'r':
     strcpy(buf,optarg);
     sptr=strchr(buf,':');
     if (sptr == 0) {
       printf("\n - The file name is not specified in the -r switch\n");
       return;
     }
     *sptr=0;
     sscanf(buf,"%i",&rflag);
     strcpy(rfilename,sptr+1);
     break;
     
   case 'm':
     parse_mflag(optarg);
     break;
     
   case 'a':
     parse_aflag(optarg);
     break;
     
   case '?':
   case ':':  
     return;
  }
}  


if (optind>=argc) {
#ifndef MODEM    
    printf("\n - Not an nvram image file, use the -h switch for a hint\n");
    return;
#else
    strcpy(nvfilename,"/mnvm2:0/nv.bin");
#endif    
}
else {
    strcpy(nvfilename,argv[optind]);
#ifdef MODEM
    // when working inside the modem, we automatically set the direct access flag when specifying an input file
    if (kernelflag==1) {
        printf("\n The -f switch does not apply to explicitly specifying a filename");
        kernelflag=0;
    }    
#endif
}


//------------ Parsing an nv file and extracting control structures from it -----------------------

// open the image nvram
nvf=fopen(nvfilename,"r+b");
if (nvf == 0) {
  printf("\n File %s not found\n",argv[optind]);
  return;
}

// read the image header
res=fread(&nvhd,1,sizeof(nvhd),nvf);
if (res != sizeof(nvhd)) {
  printf("\n - Error reading file %s\n",argv[optind]);
  return;
}
if (nvhd.magicnum != FILE_MAGIC_NUM) {
  printf("\n - File %s is not an NVRAM image\n",argv[optind]);
  return;
}

// Determine the type of CRC
switch (nvhd.crcflag) {
  case 0:
    crcmode=0;
    break;
    
  case 1:
    crcmode=1;
    break;

  case 3:
    crcmode=3;
    break;

  case 8:
    crcmode=2;
    break;

  default:
    crcmode=-1;
    break;

}

//----- Reading the file directory

fseek(nvf,nvhd.file_offset,SEEK_SET);
pos=nvhd.ctrl_size;

for(i=0;i<nvhd.file_num;i++) {
 // we take out the information available in the file 
 fread(&flist[i],1,36,nvf);
 // calculate the offset to the file data
 flist[i].offset=pos;
 pos+=flist[i].size;
}

// get the offset to the field CRC
crcoff=pos;

//----- We read the catalog of cells
fseek(nvf,nvhd.item_offset,SEEK_SET);
itemlist=malloc(nvhd.item_size);
fread(itemlist,1,nvhd.item_size,nvf);

// output of maps and parameters
if (lflag) {
  utilheader();
  print_hd_info();
  print_filelist();
  print_itemlist();
  return;
}  

// Derivation of unique parameters
if (uflag) {
  print_data();
  return;
}  

// extracting files and cells
if (cflag) extract_files();
if (eflag) extract_all_item();
if (kflag != -1) extract_comp_items(kflag);
if (xflag != -1) {
  printf("\n Retrieving cell %i\n",xflag);
  item_to_file(xflag,"");
}  

// View cell dump
if (dflag != -1) {
  if (dflag != -2) dump_item(dflag); // one cell
  else 
  // all cells
     for(i=0;i<nvhd.item_count;i++)  dump_item(itemlist[i].id);
}

// Bulk import of cells (switch -w)
if (strlen(wflag) != 0) {
check_crcmode();
mass_import(wflag);
}

// Replacing cells
if (rflag != -1) {
  check_crcmode();
  len=itemlen(rflag);
  if (len == -1) {
    printf("\n - Cell %i not found\n",rflag);
    return;
  }  
  in=fopen(rfilename,"rb");
  if (in == 0) {
    printf("\n 
- Error opening file %s\n",rfilename);
    return;
  }
  fseek(in,0,SEEK_END);
  if (ftell(in) != len) {
    printf("\n - File size (%u) does not match cell size (%i)\n",(uint32_t)ftell(in),len);
    return;
  }
  fseek(in,0,SEEK_SET);
  fread(buf,1,len,in);
  save_item(rflag,buf);
  printf("\n Cell %i was written successfully\n",rflag);
}

// direct cell editing
if (mflag != -1) {
  check_crcmode();
  len=load_item(mflag,buf);
  if (len == -1) {
    printf("\n - Cell %i not found\n",mflag);
    return;
  }  
  if ((midx+moff) > len) {
    printf("\n - cell length %i exceeded\n",mflag);
    return;
  } 
  
  for(i=0;i<midx;i++) buf[moff+i]=mdata[i];
  save_item(mflag,buf);
  printf("\n Cell %i edited\n",mflag);
 
}

// Writing text strings to a cell
if (aflag != -1) {
 check_crcmode();
 len=load_item(aflag,buf);
  if (len == -1) {
    printf("\n - Cell %i not found\n",mflag);
    return;
  }  
  if ((strlen(adata)+aoff) > len) {
    printf("\n - cell length %i exceeded\n",mflag);
    return;
  } 
  memcpy(buf+aoff,adata,strlen(adata));
  save_item(aflag,buf);
  printf("\n Cell %i edited\n",aflag);
}  

// selection of lock codes
if (bflag) {
  if (nvhd.version>121) {
    printf("\n Computing codes is not supported on this platform.");
  }
  else {
   utilheader();
   switch (bflag) {
    case 1:
      brute(1);
      return;
      
    case 2:
      brute(2);
      return;
      
    case 3:
      brute(1);
      brute(2);
      return;
  }
 } 
}

// IMEI entry
if (iflag) {
  check_crcmode();
  write_imei(imei);
}

// serial record
if (sflag) {
  check_crcmode();
  write_serial(serial);
}

// re-computation of an array КС
if (crcmode != -1) recalc_crc();
fclose(nvf);

#ifdef MODEM
  if (kernelflag) system("ecall bsp_nvm_reload");
#endif
printf("\n");  
  
}
