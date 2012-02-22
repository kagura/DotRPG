#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include "script.h"
#include "util.h"
#include <string>

//void OnInterpreterCommand(Interpreter *interpreter, Expression *exp);
void OnInterpreterCommand(Interpreter *interpreter, Command command, const char **args);
/*
struct Variable
{
	std::string name;
	std::string value;
};
*/

const int VARIABLE_MAX = 1000;
//static int variables[VARIABLE_MAX];
static std::string variables[VARIABLE_MAX];
//static std::vector<Variable *> globalVariables;
//static std::vector<Variable *> localVariables;

static Command CommandFromStr(const char *str)
{
	for(int i = 0; i < COMMAND_MAX; i++){
		if(strcmp(str, COMMAND_NAMES[i]) == 0){
			return (Command)i;
		}
	}
	return COMMAND_UNKNOWN;
}

MapScript::~MapScript()
{
	for(int i = 0; i < eventScripts.size(); i++){
		delete eventScripts[i];
	}
	eventScripts.clear();
	for(int i = 0; i < eventProperties.size(); i++){
		delete eventProperties[i];
	}
	eventProperties.clear();
}

static void ReadEventProperty(const char *str, EventProperty *eventProperty)
{
	eventProperty->switchID = -1;
	strcpy(eventProperty->imageFilename, "");
	std::vector<char *> lines;
	SplitStr(str, "\n", &lines);
	std::vector<char *> tokens;
	for(int i = 0; i < lines.size(); i++){
		//printf("lines[%d]=%s\n", i, lines[i]);
		SplitStr(lines[i], " ", &tokens);
		if(strcmp(tokens[0], "image") == 0){
			if(tokens.size() >= 2){
				strcpy(eventProperty->imageFilename, tokens[1]);
			}
		}else if(strcmp(tokens[0], "switch") == 0){
			if(tokens.size() >= 2){
				eventProperty->switchID = atoi(tokens[1]);
			}
		}
		DeleteSplitStr(&tokens);
	}
	DeleteSplitStr(&lines);
	//printf("event image=%s\n", eventProperty->imageFilename);
	//printf("event switchID=%d\n", eventProperty->switchID);
}

MapScript *LoadMapScript(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if(fp == NULL){
		printf("failed open map script file '%s'\n", filename);
		return NULL;
	}
	MapScript *mapScript = new MapScript;
	char mapScriptLine[512];
	char eventScriptLine[512];
	std::string expsStr;
	const char *p = NULL;
	while(fgets(mapScriptLine, sizeof(mapScriptLine), fp) != NULL){
		expsStr = "";
		if(mapScriptLine[strlen(mapScriptLine) - 1] == '\n'){
			//改行を取り除く
			mapScriptLine[strlen(mapScriptLine) - 1] = '\0';
		}
		p = strtok(mapScriptLine, " ");
		bool isEventProperty = strcmp(p, "eventProperty") == 0;
		bool isOnTouchEvent = strcmp(p, "onTouch") == 0;
		bool isOnCheckEvent = strcmp(p, "onCheck") == 0;
		bool isOnEnterEvent = strcmp(p, "onEnter") == 0;
		if(isEventProperty || isOnTouchEvent || isOnCheckEvent || isOnEnterEvent){
			while(fgets(eventScriptLine, sizeof(eventScriptLine), fp) != NULL){
				if(eventScriptLine[0] != '\t' || eventScriptLine[1] == '\n'){
					break;
				}
				expsStr += (eventScriptLine + 1);
			}
			if(isOnTouchEvent || isOnCheckEvent || isOnEnterEvent){
				EventScript *script = CompileEventScript(expsStr.c_str());
				if(script != NULL){
					p = strtok(NULL, " ");
					if(p != NULL){
						script->id = atoi(p);
					}else{
						script->id = -1;
					}
					if(isOnTouchEvent){
						script->trigger = EV_TRIGGER_TOUCH;
					}else if(isOnCheckEvent){
						script->trigger = EV_TRIGGER_EXAMINE;
					}else if(isOnEnterEvent){
						script->trigger = EV_TRIGGER_ENTER;
					}
					mapScript->eventScripts.push_back(script);
				}
			}else if(isEventProperty){
				EventProperty *eventProperty = new EventProperty;
				p = strtok(NULL, " ");
				if(p == NULL) {
					printf("p==NULL\n");
					exit(1);
				}
				eventProperty->id = atoi(p);
				ReadEventProperty(expsStr.c_str(), eventProperty);
				mapScript->eventProperties.push_back(eventProperty);
			}
		}
	}
	fclose(fp);
	return mapScript;
}

EventScript *LoadEventScript(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if(fp == NULL){
		printf("failed open event script '%s'\n", filename);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	unsigned int fileLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *str = new char[fileLength];
	fread(str, fileLength, 1, fp);
	fclose(fp);
	EventScript *script = CompileEventScript(str);
	delete[] str;
	return script;
}

EventScript *CompileEventScript(const char *str)
{
	const char *p = str;
	char *str2 = new char[strlen(str) + 1];
	char *p2 = str2;
	while(*p != '\0'){
		//コメント行なら読み飛ばす
		if(*p == '/' && *(p+1) == '/'){
			while(*p != '\n' && *p != '\0'){
				p++;
			}
		}
		if(*p != '\n'){
			*p2 = *p;
			p2++;
		}
		p++;
	}
	EventScript *script = new EventScript;
	script->expCount = 0;
	//式の数を調べる
	p = str2;
	while(*p != '\0'){
		if(*p == ';'){
			script->expCount++;
		}
		p++;
	}
	script->expressions = new Expression[script->expCount];
	//式を読み取る
	p = str2;
	char token[512];
	unsigned int tokenPos = 0;
	bool stringReading = false;
	for(int i = 0; i < script->expCount; i++){
		//行頭の空白を読み飛ばす
		while(*p == ' ' || *p == '\t'){
			p++;
		}
		//コマンド名を読み取る
		tokenPos = 0;
		while(*p != ' ' && *p != ';'){
			token[tokenPos] = *p;
			tokenPos++;
			p++;
		}
		p++;
		token[tokenPos] = '\0';
		Expression *exp = &script->expressions[i];
		exp->command = CommandFromStr(token);
		if(exp->command == COMMAND_UNKNOWN){
			printf("'%s' is not command name line:%d in CompileEventScript()\n",
				token, i);
			delete[] script->expressions;
			delete script;
			return NULL;
		}
		//コマンド引数を読み取る
		for(int paramIndex = 0; paramIndex < COMMAND_ARGUMENT_COUNTS[exp->command]; paramIndex++){
			tokenPos = 0;
			while(*p != ' ' && *p != ';'){
				if(*p == '"'){
					p++;
					while(*p != '"'){
						token[tokenPos] = *p;
						tokenPos++;
						p++;
					}
					tokenPos--;
				}else if(*p == '\\' && *(p + 1) == 's'){
					token[tokenPos] = ' ';
					p++;
				}else{
					token[tokenPos] = *p;
				}
				tokenPos++;
				p++;
			}
			//規定のコマンド引数の数に達していなければエラー
			if(*p == ';' && paramIndex < COMMAND_ARGUMENT_COUNTS[exp->command] - 1){
				printf("don't match argument count command:%s line:%d in CompileEventScript()\n", 
						COMMAND_NAMES[script->expressions[i].command], i);
				delete[] script->expressions;
				delete script;
				return NULL;
			}
			p++;
			token[tokenPos] = '\0';
			strcpy(script->expressions[i].args[paramIndex], token);
		}
	}
	return script;
}

EventScript *Script_FromExpressions(Expression *expressions, int expCount)
{
	EventScript *script = new EventScript;
	script->expressions = new Expression[expCount];
	script->expCount = expCount;
	for(int i = 0; i < expCount; i++){
		script->expressions[i] = expressions[i];
	}
	return script;
}

void Script_Print(EventScript *script)
{
	printf("script->expCount=%d\n", script->expCount);
	for(int i = 0; i < script->expCount; i++){
		Expression *exp = &script->expressions[i];
		switch(COMMAND_ARGUMENT_COUNTS[exp->command]){
		case 3:
			printf("%s %s %s %s\n", COMMAND_NAMES[exp->command], exp->args[0], exp->args[1], exp->args[2]);
			break;
		case 2:
			printf("%s %s %s\n", COMMAND_NAMES[exp->command], exp->args[0], exp->args[1]);
			break;
		case 1:
			printf("%s %s\n", COMMAND_NAMES[exp->command], exp->args[0]);
			break;
		case 0:
			printf("%s\n", COMMAND_NAMES[exp->command]);
			break;
		}
	}
}

void Script_Delete(EventScript *script)
{
	if(script == NULL){
		return;
	}
	delete[] script->expressions;
	delete script;
}

void Interpreter_Init(Interpreter *interpreter)
{
	interpreter->script = NULL;
	interpreter->expIndex = 0;
	interpreter->interreput = false;
	interpreter->interreputEnded = false;
	interpreter->running = false;
}

void Interpreter_Setup(Interpreter *interpreter, EventScript *script)
{
	interpreter->script = script;
	interpreter->expIndex = 0;
	interpreter->interreput = false;
	interpreter->interreputEnded = false;
	interpreter->running = true;
}

static void SkipToEndIf(Interpreter *interpreter)
{
	interpreter->expIndex++;
	int level = 0;
	while(true){
		if(interpreter->expIndex >= interpreter->script->expCount){
			break;
		}
		Expression *exp = &interpreter->script->expressions[interpreter->expIndex];
		if(exp->command == COMMAND_IF){
			level++;
		}else if(exp->command == COMMAND_ELSE){
			if(level == 0){
				break;
			}
		}else if(exp->command == COMMAND_ENDIF){
			level--;
			if(level < 0){
				break;
			}
		}
		interpreter->expIndex++;
	}
}

//戻り値がtrueならスクリプトを最後まで実行した。
bool Interpreter_Update(Interpreter *interpreter)
{
	if(!interpreter->running){
		return true;
	}
	if(interpreter->interreputEnded){
		interpreter->interreput = false;
		interpreter->interreputEnded = false;
		interpreter->expIndex++;
	}
	if(interpreter->interreput){
		return false;
	}
	Expression *expressions = interpreter->script->expressions;
	std::string varName;
	const char *expandedArgs[3];
	while(true){
		if(interpreter->expIndex >= interpreter->script->expCount){
			break;
		}
		Expression *exp = &interpreter->script->expressions[interpreter->expIndex];
		printf("	%s\n", COMMAND_NAMES[exp->command]);
		//変数を値に展開
		/*
		for(int i = 0; i < COMMAND_ARG_MAX; i++){
			if(exp->args[i][0] == '$'){ //グローバル変数
				varName = exp->args[i] + 1;
				for(int i = 0; i < globalVariables.size(); i++){
					if(globalVariables[i]->name == varName){
						expandedArgs[i] = globalVariables[i]->value.c_str();
						break;
					}
				}
			}if(exp->args[i][0] == '@'){ //ローカル変数
				varName = exp->args[i] + 1;
				for(int i = 0; i < localVariables.size(); i++){
					if(localVariables[i]->name == varName){
						expandedArgs[i] = localVariables[i]->value.c_str();
						break;
					}
				}
			}else{
				expandedArgs[i] = exp->args[i];
			}
		}
		*/
		for(int i = 0; i < COMMAND_ARG_MAX; i++){
			expandedArgs[i] = exp->args[i];
		}
		//printf("command %s\n", COMMAND_NAMES[exp->command]);
		switch(exp->command){
		case COMMAND_IF:
			if(variables[atoi(exp->args[0])] != exp->args[1]){
				//endif までスキップ
				SkipToEndIf(interpreter);
			}
			break;
		case COMMAND_ELSE:
			//endif までスキップ
			SkipToEndIf(interpreter);
			break;
		case COMMAND_SET:
			variables[atoi(exp->args[0])] = exp->args[1];
			//SetGlobalVariable(exp->args[0], const char *value);
			break;
		}
		OnInterpreterCommand(interpreter, exp->command, expandedArgs);
		if(interpreter->interreput){
			return false;
		}
		if(!interpreter->running){
			break;
		}
		
		interpreter->expIndex++;
	}
	interpreter->running = false;
	return true;
}

Command Interpreter::NextCommand()
{
	if(!running){
		return COMMAND_UNKNOWN;
	}
	if((int)expIndex >= script->expCount - 1){
		return COMMAND_UNKNOWN;
	}
	return script->expressions[expIndex + 1].command;
}

void InitEventGlobal()
{
	for(int i = 0; i < VARIABLE_MAX; i++){
		variables[i] = "";
	}
	/*
	localVariables.clear();
	globalVariables.clear();
	*/
}

void WriteEventGlobalState(FILE *fp)
{
	int strLength = 0;
	for(int i = 0; i < VARIABLE_MAX; i++){
		strLength = variables[i].size();
		fwrite(&strLength, sizeof(unsigned int), 1, fp);
		fwrite(variables[i].c_str(), sizeof(char) * variables[i].size(), 1, fp);
	}
	/*
	int variableNum = globalVariables.size();
	fwrite(&variableNum, sizeof(unsigned int), 1, fp);
	int strLength = 0;
	for(int i = 0; i < globalVariables.size(); i++){
		strLength = globalVariables[i]->name.size();
		fwrite(&strLength, sizeof(unsigned int), 1, fp);
		fwrite(globalVariables[i]->name.c_str(), sizeof(char) * strLength, 1, fp);
		strLength = globalVariables[i]->value.size();
		fwrite(&strLength, sizeof(unsigned int), 1, fp);
		fwrite(globalVariables[i]->value.c_str(), sizeof(char) * strLength, 1, fp);
	}
	variableNum = localVariables.size();
	fwrite(&variableNum, sizeof(unsigned int), 1, fp);
	for(int i = 0; i < localVariables.size(); i++){
		strLength = localVariables[i]->name.size();
		fwrite(&strLength, sizeof(unsigned int), 1, fp);
		fwrite(localVariables[i]->name.c_str(), sizeof(char) * strLength, 1, fp);
		strLength = localVariables[i]->value.size();
		fwrite(&strLength, sizeof(unsigned int), 1, fp);
		fwrite(localVariables[i]->value.c_str(), sizeof(char) * strLength, 1, fp);
	}
	*/
}

void ReadEventGlobalState(FILE *fp)
{
	int strLength = 0;
	int maxStrLength = 0;
	char *buf = NULL;
	for(int i = 0; i < VARIABLE_MAX; i++){
		fread(&strLength, sizeof(unsigned int), 1, fp);
		if(strLength > maxStrLength){
			if(buf != NULL){
				delete[] buf;
			}
			buf = new char[strLength+1];
			maxStrLength = strLength;
		}
		fread(buf, sizeof(char), strLength, fp);
		buf[strLength] = '\0';
		variables[i] = buf;
	}
	/*
	Variable *var = NULL;
	int variableNum;
	fread(&variableNum, sizeof(unsigned int), 1, fp);
	for(int i = 0; i < variableNum; i++){
		var = new Variable;
		globalVariables.push_back(var);
		fread(&strLength, sizeof(unsigned int), 1, fp);
		if(strLength > maxStrLength){
			if(buf != NULL){
				delete[] buf;
			}
			buf = new char[strLength+1];
			maxStrLength = strLength;
		}
		fread(buf, sizeof(char), strLength, fp);
		buf[strLength] = '\0';
		var->name = buf;
		fread(&strLength, sizeof(unsigned int), 1, fp);
		if(strLength > maxStrLength){
			if(buf != NULL){
				delete[] buf;
			}
			buf = new char[strLength+1];
			maxStrLength = strLength;
		}
		fread(buf, sizeof(char), strLength, fp);
		buf[strLength] = '\0';
		var->value = buf;
	}
	*/
	if(buf != NULL){
		delete[] buf;
	}
}

const char *GetVariable(int index)
{
	if(index < 0 || index >= VARIABLE_MAX){
		return NULL;
	}
	return variables[index].c_str();
}
/*
const char *GetGlobalVariable(const char *name)
{
	for(int i = 0; i < globalVariables.size(); i++){
		if(globalVariables[i]->name == name){
			return globalVariables[i]->value.c_str();
		}
	}
	return NULL;
}

void SetGlobalVariable(const char *name, const char *value)
{
	for(int i = 0; i < globalVariables.size(); i++){
		if(globalVariables[i]->name == name){
			globalVariables[i]->value = value;
			return;
		}
	}
	Variable *var = new Variable;
	var->name = name;
	var->value = value;
}
*/