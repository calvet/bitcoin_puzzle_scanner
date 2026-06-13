#ifndef CUSTOMUTILH
#define CUSTOMUTILH


char *ltrim(char *str, const char *seps);
char *rtrim(char *str, const char *seps);
char *trim(char *str, const char *seps);

char *tohex(char *ptr,int length);
void tohex_dst(char *ptr,int length,char *dst);



#endif // CUSTOMUTILH
