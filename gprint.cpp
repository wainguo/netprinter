#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iconv.h>

#define TIME_OUT_TIME 5
#define ERROR 9
#define SUCCESS 0

#define SERVERCONF "server.cfg"
#define DFTSERVER "10.1.72.251" 

/* init printer & hanzi mode*/
char strInitPrinter[] = "\x1B\x40\x1C\x26";

/* Set Font size */
char strSetFontSize[] = "\x1D\x21\x11";

/* Alignment */
char strSetAlignment[] = "\x1B\x61\x30";

/* Feed 4 lines */
char strFeed4Lines[] = "\x0A\x0A\x0A\x0A";

/* Cash Drawer open */
// ESC p m t1 t2
char strCashDrawer[] = "\x1B\x70\x00\x30\xC0";

/* Feed and cut paper */
char strCutPaper[] = "\x1D\x56\x42\x00";

int sockfd;
char linebuffer[1024];
int len = 0;

char outbuffer[4096];
int outlen = 0;

int initPrinter(){
	/* Init printer*/
	write(sockfd, strInitPrinter, 4);
}

int setFontNormal(){
	/* 0.default widht and height */
	strSetFontSize[2] = '\x00';
	write(sockfd, strSetFontSize, 3);
}

int setFontBold(){
	/* 1.Double widht and height */
	strSetFontSize[2] = '\x11';
	write(sockfd, strSetFontSize, 3);
}

int setFontDoubleHeight(){
	/* 2.Double height */
	strSetFontSize[2] = '\x01';
	write(sockfd, strSetFontSize, 3);
}

int setFontDoubleWidth(){
	/* 3.Double width */
	strSetFontSize[2] = '\x10';
	write(sockfd, strSetFontSize, 3);
}

int setAlignLeft(){
	strSetAlignment[2] = '\x30';
	write(sockfd, strSetAlignment, 3);
}

int setAlignCenter(){
	strSetAlignment[2] = '\x31';
	write(sockfd, strSetAlignment, 3);
}

int setAlignRight(){
	strSetAlignment[2] = '\x32';
	write(sockfd, strSetAlignment, 3);
}

void feed4Lines(){
	write(sockfd, strFeed4Lines, 4);
}

void openCashDrawer(){
	write(sockfd, strCashDrawer, 5);
}

void cutPaper(){
	write(sockfd, strCutPaper, 4);
}

int utf8togbk(char *from, char *to){
	char *inbuf= from;
	size_t inlen = strlen(inbuf);
	size_t outlen = inlen *4;
	char *outbuf = to;
	bzero( outbuf, inlen * 4);
	char *in = inbuf;
	char *out = outbuf;
	iconv_t cd=iconv_open("GBK","UTF-8");
	iconv(cd,&in,&inlen,&out,&outlen);
	iconv_close(cd);
	return outlen;
 }

void printLine(){
	if(len <= 0 ) return;
	memset(outbuffer,0,sizeof(outbuffer));
	utf8togbk(linebuffer, outbuffer);
	outlen = strlen(outbuffer);
	write(sockfd, outbuffer, outlen);

	memset(linebuffer,0,sizeof(linebuffer));
	len = 0;
	setFontNormal();
	setAlignLeft();

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

int readPrint(char *fname)
{
	if( fname == NULL )
		return -1;

	FILE *f = fopen(fname, "r");
	if( f == NULL ){
		return -1;
	}

	char c;
	while ( (c = fgetc(f) ) != EOF )
	{
		switch(c){
		case '\n':
			linebuffer[len] = '\n';
			printLine();
			break;
		case '\\':
			c = fgetc(f);
			if( c == 'o'){
				printLine();
				setFontNormal();
			}else if(c == 'b'){
				printLine();
				setFontBold();
			}else if( c == 'w'){
				printLine();
				setFontDoubleWidth();
			}else if( c == 'h'){
				printLine();
				setFontDoubleHeight();
			}else if ( c == '0' ){
				setAlignLeft();
			}else if ( c == 'm' ){
				setAlignCenter();
			}else if ( c == '$' ){
				setAlignRight();
			}else{
				linebuffer[len++] = '\\';
				if( c == EOF ){
					//linebuffer[len] = '\n';
					printLine();
				}else{
					linebuffer[len++] = c;
				}
			}
			break;
		default:
			linebuffer[len++] = c;
			break;
		}

		if( len >= 1023 ){
			//printf("print len >= 1023\n");
			printLine();
		}

	}
	printLine();
	feed4Lines();
	fclose(f);
	openCashDrawer();
	cutPaper();

	return 0;
}

int main(int argc, char **argv){
	if(argc < 2 ){
		//printf("Usage: %s <file>\n", argv[0]);
		//printf("     : <file> the file path to print. \n" );
		printf("%d\n", ERROR );
		return -1;
	}
	char *file = argv[1];
	char server[32];
	int len;
	struct sockaddr_in servaddr;
	timeval tm;
	fd_set set;
	unsigned long ul = 1;
	int error = -1;

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ){
		printf("%d\n", ERROR );
		return -1;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9100);
	getServerAddr( server );
	if( inet_pton(AF_INET, server, &servaddr.sin_addr) <= 0 ){
		printf("%d\n", ERROR );
		return -1;
	}
	ioctl(sockfd, FIONBIO, &ul); //set to nonblocking mode
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
		tm.tv_sec  = TIME_OUT_TIME;
		tm.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		int ret = select(sockfd+1, NULL, &set, NULL, &tm);
		if( ret == 0)
		{
			printf("%d\n", ERROR );
			return -1;
		} else if( ret < 0 ){
			printf("%d\n", ERROR );
			return -1;
		}

		len = sizeof(error);
		int code = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
		if(code <0 || error != 0) {
			//printf("getsockopt(SO_ERROR): %s", strerror(errno)));
																						//                        printf("%d\n", ERROR );
		}
	}
	ul = 0;
	ioctl(sockfd, FIONBIO, &ul); //set back to blocking mode

	int r = readPrint( file );

	if( r < 0 ){
		printf("%d\n", ERROR );
		//printf("Print add to print queue *Failed*\n");
		return -1;
	}

	printf("%d\n", SUCCESS);

	return 0;
}

