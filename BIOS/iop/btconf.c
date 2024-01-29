#include "btconf.h"

int ParseLoadAddr(char** str)
{
	char* buf = *str;
	int ret = 0;
	while ('/' < *buf)
	{
		int i;
		char c = *buf;
		buf++;
		if (c <= '9')
			i = c - '0';
		else if (c <= 'f')
			i = c - 'a';
		else if (c <= 'F')
			i = c - 'A';
		
		ret = ret * 16 + i;
	}

	*str = buf;
	return ret;
}