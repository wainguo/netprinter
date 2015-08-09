//ReadBitMap
//
#include <string.h> 
#include <math.h>   
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define SERVERCONF "server.cfg"
#define DFTSERVER "10.1.72.251"
#define   WIDTHBYTES(bits) (((bits)+31)/32*4)

//位图文件头信息结构定义
//其中不包含文件类型信息（由于结构体的内存结构决定，要是加了的话将不能正确读取文件信息）

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {
unsigned int bfSize;           //文件大小
unsigned short   bfReserved1; 	//保留字，不考虑
unsigned short   bfReserved2; 	//保留字，同上
unsigned int bfOffBits;        //实际位图数据的偏移字节数，即前三个部分长度之和
} BITMAPFILEHEADER; 


//信息头BITMAPINFOHEADER，也是一个结构，其定义如下：

typedef struct tagBITMAPINFOHEADER{
//public:
unsigned int   biSize;         	//指定此结构体的长度，为40
int    biWidth;       		//位图宽
int    biHeight;       	//位图高
unsigned short    biPlanes;       	//平面数，为1
unsigned short    biBitCount;     	//采用颜色位数，可以是1，2，4，8，16，24，新的可以是32
unsigned int   biCompression;  	//压缩方式，可以是0，1，2，其中0表示不压缩
unsigned int   biSizeImage;    	//实际位图数据占用的字节数
int    biXPelsPerMeter;	//X方向分辨率
int    biYPelsPerMeter;	//Y方向分辨率
unsigned int   biClrUsed;      	//使用的颜色数，如果为0，则表示默认值(2^颜色位数)
unsigned int   biClrImportant; 	//重要颜色数，如果为0，则表示所有颜色都是重要的
} BITMAPINFOHEADER; 


//调色板Palette，当然，这里是对那些需要调色板的位图文件而言的。24位和32位是不需要调色板的。
//（似乎是调色板结构体个数等于使用的颜色数。）

typedef struct tagRGBQUAD { 
//public:
unsigned char     rgbBlue; //该颜色的蓝色分量
unsigned char     rgbGreen; //该颜色的绿色分量
unsigned char     rgbRed; //该颜色的红色分量
unsigned char     rgbReserved; //保留值
} RGBQUAD;


/* init printer & hanzi mode*/
char strInitPrinter[] = "\x1B\x40\x1C\x26";

/* Feed 4 lines */
char strFeed4Lines[] = "\x0A\x0A\x0A\x0A";

/* print guangshan bmp */
char strPrintNewBmp[] = "\x1D\x76\x30\x30\x00\x00\x00\x00";

/* alignment */
char strSetAlignment[] = "\x1B\x61\x31";

/* print bmp */
char strPrintBmp[] = "\x1B\x2A\x01\x00\x00";

int sockfd;

int gw = 0;

void showBmpHead(BITMAPFILEHEADER* pBmpHead)
{
printf("位图文件头:\n");
printf("文件大小:%d\n",pBmpHead->bfSize);
printf("保留字:%d\n",pBmpHead->bfReserved1);
printf("保留字:%d\n",pBmpHead->bfReserved2);
printf("实际位图数据的偏移字节数:%d\n",pBmpHead->bfOffBits);

}


void showBmpInforHead(tagBITMAPINFOHEADER* pBmpInforHead)
{
printf("位图信息头:\n");
printf("结构体的长度:%d\n",pBmpInforHead->biSize);
printf("位图宽:%d\n",pBmpInforHead->biWidth);
printf("位图高:%d\n",pBmpInforHead->biHeight);
printf("biPlanes平面数:%d\n",pBmpInforHead->biPlanes);
printf("biBitCount采用颜色位数:%d\n",pBmpInforHead->biBitCount);
printf("压缩方式:%d\n",pBmpInforHead->biCompression);
printf("biSizeImage实际位图数据占用的字节数:%d\n",pBmpInforHead->biSizeImage);
printf("X方向分辨率:%d\n",pBmpInforHead->biXPelsPerMeter);
printf("Y方向分辨率:%d\n",pBmpInforHead->biYPelsPerMeter);
printf("使用的颜色数:%d\n",pBmpInforHead->biClrUsed);
printf("重要颜色数:%d\n",pBmpInforHead->biClrImportant);
}

void showRgbQuan(tagRGBQUAD* pRGB)
{ 
printf("(%-3d,%-3d,%-3d)   ",pRGB->rgbRed,pRGB->rgbGreen,pRGB->rgbBlue);

}


int initPrinter(){
        /* Init printer*/
        write(sockfd, strInitPrinter, 4);
}

void feed4Lines(){
        write(sockfd, strFeed4Lines, 4);
}

void print_bmp(int w, int h, unsigned char *data, int n){
	printf("print bmp:w=%d,h=%d, w*h*8=%d\n", w, h, w*h);

        write(sockfd, strSetAlignment, 3); //align center

	strPrintNewBmp[4]=w%256; //xL
	strPrintNewBmp[5]=w/256; //xH
	strPrintNewBmp[6]=h%256; //yL
	strPrintNewBmp[7]=h/256; //yH
        write(sockfd, strPrintNewBmp, 8);

        int index = 0;
       	printf("%3d|", 0);
	for(int i=0; i<w*8; i++){
		printf("%d", i%8+1);
	}
	printf("\n");
	for(int j=0; j<h; j++){
       		printf("%3d|", j);
		for(int i=0; i<w; i++){
			index = j*w + i;
			unsigned char r = 0;
        		//write(sockfd, &data[index], 1);

			for(int k=0; k<8; k++){
				int s = i*8 + k;
				if( s < gw ){
					r = r | (data[index] & (128>>k));
				}
				
				//if(data[index] & (1<<k)){
				if(data[index] & (128>>k)){
					printf("%d", k+1);	
				}else{
					printf(" ");
				}
			}
        		write(sockfd, &r, 1);
		}
		printf("\n");
	}
	
	feed4Lines();
}

void print_bmp2(int x, int y, unsigned char *data, int n){

	printf("print bmp:x=%d,y=%d\n", x, y);

	strPrintNewBmp[2]=48; //0
	strPrintNewBmp[3]=48; //m
	strPrintNewBmp[4]=x%256; //xL
	strPrintNewBmp[5]=x/256; //xH
	strPrintNewBmp[6]=y%256; //yL
	strPrintNewBmp[7]=y/256; //yH

	printf("print bmp:xL=%d,xH=%d, yL=%d,yH=%d\n", strPrintNewBmp[4],strPrintNewBmp[5],strPrintNewBmp[6],strPrintNewBmp[7] );
	printf("print bmp datas:n=%d\n", n);
        write(sockfd, strPrintNewBmp, 8);
        write(sockfd, data, n);

	feed4Lines();


}


void reverse(unsigned char *data, int n){
	int i;
	for(i=0; i<=n-i-1; i++){
		unsigned char c = *(data+i);
		*(data+i)=*(data+n-i-1);
		*(data+n-i-1) = c;
	}
}

int readPrint( char *strFile )
{

BITMAPFILEHEADER   bitHead;
BITMAPINFOHEADER bitInfoHead; 
FILE* pfile;

pfile = fopen(strFile,"rb");//打开文件

if(pfile!=NULL)
{
   printf("bmp file open success.\n");
   //读取位图文件头信息
   unsigned short fileType;
   fread(&fileType,1,sizeof(unsigned short),pfile);
   if(fileType != 0x4d42)
   {
    printf("sz of short:%d, fileType:%x", sizeof(unsigned short),fileType );
    printf("file is not .bmp file!");
    return 0;
   }
   fread(&bitHead,1,sizeof(tagBITMAPFILEHEADER),pfile);
  
   showBmpHead(&bitHead);
   printf("\n\n");

   //读取位图信息头信息
   fread(&bitInfoHead,1,sizeof(BITMAPINFOHEADER),pfile);
   showBmpInforHead(&bitInfoHead);
   printf("\n");
}
else
{
   printf("file open fail!\n");
   return 0;
}


tagRGBQUAD *pRgb ;

if(bitInfoHead.biBitCount < 24)//有调色板
{ 
   //读取调色盘结信息
   int nPlantNum = int(pow(2,double(bitInfoHead.biBitCount)));    //   Mix color Plant Number;
   pRgb=(tagRGBQUAD *)malloc(nPlantNum*sizeof(tagRGBQUAD)); 
   memset(pRgb,0,nPlantNum*sizeof(tagRGBQUAD));
   int num = fread(pRgb,4,nPlantNum,pfile);
  
   printf("Color Plate Number: %d\n",nPlantNum);

   printf("颜色板信息:\n");
   for (int i =0; i<nPlantNum;i++)
   {
    if (i%5==0)
    {
     printf("\n");
    }
    showRgbQuan(&pRgb[i]);
   
   }

   printf("\n");
  
}


int width = abs(bitInfoHead.biWidth);
gw = width;
int height = abs(bitInfoHead.biHeight);
//分配内存空间把源图存入内存   
int l_width   = WIDTHBYTES(width* bitInfoHead.biBitCount);//计算位图的实际宽度并确保它为32的倍数
unsigned char    *pColorData=(unsigned char *)malloc(height*l_width);   
memset(pColorData,0,height*l_width);   
int nData = height*l_width;

//把位图数据信息读到数组里   
fread(pColorData,1,nData,pfile);   

//将位图数据转化为RGB数据
tagRGBQUAD* dataOfBmp;
dataOfBmp = (tagRGBQUAD *)malloc(width*height*sizeof(tagRGBQUAD));//用于保存各像素对应的RGB数据
memset(dataOfBmp,0,width*height*sizeof(tagRGBQUAD));

//data to print
unsigned char *data; 
int dw, dh;

if(bitInfoHead.biBitCount<24)//有调色板，即位图为非真彩色 
{
   int k, d=0;
   int index = 0;
   if (bitInfoHead.biBitCount == 1)
   {
    dw = (width+7)/8;
    dh = height;
    
    unsigned char *data = (unsigned char *)malloc(dw*dh);
    memset(data, 0, dw*dh);

    for(int i=0;i<height;i++){
     for(int j=0;j<width;j++)
     {
      unsigned char mixIndex= 0;
      k = i*l_width + j/8;//k:取得该像素颜色数据在实际数据数组中的序号

      d = i*dw + j/8 ; //d:取得该像素在打印数组中所在字节的序号

      //j:提取当前像素的颜色的具体值    
      mixIndex = pColorData[k];
      switch(j%8)
      {
      case 0:
       mixIndex = mixIndex<<7;
       mixIndex = mixIndex>>7;
       break;
      case 1:
       mixIndex = mixIndex<<6;
       mixIndex = mixIndex>>7;
       break;
      case 2:
       mixIndex = mixIndex<<5;
       mixIndex = mixIndex>>7;
       break;

      case 3:
       mixIndex = mixIndex<<4;
       mixIndex = mixIndex>>7;
       break;
      case 4:
       mixIndex = mixIndex<<3;
       mixIndex = mixIndex>>7;
       break;

      case 5:
       mixIndex = mixIndex<<2;
       mixIndex = mixIndex>>7;
       break;
      case 6:
       mixIndex = mixIndex<<1;
       mixIndex = mixIndex>>7;
       break;

      case 7:
       mixIndex = mixIndex>>7;
       break;
      }

      //将像素数据保存到数组中对应的位置
      dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
      dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
      dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
      dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;

      //guoshengxing added
      if(pRgb[mixIndex].rgbRed == 0 ){
      	//data[d] |= 128>>(j%8);
      	data[d] |= 1<<(j%8);
      }
      //end

      index++;

      }
      printf("now i=%d, k=%d, d=%d\n", i,k,d);
      reverse(data+i*dw, dw);
   }
      reverse(data, dw*dh);
      print_bmp(dw, dh, data, dw*dh);
   }

   if(bitInfoHead.biBitCount==2)
   {
//guoshengxing add
    dw = (width+7)/8;
    dh = height;
    unsigned char *data = (unsigned char *)malloc(dw*dh);
    memset(data, 0, dw*dh);
//end

    for(int i=0;i<height;i++){
     for(int j=0;j<width;j++)
     {
      unsigned char mixIndex= 0;
      k = i*l_width + j/4;//k:取得该像素颜色数据在实际数据数组中的序号

      d = i*dw + j/4; //d:取得该像素在打印数组中所在字节的序号

      //j:提取当前像素的颜色的具体值    
      mixIndex = pColorData[k];
      switch(j%4)
      {
      case 0:
       mixIndex = mixIndex<<6;
       mixIndex = mixIndex>>6;
       break;
      case 1:
       mixIndex = mixIndex<<4;
       mixIndex = mixIndex>>6;
       break;
      case 2:
       mixIndex = mixIndex<<2;
       mixIndex = mixIndex>>6;
       break;
      case 3:
       mixIndex = mixIndex>>6;
       break;
      }

      //将像素数据保存到数组中对应的位置
      dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
      dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
      dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
      dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
      index++;

      //guoshengxing added
      if(pRgb[mixIndex].rgbRed == 0){
      	data[d] |= 128>>(j%8);
      }
      //end

     }
      printf("now i=%d, k=%d, d=%d\n", i,k,d);
      reverse(data+i*dw, dw);
    }
      reverse(data, dw*dh);
      print_bmp(dw, dh, data, dw*dh);
   }
   if(bitInfoHead.biBitCount == 4)
   {
//guoshengxing add
    dw = (width+7)/8;
    dh = height;
    unsigned char *data = (unsigned char *)malloc(dw*dh);
    memset(data, 0, dw*dh);
//end

    for(int i=0;i<height;i++){
     for(int j=0;j<width;j++)
     {
      unsigned char mixIndex= 0;
      k = i*l_width + j/2;

      d = i*dw + j/8; //d:取得该像素在打印数组中所在字节的序号

      mixIndex = pColorData[k];
      if(j%2==0)
      {//低      
       mixIndex = mixIndex<<4;
       mixIndex = mixIndex>>4;
      }
      else
      {//高
       mixIndex = mixIndex>>4;
      }

      dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
      dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
      dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
      dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
      index++;

      //guoshengxing added
      if(pRgb[mixIndex].rgbRed <= 5){
      	data[d] |= 128>>(j%8);
      }
      //end
     }
      printf("now i=%d, k=%d, d=%d\n", i,k,d);
      reverse(data+i*dw, dw);
    }
      reverse(data, dw*dh);
      print_bmp(dw, dh, data, dw*dh);

   }
   if(bitInfoHead.biBitCount == 8)
   {
//guoshengxing add
    dw = (width+7)/8;
    dh = height;
    unsigned char *data = (unsigned char *)malloc(dw*dh);
    memset(data, 0, dw*dh);
//end

    for(int i=0;i<height;i++){
     for(int j=0;j<width;j++)
     {
      unsigned char mixIndex= 0;

      k = i*l_width + j;

      d = i*dw + j/8; //d:取得该像素在打印数组中所在字节的序号

      mixIndex = pColorData[k];

      dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
      dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
      dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
      dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
      index++;
     
     
      //guoshengxing added
      if(pRgb[mixIndex].rgbRed == 0){
      	data[d] |= 128>>(j%8);
      }
      //end

     }
      printf("now i=%d, k=%d, d=%d\n", i,k,d);
      reverse(data+i*dw, dw);
    }
      reverse(data, dw*dh);
      print_bmp(dw, dh, data, dw*dh);
   }
   if(bitInfoHead.biBitCount == 16)
   {
//guoshengxing add
    dw = (width+7)/8;
    dh = height;
    unsigned char *data = (unsigned char *)malloc(dw*dh);
    memset(data, 0, dw*dh);
//end

    for(int i=0;i<height;i++){
     for(int j=0;j<width;j++)
     {
      unsigned short mixIndex= 0;

      k = i*l_width + j*2;

      d = i*dw + j/8; //d:取得该像素在打印数组中所在字节的序号

      unsigned short shortTemp;
      shortTemp = pColorData[k+1];
      shortTemp = shortTemp<<8;
    
      mixIndex = pColorData[k] + shortTemp;

      dataOfBmp[index].rgbRed = pRgb[mixIndex].rgbRed;
      dataOfBmp[index].rgbGreen = pRgb[mixIndex].rgbGreen;
      dataOfBmp[index].rgbBlue = pRgb[mixIndex].rgbBlue;
      dataOfBmp[index].rgbReserved = pRgb[mixIndex].rgbReserved;
      index++;

      //guoshengxing added
      if(pRgb[mixIndex].rgbRed == 0){
      	data[d] |= 128>>(j%8);
      }
      //end
     }
      printf("now i=%d, k=%d, d=%d\n", i,k,d);
      reverse(data+i*dw, dw);
    }
      reverse(data, dw*dh);
      print_bmp(dw, dh, data, dw*dh);
   }
}
else//位图为24位真彩色
{
//guoshengxing add
    dw = (width+7)/8;
    dh = height;
    printf("dw=%d, dh=%d\n", dw, dh);
    unsigned char *data = (unsigned char *)malloc(dw*dh);
    memset(data, 0, dw*dh);
//end

   int k;
   int index = 0;
   for(int i=0;i<height;i++)
    for(int j=0;j<width;j++)
    {
     k = i*l_width + j*3;

     //int d = i*dw + j/8; //d:取得该像素在打印数组中所在字节的序号
     int d = i*dw + j/8; //d:取得该像素在打印数组中所在字节的序号

     dataOfBmp[index].rgbRed = pColorData[k+2];   
     dataOfBmp[index].rgbGreen = pColorData[k+1];   
     dataOfBmp[index].rgbBlue = pColorData[k];    
     index++;

      //guoshengxing added
      //if(pRgb[index].rgbRed == 0){
      if(pRgb[index].rgbRed <= 50){
      	data[d] |= 128>>(j%8);
      }
      //end
    }            
      print_bmp(dh, dw, data, dw*dh);
}

printf("像素数据信息:\n");
for (int i=0; i<width*height; i++)
{
   if (i%5==0)
   {
        printf("\n");
   }
   showRgbQuan(&dataOfBmp[i]);
}

fclose(pfile); 
if (bitInfoHead.biBitCount<24)
{
   free(pRgb);
}
free(dataOfBmp);
free(pColorData);
printf("\n");

}

int getServerAddr( char *addr ){
	strcpy(addr, DFTSERVER);
	char path[1024];
	if (NULL == getcwd(path, 1024)){
		return -1;
	}
	strcat(path, "/");
	strcat(path, SERVERCONF);
	FILE *f = fopen(path, "r");
	if( f == NULL ){
		return -1;
	}

	char buf[32];
	char c;
	int i;
	while( (c=fgetc(f)) == ' ' );

	for(i=0; i<32; i++){
		if(c=='\n' || c == '\0' || c==' ' || c == EOF){
			buf[i]='\0';
			break;
		}
		buf[i] = c;
		c = fgetc(f);
	}

	buf[31] = '\0';
	if(strlen(buf) >= 7)
		strcpy(addr, buf);
	fclose(f);
}

int main(int argc, char **argv){
	if(argc < 2 ){
		printf("Usage: %s <file>\n", argv[0]);
		printf("     : <file> the bmp file path to print. \n" );
		return -1;
	}
	char *file = argv[1];

	char server[32];
	struct sockaddr_in servaddr;

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ){
		printf("socket error.\n");
		return 0;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9100);
	getServerAddr( server );
	if( inet_pton(AF_INET, server, &servaddr.sin_addr) <= 0 ){
	//if( inet_pton(AF_INET, "10.1.72.251", &servaddr.sin_addr) <= 0 ){
		printf("inet_pton error.\n");
		return 0;
	}
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
		printf("connect error.\n");
		return 0;
	}

//bmp_test();
	readPrint( file );
}

