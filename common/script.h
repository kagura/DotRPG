#ifndef SCRIPT_H
#define SCRIPT_H

#include <vector>

enum Command
{
	COMMAND_UNKNOWN,
	COMMAND_MSG,
	COMMAND_SPEAKER,
	COMMAND_IF,
	COMMAND_ELSE,
	COMMAND_ENDIF,
	COMMAND_SET,
	COMMAND_ASK,
	COMMAND_SAVE,
	COMMAND_LOAD,
	COMMAND_CHANGEMAP,
	COMMAND_ADD_PARTY,
	COMMAND_STAFFROLL,
	COMMAND_MAX,
};

const char * const COMMAND_NAMES[] = {
	"unknown",
	"msg",
	"speaker",
	"if",
	"else",
	"endif",
	"set",
	"ask",
	"save",
	"load",
	"changemap",
	"addparty",
	"staffroll",
};

const int COMMAND_ARGUMENT_COUNTS[] = {
	0,
	1,
	1,
	2,
	0,
	0,
	2,
	2,
	1,
	1,
	3,
	1,
	0,
};

const int COMMAND_ARG_MAX = 3;

struct Expression
{
	Command command;
	char args[COMMAND_ARG_MAX][512];
};

enum EventTrigger
{
	EV_TRIGGER_TOUCH, //触れたとき実行
	EV_TRIGGER_EXAMINE, //調べたとき実行
	EV_TRIGGER_ENTER, //マップに入ったとき実行
};

struct EventScript
{
	Expression *expressions;
	unsigned int expCount;
	int id;
	EventTrigger trigger;
};

struct Interpreter
{
	EventScript *script;
	unsigned int expIndex;
	bool running;
	bool interreput;
	bool interreputEnded;
	
	Command NextCommand();
};

struct EventProperty
{
	int id;
	char imageFilename[FILENAME_MAX];
	int switchID; //-1なら無し
};

struct MapScript
{
	~MapScript();
	
	std::vector<EventProperty *> eventProperties;
	std::vector<EventScript *> eventScripts;
};

MapScript *LoadMapScript(const char *filename);
EventScript *LoadEventScript(const char *filename);
EventScript *CompileEventScript(const char *str);
EventScript *Script_FromExpressions(Expression *expressions, int expCount);
void Script_Print(EventScript *script);
void Script_Delete(EventScript *script);
void Interpreter_Init(Interpreter *interpreter);
void Interpreter_Setup(Interpreter *interpreter, EventScript *script);
bool Interpreter_Update(Interpreter *interpreter);

void InitEventGlobal();
void WriteEventGlobalState(FILE *fp);
void ReadEventGlobalState(FILE *fp);
const char *GetVariable(int index);
//const char *GetGlobalVariable(const char *name);
//void SetGlobalVariable(const char *name, const char *value);

#endif