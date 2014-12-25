typedef struct 
{  
    //unsigned short    bfType;  
    unsigned long    bfSize;  
    unsigned short    bfReserved1;  
    unsigned short    bfReserved2;  
    unsigned long    bfOffBits;  
}ClBitMapFileHeader;  
  
typedef struct
{  
    unsigned long  biSize;   
    long   biWidth;   
    long   biHeight;   
    unsigned short   biPlanes;   
    unsigned short   biBitCount;  
    unsigned long  biCompression;   
    unsigned long  biSizeImage;   
    long   biXPelsPerMeter;   
    long   biYPelsPerMeter;   
    unsigned long   biClrUsed;   
    unsigned long   biClrImportant;   
} ClBitMapInfoHeader;  
  
typedef struct
{  
    unsigned char rgbBlue; //����ɫ����ɫ����  
    unsigned char rgbGreen; //����ɫ����ɫ����  
    unsigned char rgbRed; //����ɫ�ĺ�ɫ����  
    unsigned char rgbReserved; //����ֵ  
} ClRgbQuad;  
  
typedef struct
{  
    int width;  
    int height;  
    int channels;  
    unsigned char* imageData;  
}ClImage;  

ClImage * clLoadImage(char * path);
int clSaveImage(char* path, ClImage * bmpImg);

