#include <stdio.h>
#include <windows.h>
#define bool int
#define true 1
#define false 0
bool is_utf8(const char * string)
{
	if (!string)
		return true;

	const unsigned char * bytes = (const unsigned char *)string;
	int num;

	while (*bytes != 0x00)
	{
		if ((*bytes & 0x80) == 0x00)
		{
			/* U+0000 to U+007F */
			num = 1;
		}
		else if ((*bytes & 0xE0) == 0xC0)
		{
			/* U+0080 to U+07FF */
			num = 2;
		}
		else if ((*bytes & 0xF0) == 0xE0)
		{
			/* U+0800 to U+FFFF */
			num = 3;
		}
		else if ((*bytes & 0xF8) == 0xF0)
		{
			/* U+10000 to U+10FFFF */
			num = 4;
		}
		else
			return false;

		bytes += 1;
		for (int i = 1; i < num; ++i)
		{
			if ((*bytes & 0xC0) != 0x80)
				return false;
			bytes += 1;
		}
	}

	return true;
}

static wchar_t * convUtf8ToWC(char *utf8str)
{

	if (utf8str == NULL) return NULL;
	int wlen;
	wlen = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8str, -1, NULL, 0);
	if (wlen == 0) return NULL;

	wchar_t *p = (wchar_t*)malloc((wlen+1)*sizeof(wchar_t));
	wlen = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8str, -1, (LPWSTR)p, wlen + 1);

	return p;

}

static char * convWcToLocal(wchar_t *wstr)
{
	if (wstr == NULL) return NULL;
	int len;
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (len == 0) return NULL;

	char *p = (char*)malloc(len + 1);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, p, len + 1, NULL, NULL);

	return p;

}

static char* convUtf8ToLocal(char *utf8)
{
	wchar_t *wstr = convUtf8ToWC(utf8);
	if (wstr == NULL) return NULL;
	char *str = convWcToLocal(wstr);
	free(wstr);
	return str;
}

/*
* Convert Windows coding to UTF8
* return NULL if error
* you will need free result if not NULL
*/
char* win32local2utf8(char *str) /* vvs */
{
	int len, wlen, utf8len;
	size_t size, wsize;
	wchar_t *p;
	char *p2;

	if (str == NULL)
		return NULL;

	/* convert to wide char */
	size = strlen(str) + 1;
	wsize = size * sizeof(wchar_t);
	p = (wchar_t*)malloc(wsize);
	wlen = MultiByteToWideChar(CP_ACP, 0, (LPCCH)str, (int)size, (LPWSTR)p, (int)size);
	if (wlen == 0)
	{
		free(p);
		return NULL;
	}

	/* check size of utf8 string */
	utf8len = WideCharToMultiByte(CP_UTF8, 0, p, -1, NULL, 0, NULL, NULL);
	if (utf8len == 0)
	{
		free(p);
		return NULL;
	}

	p2 = (char*)malloc(utf8len);
	/* perform the conversion to utf8 */
	len = WideCharToMultiByte(CP_UTF8, 0, p, -1, p2, utf8len, NULL, NULL);
	if (len == 0)
	{
		free(p);
		free(p2);
		return NULL;
	}

	free(p);
	return p2;
}


void printHelp()
{
		printf("App repairs string coding in the log file\n");
		printf("Converts UTF-8 strings to Windows encoding by default.\n");
		printf("Command line options:\n");
		printf("pg_winlog_repair input_log_file output_file [-utf8]\n");
		printf("Use -utf8 option for convert to UTF-8\n");

}

int main(int argc, char *argv[])
{
	FILE *in, *out;
	char str[16000];
	char *pStr;
	int isUtf=0;
	int ch;

	if(argc<3){
		printHelp();

		return 1;
	}
	char *pIn=0,*pOut=0;
	for(int i=1;i<argc;i++){
		if(!strcmp(argv[i],"-utf8")) isUtf=1;
		else if(!pIn) pIn=argv[i];
		else if(!pOut) pOut=argv[i];
		
	}
	if(!pIn || !pOut){
		printHelp();
		return 1;
	}
	
	


	//if(argc>3 && !strcmp(argv[3],"-utf8")) isUtf=1;
	if(isUtf) printf("Converting to UTF-8\n");



	in = fopen(pIn,"rt");
	if(!in){
		printf("Error open input file!\n");
		return 1;
	}
	out = fopen(pOut,"wt");
	if(!in){
		printf("Error open output file!\n");
		fclose(in);
		return 1;
	}
	
        while(!feof (in)) {
            if (fgets(str, sizeof(str), in)){
		ch=is_utf8(str);
		if(ch && !isUtf){//convert to cp1251
			pStr = convUtf8ToLocal(str);
			if(pStr) fputs(pStr, out);
			else fputs(str, out);
			free(pStr);
		}
		else if(!ch && isUtf){
			pStr = win32local2utf8(str);
			if(pStr) fputs(pStr, out);
			else fputs(str, out);
			free(pStr);
		}
		else fputs(str, out);
            }
        }

        fclose(out);
        fclose(in);

	return 0;

}
