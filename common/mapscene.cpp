#include "mapscene.h"
#include "party.h"

void MapScene_InputKey(InputKey key, InputKeyState state)
{
	if(GetPlayer() == NULL) return;
	if(GetCurrentInterpreter() == NULL) return;
	PartyMember *player = GetPlayer();
	Interpreter *interpreter = GetCurrentInterpreter();
	int playerNextX = player->x;
	int playerNextY = player->y;
	int playerMapX = player->x / MAPCHIP_W;
	int playerMapY = player->y / MAPCHIP_H;
	if(!interpreter->running){
		Event *touchedEvent = NULL;
		if(state == KEY_STATE_PRESS){
			switch(key){
			case KEY_LEFT:
				playerNextX -= 3;
				player->dirX = -1;
				player->angle = CHARA_ANGLE_LEFT;
				touchedEvent = CheckTouchEvent(player->x, player->y, -1, 0);
				break;
			case KEY_RIGHT:
				playerNextX += 3;
				player->dirX = 1;
				player->angle = CHARA_ANGLE_RIGHT;
				touchedEvent = CheckTouchEvent(player->x, player->y, 1, 0);
				break;
			case KEY_UP:
				playerNextY -= 3;
				player->dirY = -1;
				player->angle = CHARA_ANGLE_UP;
				touchedEvent = CheckTouchEvent(player->x, player->y, 0, -1);
				break;
			case KEY_DOWN:
				playerNextY += 3;
				player->dirY = 1;
				player->angle = CHARA_ANGLE_DOWN;
				touchedEvent = CheckTouchEvent(player->x, player->y, 0, 1);
				break;
			}
		}
		if(touchedEvent != NULL){
			Interpreter_Setup(interpreter, touchedEvent->onTouchScript);
		}
		int chipID = GetChipIDAtPos((playerNextX + MAPCHIP_W / 2) / MAPCHIP_W, (playerNextY + MAPCHIP_H / 2) / MAPCHIP_H);
		if(!IsChipIDPassable(chipID)){
			playerNextX = player->x;
			playerNextY = player->y;
		}
		bool touchedToEvent = false;
		for(int i = 0; i < GetEventsCount(); i++){
			if(GetEvent(i)->image != -1 && DistanceToEvent(playerNextX, playerNextY, i) < MAPCHIP_W / 2){
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
		!interpreter->running){
		Event *event = CheckExamineEvent(player->x, player->y, player->angle);
		if(event != NULL){
			Interpreter_Setup(interpreter, event->onExamineScript);
		}
	}
}

static const int CHARA_MOVE_SPEED = 3;

void MapScene_Update()
{
	int destX, destY;
	for(int i = 1; i < GetPartyMemberCount(); i++){
		PartyMember *member = GetPartyMember(i);
		GetPartyMemberDestPos(i, &destX, &destY);
		if(abs(destX - member->x) >= CHARA_MOVE_SPEED){
			if(destX > member->x){
				member->x += CHARA_MOVE_SPEED;
			}else if(destX < member->x){
				member->x -= CHARA_MOVE_SPEED;
			}
		}else{
			member->x = destX;
		}
		if(abs(destY - member->y) >= CHARA_MOVE_SPEED){
			if(destY > member->y){
				member->y += CHARA_MOVE_SPEED;
			}else if(destY < member->y){
				member->y -= CHARA_MOVE_SPEED;
			}
		}else{
			member->y = destY;
		}
	}
}