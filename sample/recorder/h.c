#include <stdio.h>
int main()
{
    unsigned char a[11] = {0xF0,0x50,0x0F,0X07,0X02,0X06,0X00,0X00,0X00,0x00,0x00};
    unsigned short poi = 0;
    int i;
    for (i = 2; i <= 5; i++)
    {
        poi ^= a[i] << (4 * (5 - i));
    }
    printf("%x\n",poi);
    for (i = 2; i <= 5; i++)
    {
        poi ^= a[i] << (4 * (5 - i));
    }
    printf("%x\n",poi);
}
