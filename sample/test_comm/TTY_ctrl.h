#ifndef _TTY_CTRL_H
#define _TTY_CTRL_H

const unsigned char up[] = {0x87, 0x01, 0x06, 0x01, 0x10, 0x10, 0x03, 0x01, 0xFF};
const unsigned char down[] = {0x87, 0x01, 0x06, 0x01, 0x10, 0x10, 0x03, 0x02, 0xFF};
const unsigned char left[] = {0x87, 0x01, 0x06, 0x01, 0x10, 0x10, 0x01, 0x03, 0xFF};
const unsigned char right[] = {0x87, 0x01, 0x06, 0x01, 0x10, 0x10, 0x02, 0x03, 0xFF};
const unsigned char stop[] = {0x87, 0x01, 0x06, 0x01, 0x99, 0x99, 0x03, 0x03, 0xFF};
const unsigned char closeflen[] = {0x87, 0x01, 0x04, 0x07, 0x02, 0xFF};
const unsigned char furtherflen[] = {0x87, 0x01, 0x04, 0x07, 0x03, 0xFF};
const unsigned char ctrlcode[] = {0x87, 0x09, 0x06, 0x12, 0xFF};
const unsigned char flenstop[] = {0x87, 0x01, 0x04, 0x07, 0x00, 0xFF};

#pragma pack(1)
typedef struct
{
	unsigned int head;
	unsigned char YSpeed;
	unsigned char ZSpeed;
	unsigned int YPoi;
	unsigned int ZPoi;
	unsigned char tail;
} PoiCtrlCode;

typedef struct
{
	unsigned int head;
	unsigned int FLen;
	unsigned char tail;
} FLenCtrlCode;
#pragma pack()

size_t writeTTY(const unsigned char *buf, int count);
int SetPanTiltFLen(short FLen);
int SetPanTiltPoi(unsigned char YSpeed, unsigned char ZSpeed,
                  unsigned short YPoi, unsigned short ZPoi);
void Stop();
void TurnDown();
void TurnLeft();
void TurnRight();
void TurnUp();
void closeFLen();
void furtherFLen();
void StopFLen();

#endif
