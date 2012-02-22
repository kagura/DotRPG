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