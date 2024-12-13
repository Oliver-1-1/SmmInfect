#include "string.h"

int ToLower(int c)
{
    return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c;
}

int StrCmpi(const char* str1, const char* str2)
{
    while (*str1 && (ToLower(*str1) == ToLower(*str2)))
    {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

int WcsCmpi(const unsigned short* str1, const unsigned short* str2)
{
    while (*str1 && (ToLower(*str1) == ToLower(*str2)))
    {
        str1++;
        str2++;
    }
    return *(const unsigned short*)str1 - *(const unsigned short*)str2;
}
