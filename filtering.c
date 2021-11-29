  
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#define kernelSize 5
#define sigma 0.4


//#include "helpers.h"
typedef uint8_t  BYTE;
typedef uint32_t DWORD;
typedef int32_t  LONG;
typedef uint16_t WORD;

typedef struct
{
    WORD   bfType;
    DWORD  bfSize;
    WORD   bfReserved1;
    WORD   bfReserved2;
    DWORD  bfOffBits;
} __attribute__((__packed__))
BITMAPFILEHEADER;

typedef struct
{
    DWORD  biSize;
    LONG   biWidth;
    LONG   biHeight;
    WORD   biPlanes;
    WORD   biBitCount;
    DWORD  biCompression;
    DWORD  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    DWORD  biClrUsed;
    DWORD  biClrImportant;
} __attribute__((__packed__))
BITMAPINFOHEADER;

typedef struct
{
    BYTE  rgbtBlue;
    BYTE  rgbtGreen;
    BYTE  rgbtRed;
} __attribute__((__packed__))
RGBTRIPLE;

void createGaussianFilter(double gaussKernel[kernelSize][kernelSize])
{
    // Standard Deviation (sigma) is set to 1.0
    double numeratorForExp;
    double denominatorForExp = 2.0 * sigma * sigma;
 
    // Sum is for normalization
    double sum = 0.0;
 
    // Generating 3x3 kernel
    for (int y = -(kernelSize/2); y < 1+(kernelSize/2); y++)
    {
    	//printf("y = %d 	", y);
        for(int x = -(kernelSize/2); x < 1+(kernelSize/2); x++)
        {
        	//printf("x = %d 	", x);
            numeratorForExp = x*x + y*y;
            gaussKernel[y + (kernelSize/2)][x + (kernelSize/2)] = (exp(-(numeratorForExp)/denominatorForExp) )/(M_PI * denominatorForExp);
            sum += gaussKernel[y + (kernelSize/2)][x + (kernelSize/2)];
        }
    }
 
    // normalize the Kernel
    for(int y = 0; y < kernelSize; y++)
    {
        for(int x = 0; x < kernelSize; x++)
            gaussKernel[y][x] /= sum;
    }
}


void ApplyingGaussianFilter(double gaussKernel[kernelSize][kernelSize], int outputImageHeight, int outputImageWidth, char *inputRAWfile, char *outputGaussRAWfile)
{
    //Creating a NULL-initialised RAW image buffer with increased dimensions
    unsigned char filterBuffer[outputImageHeight + kernelSize - 1][outputImageWidth + kernelSize - 1];
    for(int j=0; j<outputImageHeight + kernelSize - 1; j++)
    	for(int i=0; i<outputImageWidth + kernelSize - 1; i++)
    			filterBuffer[j][i] = 0;
    
	unsigned char tempRAWimage[outputImageHeight][outputImageWidth];
    FILE *file = fopen(inputRAWfile, "rb");  
	if (!(file))
	{
		printf("Cannot open input file!!");
		exit(3);
	}
	
	fread(tempRAWimage, sizeof(unsigned char), outputImageHeight*outputImageWidth, file);
	fclose(file);
	
	printf("\n");
	//Copying the raw image in to middle of the block
	for(int j=0; j<outputImageHeight; j++)
    	for(int i=0; i<outputImageWidth; i++)
    			filterBuffer[ j+(kernelSize/2) ][ i+(kernelSize/2) ] = tempRAWimage[j][i];

    //Copying the top edge to the upper blank space
	for(int j=0; j<(kernelSize/2)+1; j++)
    	for(int i=0; i<outputImageWidth; i++)
    			filterBuffer[j][ i+(kernelSize/2) ] = tempRAWimage[0][i];

    //Copying the bottom edge to the lower blank space
    for(int j=outputImageHeight; j < outputImageHeight + kernelSize - 1; j++)
    	for(int i=0; i<outputImageWidth; i++)
    			filterBuffer[j][ i+(kernelSize/2) ] = tempRAWimage[outputImageHeight-1][i];

    //Copying the left edge to blank space on eft
    for(int j=0; j<outputImageHeight; j++)
    	for(int i=0; i < (kernelSize/2)+1; i++)
    			filterBuffer[ j+(kernelSize/2) ][i] = tempRAWimage[j][0];

    //Copying the right edge to blank space on right
    for(int j=0; j<outputImageHeight; j++)
    	for(int i=outputImageWidth; i < outputImageWidth + kernelSize - 1; i++)
    			filterBuffer[ j+(kernelSize/2) ][i] = tempRAWimage[j][outputImageHeight-1];

 	//FILE *tempoutputRAWfile = fopen("../src/TEMPclock1.raw", "w+");
 	//if (!(tempoutputRAWfile))
	//{
	//	printf("Cannot open file!!");
	// 	exit(1);
	//}
 	//fwrite(filterBuffer, sizeof(char), (outputImageHeight + kernelSize - 1)*(outputImageWidth + kernelSize - 1)*numberOfComponents, tempoutputRAWfile);
 	//fclose(tempoutputRAWfile);
	
    double tempKernelbuffer[kernelSize][kernelSize];
    unsigned char tempRAWimage2[outputImageHeight][outputImageWidth];

    for(int q=0; q<outputImageHeight; q++)
    {
    	for(int p=0; p<outputImageWidth; p++)
    	{
            double runningSum = 0;
            for(int j=0; j<kernelSize; j++)
            {
                for(int i=0; i<kernelSize; i++)
                {
                    tempKernelbuffer[j][i] = (double)filterBuffer[ j+q ][ i+p ]* gaussKernel[j][i];
                    runningSum += tempKernelbuffer[j][i];
                }
            }
            tempRAWimage[q][p]= (unsigned char)runningSum;
    	}
    	
    }

    FILE *outputfile = fopen(outputGaussRAWfile, "wb");
    if (outputfile == NULL)
	{
		printf("\nCannot open file to write!!\n");
		exit(4);
	}
    fwrite(tempRAWimage, sizeof(char), (outputImageHeight)*(outputImageWidth), outputfile);
    fclose(outputfile);


    //////named pipe: send output image to C
    if(mkfifo("myfifo4", 0777) == -1){
        if(errno !=EEXIST){
            printf("Could not create fifo4 file");
            return 1;
        }
	}

    int writefd = open("myfifo4", O_WRONLY);
    printf("Opened fifo4 file\n");
    if (write(writefd, outputGaussRAWfile, strlen(outputGaussRAWfile)+1) == -1){
        printf("\n cannot be write");
        return 6;
    }

    //printf("Written fifo4 file\n");
    close(writefd);
    printf("Closed fifo4 file\n");


    return 0;
}


int main(int argc, char *argv[])
{
    ////////named pipe: read image address
    int readfd;
    char readbuf[100];
    int read_bytes;  

    readfd = open("myfifo1", O_RDONLY);
    read_bytes = read(readfd, readbuf, sizeof(readbuf));
    readbuf[read_bytes] = '\0';
    printf("B Received string: \"%s\" and length is %d\n", readbuf, (int)strlen(readbuf));

    close(readfd);


    // Remember filenames
    char *outfile = "output.bmp";

    
    FILE* inptr = fopen(readbuf, "rb");//Read the image.bmp file in the same directory.
	if(inptr == NULL)
	{
		printf("Failed to open'%s'!\n", readbuf);
		return -1;
	}

    FILE *outptr = fopen(outfile, "wb+");
    if (outptr == NULL)
	{
		printf("Cannot open file!!");
		return 2;
	}

    // Read infile's BITMAPFILEHEADER & BITMAPINFOHEADER
    BITMAPFILEHEADER FileHead;    // Global Variable File Head  
    BITMAPINFOHEADER Infohead;    // Global Variable Information Head 

    fread(&FileHead, sizeof(FileHead), 1, inptr);
    fread(&Infohead, sizeof(Infohead), 1, inptr);

    fwrite(&FileHead, sizeof(FileHead), 1, outptr);
    fwrite(&Infohead, sizeof(Infohead), 1, outptr);

    int height = abs(Infohead.biHeight);
    int width = Infohead.biWidth;

    //////////////////////////////////////////gaussian////////////////////////////////
    // Filter image
	double gaussKernel[kernelSize][kernelSize];
    createGaussianFilter(gaussKernel);
    printf("\nB %dx%d Gaussian kernel with sigma = %0.1f:\n", kernelSize, kernelSize, sigma);
    for(int y = 0; y < kernelSize; y++)
    {
        for (int x = 0; x < kernelSize; x++)
        {
            printf("%0.003f	", gaussKernel[y][x]);
        }
        printf("\n");
    }
    ApplyingGaussianFilter(gaussKernel, height, width, readbuf, outfile);
    /////////////////////////////////////////////////////////////////////////////

    unlink("myfifo1");
    // Close infile
    fclose(inptr);

    // Close outfile
    fclose(outptr);

    return 0;
}