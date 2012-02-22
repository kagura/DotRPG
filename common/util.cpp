#include "util.h"
#include <string.h>
#include <math.h>

void SplitStr(const char *str, const char *separetors, std::vector<char *> *splitedStrs)
{
	char *str2 = new char[strlen(str) + 1];
	strcpy(str2, str);
	char *strout;
	const char *p = strtok(str2, separetors);
	strout = new char[strlen(p) + 1];
	strcpy(strout, p);
	splitedStrs->push_back(strout);
	while((p = strtok(NULL, separetors)) != NULL){
		strout = new char[strlen(p) + 1];
		strcpy(strout, p);
		splitedStrs->push_back(strout);
	}
	delete[] str2;
}

void DeleteSplitStr(std::vector<char *> *splitedStrs)
{
	for(int i = 0; i < splitedStrs->size(); i++){
		delete[] splitedStrs->at(i);
	}
	splitedStrs->clear();
}

const char *sgets(char *buf, char **s)
{
	if(**s == '\0') return NULL;
	const char *p = strchr(*s, '\n');
	if(p != NULL){
		strncpy(buf, *s, p - *s - 1);
		buf[p - *s - 1] = '\0';
		*s += p - *s;
		*s += 1;
	}else{
		strcpy(buf, *s);
		*s += strlen(*s);
	}
	return *s;
}

char *LoadChars(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL){
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	unsigned int fileLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *str = new char[fileLength + 1];
	fread(str, fileLength, 1, fp);
	str[fileLength] = '\0';
	fclose(fp);
	return str;
}