#pragma once

#include <stddef.h>

char* strcpy(char* dst, char* src)
{
	char c;
	char* curDst = dst;
	char* curSrc = src;

	if (dst && src)
	{
		c = *curSrc;
		while (c)
		{
			c = *curSrc;
			*curDst = c;
			curSrc++;
			curDst++;
		}
		return src;
	}
	return NULL;
}

size_t strlen(char* str)
{
	if (!str)
		return NULL;
	
	size_t s;
	while (*str++)
		s++;
	
	return s;
}

int strncmp(char* s1, char* s2, int len)
{
	int ret = 0;
	if (!s1 || !s2)
	{
		if (s1 != s2 && (ret = -1, s1 != NULL))
			return 1;
	}
	else
	{
		while (len)
		{
			char c2 = *s2;
			char c1 = *s1;
			if (c1 != c2) break;
			s1++;
			if (c1 == '\0')
				return 0;
			len--;
			s2++;
		}
		if (len)
			return *s1 - *s2;
		ret = 0;
	}
	return ret;
}