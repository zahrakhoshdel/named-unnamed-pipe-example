// histogram_calculator.c : the child program (A)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#define  WIDTHBYTES(bits)    (((bits)+31)/32*4)

#pragma pack(push, 1) 

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef long           LONG;




typedef struct tagBITMAPFILEHEADER {    
WORD    bfType;  
DWORD   bfSize;  
WORD    bfReserved1;  
WORD    bfReserved2;  
DWORD   bfOffBits;  
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {  
DWORD biSize; 
LONG biWidth;  
LONG biHeight;  
WORD biPlanes;  
WORD biBitCount; 
DWORD biCompression;  
DWORD biSizeImage;  
LONG biXPelsPerMeter;  
LONG biYPelsPerMeter;  
DWORD biClrUsed;  
DWORD biClrImportant;  
} BITMAPINFOHEADER;


typedef struct tagRGBQUAD { 
unsigned char  rgbBlue;  
unsigned char  rgbGreen; 
unsigned char  rgbRed;   
unsigned char  rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
BITMAPINFOHEADER bmiHeader;
RGBQUAD bmiColors[1];
} BITMAPINFO;


int main(int argc, char *argv[])
{

    ////////////named pipe : read image address from C
    int readfd1;
    char readbuf[100];
    int read_bytes;  
    
    readfd1 = open("myfifo2", O_RDONLY );
    if(readfd1 == -1){
        return 1;
    }
    read_bytes = read(readfd1, readbuf, sizeof(readbuf));
    readbuf[read_bytes] = '\0';
    printf("A RECIVED the image address from C \"%s\" \n", readbuf);

    close(readfd1);
    
    //named pipe: send histogram to C
    int writefd = open("myfifo3", O_WRONLY);
    printf("Opened write fifo3 file\n");


    //Declare related variables
    BITMAPFILEHEADER bmpFileHeader;
    BITMAPINFOHEADER bmpInfoHeader;
    RGBQUAD bmpColorTable[256];
    BYTE bmpValue[512 * 512];

    FILE* fp = fopen(readbuf, "rb");//Read the image.bmp file in the same directory.
	if(fp == NULL)
	{
		printf("Failed to open'%s'!\n", readbuf);
		return -1;
	}


    //Read image information
    fread(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);
    fread(bmpColorTable, sizeof(RGBQUAD), 256, fp);
    fread(bmpValue, 1, 512 * 512, fp);	
    //Store the image gray value in a one-bit array
    int grayValue[512 * 512] = { 0 };
    for (int i = 0; i < 512 * 512; i++)
    {
        grayValue[i] = bmpColorTable[bmpValue[i]].rgbBlue;
    }
    //Statistical histogram
    int grayCount[256] = { 0 };
    double grayFrequency[256] = { 0.0 };
    for (int i = 0; i < 512 * 512; i++)
    {
        grayCount[grayValue[i]]++;
    }
    for (int i = 0; i < 256; i++){
        if (grayCount[i]){
            //grayFrequency[i] = grayCount[i] / (512.0*512.0);
            //printf("Gray value %3d frequency is %6d frequency is %f\n", i, grayCount[i], grayFrequency[i]);
            //printf("Gray value %3d frequency is %6d \n", i, grayCount[i]);        
            
            if (write(writefd, grayCount, sizeof(grayCount)) == -1){
                printf("\n cannot be write");
                return 6;
            }
        }	       
    }
    //printf("\nWritten fifo3 file");
    close(writefd);
    printf("\nClosed written fifo3 \n");

    unlink("myfifo2");
    //Close the image file
    fclose(fp);

    return 0;
}