#include "gamemain.h"
#include "gamefunc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <sstream>
#include "script.h"
#include "fmfmap.h"
#include "util.h"
#include "DxLib.h"
#include <algorithm>

#define snprintf _snprintf

struct Event
{
	int id;
	int x, y;
	int mapX, mapY;
	int image;
	EventScript *onTouchScript;
	EventScript *onExamineScript;
	std::vector<EventProperty *> properties;
};

enum CharaAngle
{
	CHARA_ANGLE_RIGHT,
	CHARA_ANGLE_LEFT,
	CHARA_ANGLE_UP,
	CHARA_ANGLE_DOWN,
};

struct ChipSet
{
	char name[FILENAME_MAX];
	char imageFilename[FILENAME_MAX];
	std::vector<unsigned int> impassableIDs;
};

struct MapInfo
{
	char name[FILENAME_MAX];
	char chipsetName[FILENAME_MAX];
	ChipSet *chipSet;
};

struct PartyMember
{
	char name[FILENAME_MAX];
	char imageFilename[FILENAME_MAX];
	int x, y;
	int dirX, dirY;
	CharaAngle angle;
	int image;
};

static const int PARTY_FIRST_ID = 1000;
static const int MAPCHIP_VISIBLE_W = 8;
static const int MAPCHIP_VISIBLE_H = 10;
static const int PLAYER_W = 64;
static std::string messageText;
//static int playerTex;
static int messageWindowTex;
static int messageWaitTex;
static int mapchipTex;
static const int MAPCHIP_W = 64;
static const int MAPCHIP_H = 64;
static const int SAVEDATA_VERSION = 2;
static int mapScrollX = 0;
static int mapScrollY = 0;
static int mapViewX = 0;
static int mapViewY = 0;
static std::vector<Event *> events;
static int screenW = 1;
static int screenH = 1;
static Interpreter mainInterpreter;
static bool messageShowing = false;
static Interpreter *messageShowInterpreter = NULL;
static CFmfMap map;
static MapInfo *currentMapInfo = NULL;
static MapScript *mapScript = NULL;
static std::vector<ChipSet *> chipSets;
static std::vector<MapInfo *> mapInfos;
static bool isMapChanging = false;
static char nextMapName[FILENAME_MAX];
static int nextMapStartX = 0;
static int nextMapStartY = 0;
static std::vector<int> charaIDAry;
static char speakerName[512] = "";
static std::vector<PartyMember *> partyMembers;
static std::vector<PartyMember *> currentPartyMembers;
static PartyMember *player = NULL;
static bool inBattle = false;

static void GetPartyMemberDestPos(int memberIndex, int *destX, int *destY)
{
	if(memberIndex == 0){
		*destX = currentPartyMembers[memberIndex]->x;
		*destY = currentPartyMembers[memberIndex]->y;
		return;
	}
	*destX = currentPartyMembers[memberIndex - 1]->x;
	if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_LEFT){
		*destX = *destX + MAPCHIP_W;
	}else if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_RIGHT){
		*destX = *destX - MAPCHIP_W;
	}
	*destY = currentPartyMembers[memberIndex - 1]->y;
	if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_UP){
		*destY = *destY + MAPCHIP_H;
	}else if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_DOWN){
		*destY = *destY - MAPCHIP_H;
	}
}

static const char *sgets(char *buf, char **s)
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

static char *LoadChars(const char *filename)
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

void LoadMapInfo()
{
	char filename[] = "mapInfo.txt";
	char *str = LoadChars(filename);
	if(str == NULL){
		printf("failed open event script '%s'\n", filename);
		return;
	}
	std::vector<char *> tokens;
	std::vector<char *> tokens2;
	//std::string expsStr;
	char line[512];
	MapInfo *mapInfo = NULL;
	ChipSet *chipSet = NULL;
	while(sgets(line, &str) != NULL){
		SplitStr(line, " ", &tokens);
		bool isMapInfo = strcmp(tokens[0], "map") == 0;
		bool isChipSet = strcmp(tokens[0], "chipSet") == 0;
		if(isMapInfo || isChipSet){
			//expsStr = "";
			if(isMapInfo){
				mapInfo = new MapInfo;
				mapInfo->chipSet = NULL;
				if(tokens.size() >= 2){
					strcpy(mapInfo->name, tokens[1]);
				}
				strcpy(mapInfo->chipsetName, "");
				mapInfos.push_back(mapInfo);
			}else if(isChipSet){
				chipSet = new ChipSet;
				if(tokens.size() >= 2){
					strcpy(chipSet->name, tokens[1]);
				}
				strcpy(chipSet->imageFilename, "");
				chipSets.push_back(chipSet);
			}
			while(sgets(line, &str) != NULL){
				if(line[0] != '\t' || line[1] == '\0'){
					break;
				}
				SplitStr(line + 1, " ", &tokens2); //最初のタブを飛ばす
				//printf("tokens2[0]=%s\n", tokens2[0]);
				if(isMapInfo){
					if(strcmp(tokens2[0], "chipset") == 0 && tokens2.size() >= 2){
						strcpy(mapInfo->chipsetName, tokens2[1]);
						//printf("chipset=%s\n", tokens2[1]);
					}
				}else if(isChipSet){
					if(strcmp(tokens2[0], "image") == 0 && tokens2.size() >= 2){
						strcpy(chipSet->imageFilename, tokens2[1]);
						//printf("imageFilename=%s\n", tokens2[1]);
					}else if(strcmp(tokens2[0], "impassableIDs") == 0){
						//printf("impassableIDs=");
						for(int i = 1; i < tokens2.size(); i++){
							chipSet->impassableIDs.push_back(atoi(tokens2[i]));
							//printf("%d ", atoi(tokens2[i]));
						}
						//printf("\n");
					}
				}
				DeleteSplitStr(&tokens2);
				//expsStr += (line + 1);
				//expsStr += "\n";
			}
			//printf("expsStr=%s\n", expsStr.c_str());
		}
		DeleteSplitStr(&tokens);
	}
	for(int i = 0; i < mapInfos.size(); i++){
		for(int k = 0; k < chipSets.size(); k++){
			if(strcmp(mapInfos[i]->chipsetName, chipSets[k]->name) == 0){
				mapInfos[i]->chipSet = chipSets[k];
				break;
			}
		}
		if(mapInfos[i]->chipSet == NULL){
			printf("chipset '%s' not found in mapInfo.txt.\n", mapInfos[i]->chipsetName);
			exit(1);
		}
	}
	delete[] str;
}

static void LoadParty()
{
	char filename[] = "party.txt";
	char *str = LoadChars(filename);
	if(str == NULL){
		printf("failed open event script '%s'\n", filename);
		return;
	}
	std::vector<char *> tokens;
	std::vector<char *> tokens2;
	char line[512];
	while(sgets(line, &str) != NULL){
		SplitStr(line, " ", &tokens);
		bool isMember = strcmp(tokens[0], "member") == 0;
		if(isMember){
			PartyMember *member = new PartyMember;
			partyMembers.push_back(member);
			if(tokens.size() >= 2){
				strcpy(member->name, tokens[1]);
			}
			while(sgets(line, &str) != NULL){
				if(line[0] != '\t' || line[1] == '\0'){
					break;
				}
				SplitStr(line + 1, " ", &tokens2); //最初のタブを飛ばす
				if(strcmp(tokens2[0], "image") == 0 && tokens2.size() >= 2){
					strcpy(member->imageFilename, tokens2[1]);
					member->image = LoadTexture(member->imageFilename);
					//printf("image=%s\n", member->imageFilename);
				}
				DeleteSplitStr(&tokens2);
			}
		}
		DeleteSplitStr(&tokens);
	}
	delete[] str;
}

static void UpdateMapScroll()
{
	if(player == NULL) return;
	mapScrollX = player->x - (MAPCHIP_VISIBLE_W - 2) * MAPCHIP_W / 2 - PLAYER_W / 2;
	mapScrollY = player->y - (MAPCHIP_VISIBLE_H - 2) * MAPCHIP_H / 2;
}

static void UpdateEventProperty(Event *event)
{
	int propertIndex = -1;
	int defautlPropertyIndex = -1;
	for(int i = 0; i < event->properties.size(); i++){
		if(event->properties[i]->switchID != -1){
			const char *var = GetVariable(event->properties[i]->switchID);
			if(var == NULL){
				printf("event->properties[i]->switchID=%d\n", event->properties[i]->switchID);
				continue;
			}
			if(var != NULL && atoi(GetVariable(event->properties[i]->switchID)) == 1){
				propertIndex = i;
			}
		}else{
			defautlPropertyIndex = i;
		}
	}
	if(defautlPropertyIndex != -1 && propertIndex == -1){
		propertIndex = defautlPropertyIndex;
	}
	if(propertIndex == -1){
		return;
	}
	if(event->properties[propertIndex]->imageFilename[0] != '\0'){
		event->image = LoadTexture(event->properties[propertIndex]->imageFilename);
	}else{
		event->image = -1;
	}
}

static void ChangeMap(const char *name, int startX, int startY)
{
	if(player == NULL) return;
	map.Close();
	if(mapScript != NULL){
		delete mapScript;
		mapScript = NULL;
	}
	for(int i = 0; i < events.size(); i++){
		delete events[i];
	}
	events.clear();
	char fmfFilename[FILENAME_MAX];
	sprintf(fmfFilename, "%s.fmf", name);
	if(!map.Open(fmfFilename)){
		printf("failed open fmf map '%s'", fmfFilename);
		exit(1);
	}
	currentMapInfo = NULL;
	for(int i = 0; i < mapInfos.size(); i++){
		if(strcmp(mapInfos[i]->name, name) == 0){
			currentMapInfo = mapInfos[i];
			break;
		}
	}
	if(currentMapInfo == NULL){
		printf("map info '%s' not found.\n", name);
		exit(1);
	}
	mapchipTex = LoadTexture(currentMapInfo->chipSet->imageFilename);
	char scriptFilename[FILENAME_MAX];
	sprintf(scriptFilename, "%s.txt", name);
	mapScript = LoadMapScript(scriptFilename);
	if(mapScript == NULL){
		exit(1);
	}
	for(int x = 0; x < map.GetMapWidth(); x++){
		for(int y = 0; y < map.GetMapHeight(); y++){
			if(map.GetValue(1, x, y) != 255 && map.GetValue(1, x, y) != -1){
				Event *event = new Event;
				event->id = map.GetValue(1, x, y) / 16 * 4 + map.GetValue(1, x, y) % 16;
				event->x = x * MAPCHIP_W;
				event->y = y * MAPCHIP_H;
				event->mapX = x;
				event->mapY = y;
				event->onTouchScript = NULL;
				event->onExamineScript = NULL;
				event->image = -1;
				events.push_back(event);
			}
		}
	}
	for(int i = 0; i < events.size(); i++){
		Event *event = events[i];
		for(int k = 0; k < mapScript->eventScripts.size(); k++){
			if(mapScript->eventScripts[k]->id == event->id){
				switch(mapScript->eventScripts[k]->trigger){
				case EV_TRIGGER_TOUCH:
					event->onTouchScript = mapScript->eventScripts[k];
					break;
				case EV_TRIGGER_EXAMINE:
					event->onExamineScript = mapScript->eventScripts[k];
					break;
				}
			}
		}
		for(int k = 0; k < mapScript->eventProperties.size(); k++){
			if(mapScript->eventProperties[k]->id == event->id){
				event->properties.push_back(mapScript->eventProperties[k]);
			}
		}
		UpdateEventProperty(event);
	}
	player->x = startX * MAPCHIP_W;
	player->y = startY * MAPCHIP_H;
	UpdateMapScroll();
	for(int i = 1; i < currentPartyMembers.size(); i++){
		GetPartyMemberDestPos(i, &currentPartyMembers[i]->x, &currentPartyMembers[i]->y);
	}
	charaIDAry.clear();
	for(int i = 0; i < currentPartyMembers.size(); i++){
		charaIDAry.push_back(PARTY_FIRST_ID + i);
	}
	for(int i = 0; i < events.size(); i++){
		charaIDAry.push_back(i);
	}
	for(int i = 0; i < mapScript->eventScripts.size(); i++){
		if(mapScript->eventScripts[i]->trigger == EV_TRIGGER_ENTER){
			Interpreter_Setup(&mainInterpreter, mapScript->eventScripts[i]);
			break;
		}
	}
}

static int DistanceToEvent(int x, int y, int eventIndex)
{
	return (int)(sqrt(pow((double)(events[eventIndex]->x - x), 2.0) + 
		pow((double)(events[eventIndex]->y - y), 2.0)));
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

void OnInterpreterCommand(Interpreter *interpreter, Command command, const char **args)
{
	switch(command){
	case COMMAND_MSG:
		{
			MakeMessageText(args[0]);
			messageShowing = true;
			messageShowInterpreter = interpreter;
			messageShowInterpreter->interreput = true;
		}
		break;
	case COMMAND_SPEAKER:
		strcpy(speakerName, args[0]);
		break;
	case COMMAND_SAVE:
		SaveGame(atoi(args[0]));
		break;
	case COMMAND_LOAD:
		LoadGame(atoi(args[0]));
		break;
	case COMMAND_SET:
		{
			for(int i = 0; i < events.size(); i++){
				UpdateEventProperty(events[i]);
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
			for(int i = 0; i < currentPartyMembers.size(); i++){
				if(strcmp(currentPartyMembers[i]->name, args[0]) == 0){
					printf("party member '%s' aleady exist\n", args[0]);
					return;
				}
			}
			bool memberFound = false;
			for(int i = 0; i < partyMembers.size(); i++){
				if(strcmp(partyMembers[i]->name, args[0]) == 0){
					currentPartyMembers.push_back(partyMembers[i]);
					partyMembers[i]->x = 0;
					partyMembers[i]->y = 0;
					partyMembers[i]->dirX = 0;
					partyMembers[i]->dirY = 0;
					partyMembers[i]->angle = CHARA_ANGLE_DOWN;
					player = currentPartyMembers[0];
					charaIDAry.push_back(PARTY_FIRST_ID + currentPartyMembers.size() - 1);
					GetPartyMemberDestPos(
						currentPartyMembers.size() - 1,
						&currentPartyMembers[currentPartyMembers.size() - 1]->x,
						&currentPartyMembers[currentPartyMembers.size() - 1]->y
					);
					memberFound = true;
					break;
				}
			}
			if(!memberFound){
				printf("party member '%s' not found\n", args[0]);
			}
		}
		break;
	case COMMAND_STAFFROLL:
		exit(0);
		break;
	}
}

static void CheckExamineEvent(CharaAngle angle)
{
	if(player == NULL) return;
	for(int i = 0; i < events.size(); i++){
		Event *event = events[i];
		if((angle == CHARA_ANGLE_LEFT && event->x < player->x  && abs(event->x - player->x) <= MAPCHIP_W * 0.75 && event->mapY == (player->y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(angle == CHARA_ANGLE_RIGHT && event->x > player->x && abs(event->x - player->x) <= MAPCHIP_W * 0.75 && event->mapY == (player->y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(angle == CHARA_ANGLE_UP && event->y < player->y && abs(event->y - player->y) <= MAPCHIP_H * 0.75 && event->mapX == (player->x + (MAPCHIP_W / 2)) / MAPCHIP_W) ||
			(angle == CHARA_ANGLE_DOWN && event->y > player->y && abs(event->y - player->y) <= MAPCHIP_H * 0.75 && event->mapX == (player->x + (MAPCHIP_W / 2)) / MAPCHIP_W)){
			if(event->onExamineScript != NULL){
				Interpreter_Setup(&mainInterpreter, event->onExamineScript);
				return;
			}
		}
	}
}

static void CheckTouchEvent(int dirX, int dirY)
{
	if(player == NULL) return;
	for(int i = 0; i < events.size(); i++){
		Event *event = events[i];
		if((dirX == -1 && event->x < player->x  && abs(event->x - player->x) <= MAPCHIP_W * 0.75 && event->mapY == (player->y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(dirX == 1 && event->x > player->x && abs(event->x - player->x) <= MAPCHIP_W * 0.75 && event->mapY == (player->y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(dirY == -1 && event->y < player->y && abs(event->y - player->y) <= MAPCHIP_H * 0.75 && event->mapX == (player->x + (MAPCHIP_W / 2)) / MAPCHIP_W) ||
			(dirY == 1 && event->y > player->y && abs(event->y - player->y) <= MAPCHIP_H * 0.75 && event->mapX == (player->x + (MAPCHIP_W / 2)) / MAPCHIP_W)){
			if(event->onTouchScript != NULL){
				Interpreter_Setup(&mainInterpreter, event->onTouchScript);
				return;
			}
		}
	}
}

void GameInputKey(InputKey key, InputKeyState state)
{
	if(currentMapInfo == NULL) return;
	if(player == NULL) return;
	int playerNextX = player->x;
	int playerNextY = player->y;
	int playerMapX = player->x / MAPCHIP_W;
	int playerMapY = player->y / MAPCHIP_H;
	if(!mainInterpreter.running){
		if(state == KEY_STATE_PRESS){
			switch(key){
			case KEY_LEFT:
				playerNextX -= 3;
				player->dirX = -1;
				player->angle = CHARA_ANGLE_LEFT;
				CheckTouchEvent(-1, 0);
				break;
			case KEY_RIGHT:
				playerNextX += 3;
				player->dirX = 1;
				player->angle = CHARA_ANGLE_RIGHT;
				CheckTouchEvent(1, 0);
				break;
			case KEY_UP:
				playerNextY -= 3;
				player->dirY = -1;
				player->angle = CHARA_ANGLE_UP;
				CheckTouchEvent(0, -1);
				break;
			case KEY_DOWN:
				playerNextY += 3;
				player->dirY = 1;
				player->angle = CHARA_ANGLE_DOWN;
				CheckTouchEvent(0, 1);
				break;
			}
		}
		int chipID = map.GetValue(0, (playerNextX + MAPCHIP_W / 2) / MAPCHIP_W, (playerNextY + MAPCHIP_H / 2) / MAPCHIP_H);
		for(int i = 0; i < currentMapInfo->chipSet->impassableIDs.size(); i++){
			if(chipID == currentMapInfo->chipSet->impassableIDs[i]){
				playerNextX = player->x;
				playerNextY = player->y;
				break;
			}
		}
		bool touchedToEvent = false;
		for(int i = 0; i < events.size(); i++){
			if(events[i]->image != -1 && DistanceToEvent(playerNextX, playerNextY, i) < MAPCHIP_W / 2){
				touchedToEvent = true;
				break;
			}
		}
		if(!touchedToEvent){
			player->x = playerNextX;
			player->y = playerNextY;
		}
	}
	if(key == KEY_DECIDE &&
		state == KEY_STATE_DOWN &&
		!mainInterpreter.running){
		CheckExamineEvent(player->angle);
	}
	if(messageShowing && key == KEY_DECIDE && state == KEY_STATE_DOWN){
		messageShowInterpreter->interreputEnded = true;
		if(messageShowInterpreter->NextCommand() != COMMAND_MSG){
			messageShowing = false;
			messageText = "";
			messageShowInterpreter = NULL;
		}
	}	
}

static const int CHARA_MOVE_SPEED = 3;

void GameUpdate()
{
	mapViewX = screenW / 2 - MAPCHIP_VISIBLE_W * MAPCHIP_W / 2;
	mapViewY = screenH / 2 - MAPCHIP_VISIBLE_H * MAPCHIP_H / 2;
	
	UpdateMapScroll();
	
	int destX, destY;
	for(int i = 1; i < currentPartyMembers.size(); i++){
		GetPartyMemberDestPos(i, &destX, &destY);
		if(abs(destX - currentPartyMembers[i]->x) >= CHARA_MOVE_SPEED){
			if(destX > currentPartyMembers[i]->x){
				currentPartyMembers[i]->x += CHARA_MOVE_SPEED;
			}else if(destX < currentPartyMembers[i]->x){
				currentPartyMembers[i]->x -= CHARA_MOVE_SPEED;
			}
		}else{
			currentPartyMembers[i]->x = destX;
		}
		if(abs(destY - currentPartyMembers[i]->y) >= CHARA_MOVE_SPEED){
			if(destY > currentPartyMembers[i]->y){
				currentPartyMembers[i]->y += CHARA_MOVE_SPEED;
			}else if(destY < currentPartyMembers[i]->y){
				currentPartyMembers[i]->y -= CHARA_MOVE_SPEED;
			}
		}else{
			currentPartyMembers[i]->y = destY;
		}
	}
	
	Interpreter_Update(&mainInterpreter);
	if(isMapChanging){
		ChangeMap(nextMapName, nextMapStartX, nextMapStartY);
		isMapChanging = false;
	}
}

bool CharaZComp(const int& riLeft, const int& riRight)
{
	int leftZ = 0;
	if(riLeft >= PARTY_FIRST_ID){
		leftZ = currentPartyMembers[riLeft - PARTY_FIRST_ID]->y;
	}else{
		leftZ = events[riLeft]->y;
	}
	int rightZ = 0;
	if(riRight >= PARTY_FIRST_ID){
		rightZ = currentPartyMembers[riRight - PARTY_FIRST_ID]->y;
	}else{
		rightZ = events[riRight]->y;
	}
    return leftZ < rightZ;
}

void GameRender()
{
	if(player == NULL) return;

	SetViewport(
		mapViewX + MAPCHIP_W,
		mapViewY + MAPCHIP_H, 
		(MAPCHIP_VISIBLE_W - 2) * MAPCHIP_W,
		(MAPCHIP_VISIBLE_H - 2) * MAPCHIP_H
	);

	for(int y = 0; y < MAPCHIP_VISIBLE_H; y++){
		for(int x = 0; x < MAPCHIP_VISIBLE_W; x++){
			int mapchipX = x + mapScrollX / MAPCHIP_W;
			if(mapchipX >= map.GetMapWidth()){
				mapchipX = map.GetMapWidth() - 1;
			}
			if(mapchipX < 0){
				mapchipX = 0;
			}
			int mapchipY = y + mapScrollY / MAPCHIP_H;
			if(mapchipY >= map.GetMapHeight()){
				mapchipY = map.GetMapHeight() - 1;
			}
			if(mapchipY < 0){
				mapchipY = 0;
			}
			int chipID = map.GetValue(0, mapchipX, mapchipY);
			chipID = chipID / 16 * 4 + chipID % 16;
			DrawRectTexture(
				mapchipTex,
				x * MAPCHIP_W - mapScrollX % MAPCHIP_W + mapViewX,
				y * MAPCHIP_H - mapScrollY % MAPCHIP_H + mapViewY,
				(chipID % 4) * MAPCHIP_W,
				(chipID / 4) * MAPCHIP_H,
				MAPCHIP_W,
				MAPCHIP_H
			);
		}
	}
	
	//int rectX = (player->x + MAPCHIP_W / 2) / MAPCHIP_W * MAPCHIP_W - mapScrollX + mapViewX;
	//int rextY = (player->y + MAPCHIP_H / 2) / MAPCHIP_H * MAPCHIP_H - mapScrollY + mapViewY;
	//DrawBox(rectX, rextY, rectX + MAPCHIP_W, rextY + MAPCHIP_H, GetColor(0, 0, 255), TRUE);
	std::sort(charaIDAry.begin(), charaIDAry.end(), CharaZComp);
	for(int i = 0; i < charaIDAry.size(); i++){
		if(charaIDAry[i] >= PARTY_FIRST_ID){
			PartyMember *member = currentPartyMembers[charaIDAry[i] - PARTY_FIRST_ID];
			DrawTexture(
				member->image,
				-mapScrollX + member->x + mapViewX,
				-mapScrollY + member->y + mapViewY
			);
		}else{
			Event *event = events[charaIDAry[i]];
			if(event->image != -1){
				DrawTexture(
					event->image,
					-mapScrollX + event->x + mapViewX,
					-mapScrollY + event->y + mapViewY
				);
			}
		}
	}
	/*
	DrawTexture(
		playerTex,
		-mapScrollX + player->x + mapViewX,
		-mapScrollY + player->y + mapViewY
	);
	for(int i = 0; i < events.size(); i++){
		if(events[i]->image != -1){
			DrawTexture(
				events[i]->image,
				-mapScrollX + events[i]->x + mapViewX,
				-mapScrollY + events[i]->y + mapViewY
			);
		}
	}
	*/
	SetViewport(
		0, 0, screenW, screenH
	);
	
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
}

void GameInit()
{
	events.clear();
	
	//playerTex = LoadTexture("man.tga");
	messageWindowTex = LoadTexture("msgWin.tga");
	messageWaitTex = LoadTexture("msgWait.tga");

	messageText = "";
	
	/*
	MakeEvent(
		2, 0, 
		"msg \"変数の値は\\v0なのである\";"
		"msg \"主人公よ。\";"
		"set 0 9999;"
		"save 0;"
	);
	*/
	
	Interpreter_Init(&mainInterpreter);
	
	InitEventGlobal();
	
	LoadMapInfo();
	LoadParty();
	
	LoadGame(0);
	
	EventScript *script = LoadEventScript("init.txt");
	if(script == NULL){
		exit(1);
	}
	Interpreter_Setup(&mainInterpreter, script);
	Interpreter_Update(&mainInterpreter);
}

void GameScreenResized(int width, int height)
{
	screenW = width;
	screenH = height;
}

void GameDestroy()
{
	for(int i = 0; i < events.size(); i++){
		delete events[i];
	}
	events.clear();
	for(int i = 0; i < mapInfos.size(); i++){
		delete mapInfos[i];
	}
	mapInfos.clear();
	for(int i = 0; i < partyMembers.size(); i++){
		delete partyMembers[i];
	}
	partyMembers.clear();
	currentPartyMembers.clear();
	for(int i = 0; i < chipSets.size(); i++){
		delete chipSets[i];
	}
	chipSets.clear();
}