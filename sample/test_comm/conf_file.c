#include "conf_file.h"
#include <stdio.h>

int GetProfileString(char *profile, char *KeyName, char *KeyVal)
{
	char buf[100];
	char keyname[50];
	FILE *fp;
	if ((fp = fopen(profile, "r")) == NULL)
	{
		perror("open error");
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	while (!feof(fp) && fgets(buf, 100, fp) != NULL)
	{
		if (strstr(buf, KeyName) != NULL)
		{
			buf[strlen(buf) - 1] = '\0';
			strcpy(KeyVal, buf + strlen(KeyName) + 1);
			break;
		}
	}
	fclose(fp);
	return 0;
}

int changeConfFile(char *KeyName, int Val)
{
	FILE *fp;
	char str[20][20] = {};
	char buf[50] = {};
	char Val_str[10] = {};
	char *p;
	int i, k;
	int ishad = 0;
	if ((fp = fopen("/home/tmp.conf", "r")) == NULL)
	{
		//fprintf(cgiOut, "open error\n");
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	i = 0;
	while (!feof(fp) && fgets(buf, 50, fp) != NULL)
	{
		if ((p = strstr(buf, KeyName)) != NULL)
		{
			sprintf(Val_str, "%d\n\0", Val);
			strcpy(p + strlen(KeyName) + 1, Val_str);
			ishad = 1;
		}
		strcpy(str[i], buf);
		i++;
	}
	fclose(fp);

	if ((fp = fopen("/home/tmp.conf", "w")) == NULL)
	{
		//fprintf(cgiOut, "open error\n");
		return -1;
	}
	for (k = 0; k < i; k++)
	{
		fprintf(fp, "%s", str[k]);
	}
	if (ishad == 0)
	{
		fprintf(fp, "%s=%d\n", KeyName, Val);
	}
	fclose(fp);
	return 1;
}

