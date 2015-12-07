#include "TTY_ctrl.h"
#include <stdio.h>
size_t writeTTY(const unsigned char *buf, int count)
{
	int fd;
	struct termios options;
	size_t ret = 0;
	fd = open("/dev/ttyS000", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0)
	{
		fprintf(cgiOut, "open tty error!\n");
		return -1;
	}
	tcgetattr(fd, &options);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	tcsetattr(fd, TCSANOW, &options);
	tcflush(fd, TCIFLUSH);
	tcflush(fd, TCOFLUSH);
	ret = write(fd, buf, count);
	close(fd);
	return ret;
}

int SetPanTiltPoi(unsigned char YSpeed, unsigned char ZSpeed,
                  unsigned short YPoi, unsigned short ZPoi)
{
	PoiCtrlCode c;
	int ret;
	memset(&c, 0, sizeof(c));
	c.head = htonl(0x87010602);
	c.YSpeed = YSpeed;
	c.ZSpeed = ZSpeed;
	c.YPoi = htonl(NumToContlCode(YPoi));
	c.ZPoi = htonl(NumToContlCode(ZPoi));
	c.tail = 0xFF;
	ret = writeTTY(&c, sizeof(c));
	return ret;
}

int SetPanTiltFLen(short FLen)
{
    FLenCtrlCode f;
    int ret;
    f.head = htonl(0x87010447);
    f.FLen = htonl(NumToContlCode(FLen));
    f.tail = 0xFF;
    //printf("FLen=%x\n",f.FLen);
    ret = writeTTY(&f, sizeof(f));
    return ret;
}

void TurnUp()
{
	writeTTY(up, sizeof(up));
}

void TurnDown()
{
	writeTTY(down, sizeof(down));
}

void TurnLeft()
{
	writeTTY(left, sizeof(left));
}

void TurnRight()
{
	writeTTY(right, sizeof(right));
}

void Stop()
{
	writeTTY(stop, sizeof(stop));
}

void closeFLen()
{
	writeTTY(closeflen, sizeof(closeflen));
}

void furtherFLen()
{
	writeTTY(furtherflen, sizeof(furtherflen));
}

void stopFLen()
{
	writeTTY(flenstop, sizeof(flenstop));
}
