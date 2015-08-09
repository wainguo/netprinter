#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TIME_OUT_TIME 5
#define ERROR 9
#define PAPERENDED 2
#define PRINTEROFFLINE 1
#define PRINTERREADY 0
#define SERVERCONF "server.cfg"
#define DFTSERVER "10.1.72.251"

unsigned char TestStatusBits[4];
int rTest = 0;
unsigned char PrintStatusBits[4];
int rPrint= 0;

bool isPrinterOnline(){
	if(rTest != 4 ) return false;
	if( (TestStatusBits[0]&0x08) == 0x08 ) return false;
	return true;
}

/* true: lack of paper, false: plenty of paper */
bool isPaperEnded(){
	if(rTest==4 && (TestStatusBits[2]&0x0C == 0x0C) ) return true;
	return false;
}

bool isCoverOpened(){
	if(rTest != 4 ) return true;
	if((TestStatusBits[0]&0x08) == 0x08 ) return true;
	return false;
}

/*
bool isDrawerOpened(){
	if(rTest==4 && (TestStatusBits[0]&0x04 == 0) ) return false;
	return true;
}

bool isBusy(){
	return false;
}
*/

void testStatus( int sockfd ){
	/* ESC v */
	char strTestStatus[] = "\x1B\x76";
	/* ESC x */
	char strPrintStatus[] = "\x1B\x78";

        int w = write(sockfd, strTestStatus, 2);
        if( w != 2 ){
		return;
        }

        rTest = read(sockfd, TestStatusBits, 4); 

	/*
        if( rTest< 4 ){
                printf("read error.\n");
	}
	int i;
	for(i=0; i<rTest; i++)
		printf("0x%02x ", TestStatusBits[i]);
	printf("\n");
	*/
}

void getStatus(){
	if( isPaperEnded() ){
		printf("%d\n", PAPERENDED);
		return;
	}
	if( !isPrinterOnline() ){
		printf("%d\n", PRINTEROFFLINE );
		return;
	}
	printf("%d\n", PRINTERREADY );

}

void test(){
	printf("Printer is %s\n", isPrinterOnline()?"Online":"Offline");
	printf("Printer is %s\n", isPaperEnded()?"Paper ended":"Paper enough");
	printf("Printer is %s\n", isCoverOpened()?"Cover opened":"Cover closed");
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

int main(){
	int sockfd, len;
	char server[32];
	struct sockaddr_in servaddr;
	timeval tm;
	fd_set set;
	unsigned long ul = 1;
	int error = -1;

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ){
		//printf("socket error.\n");
		printf("%d\n", ERROR );
		return -1;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(4000);

	getServerAddr( server );

	if( inet_pton(AF_INET, server, &servaddr.sin_addr) <= 0 ){
		//printf("inet_pton error.\n");
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
			printf("%d\n", ERROR );
			return -1;
		}
	}
	ul = 0;
	ioctl(sockfd, FIONBIO, &ul); //set back to blocking mode
	testStatus( sockfd );
	getStatus();
	//test();

	return 0;
}

