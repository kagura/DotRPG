#include "map.h"
#include "util.h"
#include "fmfmap.h"
#include "gamefunc.h"
#include <math.h>
#include <vector>

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

static std::vector<ChipSet *> chipSets;
static std::vector<MapInfo *> mapInfos;
static CFmfMap map;
static int mapchipTex;
static MapInfo *currentMapInfo = NULL;
static std::vector<Event *> events;
static int scrollX = 0;
static int scrollY = 0;

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
				SplitStr(line + 1, " ", &tokens2); //Å‰‚Ìƒ^ƒu‚ð”ò‚Î‚·
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

void CleanupMapInfo()
{
	for(int i = 0; i < mapInfos.size(); i++){
		delete mapInfos[i];
	}
	mapInfos.clear();
	for(int i = 0; i < chipSets.size(); i++){
		delete chipSets[i];
	}
	chipSets.clear();
}

void Map_Setup(const char *name, MapScript *mapScript)
{
	Map_Cleanup();
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
}

void Map_Cleanup()
{
	for(int i = 0; i < events.size(); i++){
		delete events[i];
	}
	events.clear();
	map.Close();
}

void Map_SetScroll(int x, int y)
{
	scrollX = x;
	scrollY = y;
}

int GetMapScrollX()
{
	return scrollX;
}

int GetMapScrollY()
{
	return scrollY;
}

void DrawUnderMap(int mapViewX, int mapViewY, int visibleMapChipXCount, int visibleMapChipYCount)
{
	for(int y = 0; y < visibleMapChipYCount; y++){
		for(int x = 0; x < visibleMapChipXCount; x++){
			int mapchipX = x + scrollX / MAPCHIP_W;
			if(mapchipX >= map.GetMapWidth()){
				mapchipX = map.GetMapWidth() - 1;
			}
			if(mapchipX < 0){
				mapchipX = 0;
			}
			int mapchipY = y + scrollY / MAPCHIP_H;
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
				x * MAPCHIP_W - scrollX % MAPCHIP_W + mapViewX,
				y * MAPCHIP_H - scrollY % MAPCHIP_H + mapViewY,
				(chipID % 4) * MAPCHIP_W,
				(chipID / 4) * MAPCHIP_H,
				MAPCHIP_W,
				MAPCHIP_H
			);
		}
	}
}

int GetChipIDAtPos(int mapX, int mapY)
{
	return map.GetValue(0, mapX, mapY);
}

bool IsChipIDPassable(int chipID)
{
	if(currentMapInfo == NULL) return false;
	for(int i = 0; i < currentMapInfo->chipSet->impassableIDs.size(); i++){
		if(chipID == currentMapInfo->chipSet->impassableIDs[i]){
			return false;
		}
	}
	return true;
}

void UpdateEventProperty(Event *event)
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

int DistanceToEvent(int x, int y, int eventIndex)
{
	return (int)(sqrt(pow((double)(events[eventIndex]->x - x), 2.0) + 
		pow((double)(events[eventIndex]->y - y), 2.0)));
}

Event *CheckExamineEvent(int x, int y, CharaAngle angle)
{
	for(int i = 0; i < events.size(); i++){
		Event *event = events[i];
		if((angle == CHARA_ANGLE_LEFT && event->x < x  && abs(event->x - x) <= MAPCHIP_W * 0.75 && event->mapY == (y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(angle == CHARA_ANGLE_RIGHT && event->x > x && abs(event->x - x) <= MAPCHIP_W * 0.75 && event->mapY == (y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(angle == CHARA_ANGLE_UP && event->y < y && abs(event->y - y) <= MAPCHIP_H * 0.75 && event->mapX == (x + (MAPCHIP_W / 2)) / MAPCHIP_W) ||
			(angle == CHARA_ANGLE_DOWN && event->y > y && abs(event->y - y) <= MAPCHIP_H * 0.75 && event->mapX == (x + (MAPCHIP_W / 2)) / MAPCHIP_W)){
			if(event->onExamineScript != NULL){
				return event;
			}
		}
	}
	return NULL;
}

Event *CheckTouchEvent(int x, int y, int dirX, int dirY)
{
	for(int i = 0; i < events.size(); i++){
		Event *event = events[i];
		if((dirX == -1 && event->x < x  && abs(event->x - x) <= MAPCHIP_W * 0.75 && event->mapY == (y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(dirX == 1 && event->x > x && abs(event->x - x) <= MAPCHIP_W * 0.75 && event->mapY == (y + (MAPCHIP_H / 2)) / MAPCHIP_H) ||
			(dirY == -1 && event->y < y && abs(event->y - y) <= MAPCHIP_H * 0.75 && event->mapX == (x + (MAPCHIP_W / 2)) / MAPCHIP_W) ||
			(dirY == 1 && event->y > y && abs(event->y - y) <= MAPCHIP_H * 0.75 && event->mapX == (x + (MAPCHIP_W / 2)) / MAPCHIP_W)){
			if(event->onTouchScript != NULL){
				return event;
			}
		}
	}
	return NULL;
}

int GetEventsCount()
{
	return events.size();
}

Event *GetEvent(int index)
{
	if(index < 0 || index >= events.size()) return NULL;
	return events[index];
}