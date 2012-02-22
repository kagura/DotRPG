#include "gamemain.h"
#include "gamefunc.h"
#include "script.h"
#include "util.h"
#include "map.h"
#include "party.h"
#include "enemy.h"
#include "mapscene.h"
#include "battlescene.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stack>

#define snprintf _snprintf

enum Scene
{
	SCENE_MAP,
	SCENE_BATTLE,
};

static const int PARTY_FIRST_ID = 1000;
static const int MAPCHIP_VISIBLE_W = 8;
static const int MAPCHIP_VISIBLE_H = 10;
static const int PLAYER_W = 64;
static std::string messageText;
static int messageWindowTex;
static int messageWaitTex;
static const int SAVEDATA_VERSION = 2;
static int mapViewX = 0;
static int mapViewY = 0;
static int screenW = 1;
static int screenH = 1;
static Interpreter mainInterpreter;
static bool messageShowing = false;
static Interpreter *messageShowInterpreter = NULL;
static MapScript *mapScript = NULL;
static bool messageWindowCloseWhenExit = false;
static bool interpreterPause = false;
static bool isMapChanging = false;
static char nextMapName[FILENAME_MAX];
static int nextMapStartX = 0;
static int nextMapStartY = 0;
static std::vector<int> charaIDAry;
static char speakerName[512] = "";
static bool inBattle = false;
static std::stack<Scene> sceneStack;
static int count = 0;

static void InitScene()
{
	switch(sceneStack.top()){
	case SCENE_BATTLE:
		BattleScene_Init();
		break;
	}
}

static void PushScene(Scene nextScene)
{
	sceneStack.push(nextScene);
	InitScene();
}

static void UpdateMapScroll()
{
	if(GetPlayer() == NULL) return;
	Map_SetScroll(
		GetPlayer()->x - (MAPCHIP_VISIBLE_W - 2) * MAPCHIP_W / 2 - PLAYER_W / 2,
		GetPlayer()->y - (MAPCHIP_VISIBLE_H - 2) * MAPCHIP_H / 2
	);
}

static void ChangeMap(const char *name, int startX, int startY)
{
	if(GetPlayer() == NULL) return;
	if(mapScript != NULL){
		delete mapScript;
		mapScript = NULL;
	}
	char scriptFilename[FILENAME_MAX];
	sprintf(scriptFilename, "%s.txt", name);
	mapScript = LoadMapScript(scriptFilename);
	if(mapScript == NULL){
		exit(1);
	}
	Map_Setup(name, mapScript);
	GetPlayer()->x = startX * MAPCHIP_W;
	GetPlayer()->y = startY * MAPCHIP_H;
	UpdateMapScroll();
	for(int i = 1; i < GetPartyMemberCount(); i++){
		GetPartyMemberDestPos(i, &GetPartyMember(i)->x, &GetPartyMember(i)->y);
	}
	charaIDAry.clear();
	for(int i = 0; i < GetPartyMemberCount(); i++){
		charaIDAry.push_back(PARTY_FIRST_ID + i);
	}
	for(int i = 0; i < GetEventsCount(); i++){
		charaIDAry.push_back(i);
	}
	for(int i = 0; i < mapScript->eventScripts.size(); i++){
		if(mapScript->eventScripts[i]->trigger == EV_TRIGGER_ENTER){
			Interpreter_Setup(&mainInterpreter, mapScript->eventScripts[i]);
			break;
		}
	}
}

static const char savedata_format[] = "savedata_%02d";

static void SaveGame(int id)
{
	char filename[FILENAME_MAX];
	sprintf(filename, savedata_format, id);
	FILE *fp = fopen(filename, "w");
	if(fp == NULL){
		printf("failed open '%s' in SaveGame()", filename);
		return;
	}
	fwrite(&SAVEDATA_VERSION, sizeof(int), 1, fp);
	WriteEventGlobalState(fp);
	fclose(fp);
}

static void LoadGame(int id)
{
	char filename[FILENAME_MAX];
	sprintf(filename, savedata_format, id);
	FILE *fp = fopen(filename, "r");
	if(fp == NULL){
		printf("failed open '%s' in LoadGame()", filename);
		return;
	}
	int version = 0;
	fread(&version, sizeof(int), 1, fp);
	if(version == SAVEDATA_VERSION){
		ReadEventGlobalState(fp);
	}
	fclose(fp);
}

static void MakeMessageText(const char *str)
{
	const char *p = str;
	std::stringstream os;
	while(*p != '\0'){
		if(*p == '\\' && *(p + 1) == 'v'){
			p += 2;
			int varIndex;
			if(sscanf(p, "%d", &varIndex) != 1){
				printf("message '%s' '\v' variable index not set.", str);
				return;
			}
			os << GetVariable(varIndex);
			char s[128];
			snprintf(s, 127, "%d", varIndex);
			p += strlen(s);
		}else{
			os << *p;
			p++;
		}
	}
	messageText = os.str();
}

void PopScene()
{
	if(sceneStack.size() > 1){
		sceneStack.pop();
	}
}

void ShowMessageWindow(const char *text, bool closeWhenExit)
{
	MakeMessageText(text);
	messageShowing = true;
	messageWindowCloseWhenExit = closeWhenExit;
}

void CloseMessageWindow()
{
	messageShowing = false;
	if(messageShowInterpreter != NULL){
		messageShowInterpreter->interreputEnded = true;
		messageShowInterpreter = NULL;
	}
	printf("CloseMessageWindow\n");
}

bool IsMessageShowEnded()
{
	return messageShowing;
}

void SetSpeakerName(const char *name)
{
	strcpy(speakerName, name);
}

Interpreter *GetCurrentInterpreter()
{
	return &mainInterpreter;
}

void PauseInterpreter()
{
	interpreterPause = true;
}

void ResumeInterpreter()
{
	interpreterPause = false;
}

void OnInterpreterCommand(Interpreter *interpreter, Command command, const char **args)
{
	switch(command){
	case COMMAND_MSG:
		{
			ShowMessageWindow(args[0], interpreter->NextCommand() != COMMAND_MSG);
			messageShowInterpreter = interpreter;
			messageShowInterpreter->interreput = true;
		}
		break;
	case COMMAND_SPEAKER:
		SetSpeakerName(args[0]);
		break;
	case COMMAND_SAVE:
		SaveGame(atoi(args[0]));
		break;
	case COMMAND_LOAD:
		LoadGame(atoi(args[0]));
		break;
	case COMMAND_SET:
		{
			for(int i = 0; i < GetEventsCount(); i++){
				UpdateEventProperty(GetEvent(i));
			}
		}
		break;
	case COMMAND_CHANGEMAP:
		isMapChanging = true;
		strcpy(nextMapName, args[0]);
		nextMapStartX = atoi(args[1]);
		nextMapStartY = atoi(args[2]);
		interpreter->running = false;
		break;
	case COMMAND_ADD_PARTY:
		{
			int addedPartyMemberIndex = AddPartyMember(args[0]);
			if(addedPartyMemberIndex != -1){
				SetPlayer(0);
				charaIDAry.push_back(PARTY_FIRST_ID + addedPartyMemberIndex);
				GetPartyMemberDestPos(
					addedPartyMemberIndex,
					&GetPartyMember(addedPartyMemberIndex)->x,
					&GetPartyMember(addedPartyMemberIndex)->y
				);
			}
		}
		break;
	case COMMAND_STAFFROLL:
		exit(0);
		break;
	}
}

void GameInputKey(InputKey key, InputKeyState state)
{
	switch(sceneStack.top()){
	case SCENE_MAP:
		MapScene_InputKey(key, state);
		break;
	case SCENE_BATTLE:
		BattleScene_InputKey(key, state);
		break;
	}
	if(messageShowing && key == KEY_DECIDE && state == KEY_STATE_DOWN){
		if(messageShowInterpreter != NULL){
			messageShowInterpreter->interreputEnded = true;
			messageShowInterpreter = NULL;
		}
		if(messageWindowCloseWhenExit){
			messageShowing = false;
			messageText = "";
		}
	}
}

void GameUpdate()
{
	mapViewX = screenW / 2 - MAPCHIP_VISIBLE_W * MAPCHIP_W / 2;
	mapViewY = screenH / 2 - MAPCHIP_VISIBLE_H * MAPCHIP_H / 2;
	
	switch(sceneStack.top()){
	case SCENE_MAP:
		MapScene_Update();
		break;
	case SCENE_BATTLE:
		BattleScene_Update();
		break;
	}
	
	UpdateMapScroll();
	
	if(!interpreterPause){
		Interpreter_Update(&mainInterpreter);
		if(isMapChanging){
			ChangeMap(nextMapName, nextMapStartX, nextMapStartY);
			isMapChanging = false;
		}
	}
	
	count++;
	if(count == 10){
		AddBattleEnemy(0);
		AddBattleEnemy(0);
		PushScene(SCENE_BATTLE);
	}
}

bool CharaZComp(const int& riLeft, const int& riRight)
{
	int leftZ = 0;
	if(riLeft >= PARTY_FIRST_ID){
		leftZ = GetPartyMember(riLeft - PARTY_FIRST_ID)->y;
	}else{
		leftZ = GetEvent(riLeft)->y;
	}
	int rightZ = 0;
	if(riRight >= PARTY_FIRST_ID){
		rightZ = GetPartyMember(riRight - PARTY_FIRST_ID)->y;
	}else{
		rightZ = GetEvent(riRight)->y;
	}
    return leftZ < rightZ;
}

void GameRender()
{
	SetViewport(
		mapViewX + MAPCHIP_W,
		mapViewY + MAPCHIP_H, 
		(MAPCHIP_VISIBLE_W - 2) * MAPCHIP_W,
		(MAPCHIP_VISIBLE_H - 2) * MAPCHIP_H
	);
	
	DrawUnderMap(mapViewX, mapViewY, MAPCHIP_VISIBLE_W, MAPCHIP_VISIBLE_H);
	
	std::sort(charaIDAry.begin(), charaIDAry.end(), CharaZComp);
	for(int i = 0; i < charaIDAry.size(); i++){
		if(charaIDAry[i] >= PARTY_FIRST_ID){
			PartyMember *member = GetPartyMember(charaIDAry[i] - PARTY_FIRST_ID);
			DrawTexture(
				member->image,
				-GetMapScrollX() + member->x + mapViewX,
				-GetMapScrollY() + member->y + mapViewY
			);
		}else{
			Event *event = GetEvent(charaIDAry[i]);
			if(event->image != -1){
				DrawTexture(
					event->image,
					-GetMapScrollX() + event->x + mapViewX,
					-GetMapScrollY() + event->y + mapViewY
				);
			}
		}
	}

	SetViewport(
		0, 0, screenW, screenH
	);
	
	if(sceneStack.top() == SCENE_BATTLE){
		BattleScene_RenderEnemies();
		BattleScene_RenderPartyStateWindow();
		BattleScene_RenderCommandChoices();
		BattleScene_RenderSkillChoicesWindow();
	}
	
	if(messageShowing){
		DrawTexture(
			messageWindowTex,
			screenW / 2 - GetTextureWidth(messageWindowTex) / 2,
			screenH - GetTextureHeight(messageWindowTex)
		);

		DrawTexture(
			messageWaitTex,
			screenW / 2 + GetTextureWidth(messageWindowTex) / 2 - GetTextureWidth(messageWaitTex) - 20,
			screenH - GetTextureHeight(messageWaitTex) - 20
		);
		
		DrawString(
			screenW / 2 - GetTextureWidth(messageWindowTex) / 2 + 40, 
			screenH - GetTextureHeight(messageWindowTex) + 40,
			messageText.c_str()
		);
		
		DrawString(
			screenW / 2 - GetTextureWidth(messageWindowTex) / 2 + 10, 
			screenH - GetTextureHeight(messageWindowTex),
			speakerName
		);
	}
	
	if(sceneStack.top() == SCENE_BATTLE){
		BattleScene_RenderHelpWindow();
	}
}

void GameInit()
{
	messageWindowTex = LoadTexture("msgWin.tga");
	messageWaitTex = LoadTexture("msgWait.tga");

	messageText = "";
	
	Interpreter_Init(&mainInterpreter);
	
	InitEventGlobal();
	
	LoadMapInfo();
	LoadParty();
	LoadEnemy();
	LoadDatabase();
	
	LoadGame(0);
	
	EventScript *script = LoadEventScript("init.txt");
	if(script == NULL){
		exit(1);
	}
	Interpreter_Setup(&mainInterpreter, script);
	Interpreter_Update(&mainInterpreter);
	
	PushScene(SCENE_MAP);
}

void GameScreenResized(int width, int height)
{
	screenW = width;
	screenH = height;
}

void GameDestroy()
{
	Map_Cleanup();
	CleanupMapInfo();
	CleanupParty();
	CleanupEnemy();
	CleanupDatabase();
}

int GetScreenWidth()
{
	return screenW;
}

int GetScreenHeight()
{
	return screenH;
}