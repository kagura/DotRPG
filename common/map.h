#ifndef MAP_H
#define MAP_H

#include "script.h"

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

const int MAPCHIP_W = 64;
const int MAPCHIP_H = 64;

void LoadMapInfo();
void CleanupMapInfo();

void Map_Setup(const char *name, MapScript *mapScript);
void Map_Cleanup();
void Map_SetScroll(int x, int y);

int GetMapScrollX();
int GetMapScrollY();
void DrawUnderMap(int mapViewX, int mapViewY, int visibleMapChipXCount, int visibleMapChipYCount);
int GetChipIDAtPos(int mapX, int mapY);
bool IsChipIDPassable(int chipID);

void UpdateEventProperty(Event *event);
int DistanceToEvent(int x, int y, int eventIndex);
//x, y はピクセル座標
Event *CheckExamineEvent(int x, int y, CharaAngle angle);
//x, y はピクセル座標
Event *CheckTouchEvent(int x, int y, int dirX, int dirY);
int GetEventsCount();
Event *GetEvent(int index);

#endif