#include "bmp_io.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>



ClImage* clLoadImage(char* path)  
{  
	ClImage* bmpImg;  
	FILE* pFile;  
	unsigned short fileType;  
	ClBitMapFileHeader bmpFileHeader;  
	ClBitMapInfoHeader bmpInfoHeader;  
	int channels = 1;  
	int width = 0;  
	int height = 0;  
	int step = 0;  
	int offset = 0;  
	unsigned char pixVal;  
	ClRgbQuad* quad;  
	int i, j, k;  

	bmpImg = (ClImage*)malloc(sizeof(ClImage));  
	pFile = fopen(path, "rb");  
	if (!pFile)  
	{  
		free(bmpImg);  
		return NULL;  
	}  

	fread(&fileType, sizeof(unsigned short), 1, pFile);  
	if (fileType == 0x4D42)  
	{  
		//printf("bmp file! \n");  

		fread(&bmpFileHeader, sizeof(ClBitMapFileHeader), 1, pFile);  
		/*
		printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ \n"); 
		printf("bmp文件头信息：\n"); 
		printf("文件大小：%d \n", bmpFileHeader.bfSize); 
		printf("保留字：%d \n", bmpFileHeader.bfReserved1); 
		printf("保留字：%d \n", bmpFileHeader.bfReserved2); 
		printf("位图数据偏移字节数：%d \n", bmpFileHeader.bfOffBits);
		*/
		fread(&bmpInfoHeader, sizeof(ClBitMapInfoHeader), 1, pFile); 
		/*
		printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ \n"); 
		printf("bmp文件信息头\n"); 
		printf("结构体长度：%d \n", bmpInfoHeader.biSize); 
		printf("位图宽度：%d \n", bmpInfoHeader.biWidth); 
		printf("位图高度：%d \n", bmpInfoHeader.biHeight); 
		printf("位图平面数：%d \n", bmpInfoHeader.biPlanes); 
		printf("颜色位数：%d \n", bmpInfoHeader.biBitCount); 
		printf("压缩方式：%d \n", bmpInfoHeader.biCompression); 
		printf("实际位图数据占用的字节数：%d \n", bmpInfoHeader.biSizeImage); 
		printf("X方向分辨率：%d \n", bmpInfoHeader.biXPelsPerMeter); 
		printf("Y方向分辨率：%d \n", bmpInfoHeader.biYPelsPerMeter); 
		printf("使用的颜色数：%d \n", bmpInfoHeader.biClrUsed); 
		printf("重要颜色数：%d \n", bmpInfoHeader.biClrImportant); 
		printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ \n"); 
		*/
		if (bmpInfoHeader.biBitCount == 8)  
		{  
			//printf("该文件有调色板，即该位图为非真彩色\n\n");  
			channels = 1;  
			width = bmpInfoHeader.biWidth;  
			height = bmpInfoHeader.biHeight;  
			offset = (channels*width)%4;  
			if (offset != 0)  
			{  
				offset = 4 - offset;  
			}  
			//bmpImg->mat = kzCreateMat(height, width, 1, 0);  
			bmpImg->width = width;  
			bmpImg->height = height;  
			bmpImg->channels = 1;  
			bmpImg->imageData = (unsigned char*)malloc(sizeof(unsigned char)*width*height);  
			step = channels*width;  

			quad = (ClRgbQuad*)malloc(sizeof(ClRgbQuad)*256);  
			fread(quad, sizeof(ClRgbQuad), 256, pFile);
			free(quad);  

			for (i=0; i<height; i++)
			{
				for (j=0; j<width; j++)  
				{  
					fread(&pixVal, sizeof(unsigned char), 1, pFile);  
					bmpImg->imageData[(height-1-i)*step+j] = pixVal;  
				}  
				if (offset != 0)  
				{  
					for (j=0; j<offset; j++)  
					{  
						fread(&pixVal, sizeof(unsigned char), 1, pFile);  
					}  
				}  
			}             
		}  
		else if (bmpInfoHeader.biBitCount == 24)  
		{  
			//printf("该位图为位真彩色\n\n");  
			channels = 3;  
			width = bmpInfoHeader.biWidth;  
			height = bmpInfoHeader.biHeight;  

			bmpImg->width = width;  
			bmpImg->height = height;  
			bmpImg->channels = 3;  
			bmpImg->imageData = (unsigned char*)malloc(sizeof(unsigned char)*width*3*height);  
			step = channels*width;  

			offset = (channels*width)%4;  
			if (offset != 0)  
			{  
				offset = 4 - offset;  
			}  

			for (i=0; i<height; i++)  
			{  
				for (j=0; j<width; j++)  
				{    
					fread(&pixVal, sizeof(unsigned char), 1, pFile);  
					bmpImg->imageData[(height-1-i)*step+j*3] = pixVal; 
					fread(&pixVal, sizeof(unsigned char), 1, pFile);  
					bmpImg->imageData[(height-1-i)*step+j*3+1] = pixVal;
					fread(&pixVal, sizeof(unsigned char), 1, pFile);  
					bmpImg->imageData[(height-1-i)*step+j*3+2] = pixVal;
					//kzSetMat(bmpImg->mat, height-1-i, j, kzScalar(pixVal[0], pixVal[1], pixVal[2]));  
				}  
				if (offset != 0)  
				{  
					for (j=0; j<offset; j++)  
					{  
						fread(&pixVal, sizeof(unsigned char), 1, pFile);  
					}  
				}  
			}  
		}  
		else // 若图像不是8位图或者24位图，缺乏处理程序，直接放弃。。。
		{
			printf("the image is no 8-bit or 24-bit, no method has been built");
			bmpImg = NULL;
		}
	}  

	return bmpImg;  
}  

int clSaveImageUYVY(char* path, char *img, int width, int height, int sty)
{
    unsigned char u, v, y1, y2;
    ClImage *bmpImg;
    bmpImg = (ClImage*)malloc(sizeof(ClImage));
    if(bmpImg == NULL)
    {
        printf ( "error for malloc in bmpimg\n" );
        exit(1);
    }
    bmpImg->imageData = (unsigned char *)malloc(sizeof(unsigned char)*width*height*sty);
    if(bmpImg->imageData == NULL)
    {
        printf ( "error for malloc in bmpimg->imageData\n" );
        exit(1);
    }

    bmpImg->width = width;
    bmpImg->height = height;
    bmpImg->channels = sty;

    if (sty == 1)
    {
        printf ( "save to 1 channel image in UYVY\n\n" );
        // this is for gray image saving
        int i;
        for(i=0; i<width*height; i++)
        {
            *(bmpImg->imageData + i) = *(img + i*2 + 1);
        }
        clSaveImage(path, bmpImg);
    } else if(sty == 3)
    {
        printf ( "save to 3 channel image in UYVY\n\n" );
        // this for rgb color image saving
        int i;
        for(i = 0; i < width*height; i+=2)
        {
            y1 = img[2*i+1];
            y2 = img[2*i+3];
            u = img[2*i];
            v = img[2*i+2];

            *(bmpImg->imageData + i*3) = (unsigned char)(y1 + (u - 128) + ((104*(u - 128))>>8));  
            *(bmpImg->imageData + i*3+1) = (unsigned char)(y1 - (89*(v - 128)>>8) - ((183*(u - 128))>>8));  
            *(bmpImg->imageData + i*3+2) = (unsigned char)(y1 + (v - 128) + ((199*(v - 128))>>8));  

            *(bmpImg->imageData + i*3+3) = (unsigned char)(y2 + (u - 128) + ((104*(u - 128))>>8));  
            *(bmpImg->imageData + i*3+4) = (unsigned char)(y2 - (89*(v - 128)>>8) - ((183*(u - 128))>>8));  
            *(bmpImg->imageData + i*3+5) = (unsigned char)(y2 + (v - 128) + ((199*(v - 128))>>8));  
        }
        clSaveImage(path, bmpImg);
    }
    return 1;
}

int clSaveImage(char* path, ClImage * bmpImg)  
{  
	FILE *pFile;  
	unsigned short fileType;  
	ClBitMapFileHeader bmpFileHeader;  
	ClBitMapInfoHeader bmpInfoHeader;  
	int step;  
	int offset;  
	unsigned char pixVal = '\0';  
	int i, j;  
	ClRgbQuad* quad;  

	pFile = fopen(path, "wb");  
	if (!pFile)  
	{  
		return 0;  
	}  

	fileType = 0x4D42;  
	fwrite(&fileType, sizeof(unsigned short), 1, pFile);  

	if (bmpImg->channels == 3)//24位，通道，彩图  
	{  
        printf ( "save to 3 channel image\n\n" );
		step = bmpImg->channels*bmpImg->width;  
		offset = step%4;  
		if (offset != 4)  
		{  
			step += 4-offset;  
		}  

		bmpFileHeader.bfSize = bmpImg->height*step + 54;  
		bmpFileHeader.bfReserved1 = 0;  
		bmpFileHeader.bfReserved2 = 0;  
		bmpFileHeader.bfOffBits = 54;  
		fwrite(&bmpFileHeader, sizeof(ClBitMapFileHeader), 1, pFile);  

		bmpInfoHeader.biSize = 40;  
		bmpInfoHeader.biWidth = bmpImg->width;  
		bmpInfoHeader.biHeight = bmpImg->height;  
		bmpInfoHeader.biPlanes = 1;  
		bmpInfoHeader.biBitCount = 24;  
		bmpInfoHeader.biCompression = 0;  
		bmpInfoHeader.biSizeImage = bmpImg->height*step;  
		bmpInfoHeader.biXPelsPerMeter = 0;  
		bmpInfoHeader.biYPelsPerMeter = 0;  
		bmpInfoHeader.biClrUsed = 0;  
		bmpInfoHeader.biClrImportant = 0;  
		fwrite(&bmpInfoHeader, sizeof(ClBitMapInfoHeader), 1, pFile);  

		for (i=bmpImg->height-1; i>-1; i--)  
		{  
			for (j=0; j<bmpImg->width; j++)  
			{  
				pixVal = bmpImg->imageData[i*bmpImg->width*3+j*3];  
				fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
				pixVal = bmpImg->imageData[i*bmpImg->width*3+j*3+1];  
				fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
				pixVal = bmpImg->imageData[i*bmpImg->width*3+j*3+2];  
				fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
			}  
			if (offset!=0)  
			{  
				for (j=0; j<offset; j++)  
				{  
					pixVal = 0;  
					fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
				}  
			}  
		}  
	}  
	else if (bmpImg->channels == 1)//8位，单通道，灰度图  
	{  
        printf ( "save to 1 channel image\n\n" );
		step = bmpImg->width;  
		offset = step%4;  
		if (offset != 4)  
		{  
			step += 4-offset;  
		}  

		bmpFileHeader.bfSize = 54 + 256*4 + bmpImg->width;  
		bmpFileHeader.bfReserved1 = 0;  
		bmpFileHeader.bfReserved2 = 0;  
		bmpFileHeader.bfOffBits = 54 + 256*4;  
		fwrite(&bmpFileHeader, sizeof(ClBitMapFileHeader), 1, pFile);  

		bmpInfoHeader.biSize = 40;  
		bmpInfoHeader.biWidth = bmpImg->width;  
		bmpInfoHeader.biHeight = bmpImg->height;  
		bmpInfoHeader.biPlanes = 1;  
		bmpInfoHeader.biBitCount = 8;  
		bmpInfoHeader.biCompression = 0;  
		bmpInfoHeader.biSizeImage = bmpImg->height*step;  
		bmpInfoHeader.biXPelsPerMeter = 0;  
		bmpInfoHeader.biYPelsPerMeter = 0;  
		bmpInfoHeader.biClrUsed = 256;  
		bmpInfoHeader.biClrImportant = 256;  
		fwrite(&bmpInfoHeader, sizeof(ClBitMapInfoHeader), 1, pFile);  

		quad = (ClRgbQuad*)malloc(sizeof(ClRgbQuad)*256);  
		for (i=0; i<256; i++)  
		{  
			quad[i].rgbBlue = i;  
			quad[i].rgbGreen = i;  
			quad[i].rgbRed = i;  
			quad[i].rgbReserved = 0;  
		}  
		fwrite(quad, sizeof(ClRgbQuad), 256, pFile);  
		free(quad);  

		for (i=bmpImg->height-1; i>-1; i--)  
		{  
			for (j=0; j<bmpImg->width; j++)  
			{  
				pixVal = bmpImg->imageData[i*bmpImg->width+j];  
				fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
			}  
			if (offset!=0)  
			{  
				for (j=0; j<offset; j++)  
				{  
					pixVal = 0;  
					fwrite(&pixVal, sizeof(unsigned char), 1, pFile);  
				}  
			}  
		}  
	}  
	fclose(pFile);  

	return 1;  
}  
