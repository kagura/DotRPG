#include <vector>

void SplitStr(const char *str, const char *separetors, std::vector<char *> *splitedStrs);
void DeleteSplitStr(std::vector<char *> *splitedStrs);
const char *sgets(char *buf, char **s);
char *LoadChars(const char *filename);