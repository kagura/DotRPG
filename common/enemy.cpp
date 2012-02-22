#include "enemy.h"
#include "gamefunc.h"
#include "picojson.h"
#include "gamefunc.h"
#include "util.h"

static std::vector<Enemy *> enemies;

void LoadEnemy()
{
	char *chars = LoadChars("enemy.json");
	if(chars == NULL){
		printf("failed load enemy.json\n");
		return;
	}
	picojson::value v;
    std::string err = picojson::parse(v, chars, chars + strlen(chars));
	if (err.empty()) {
		picojson::array arr = v.get<picojson::array>();
		picojson::array::iterator it;
		for (it = arr.begin(); it != arr.end(); it++) {
			picojson::object obj = it->get<picojson::object>();
			Enemy *enemy = new Enemy;
			strncpy(enemy->battler.name, obj["name"].get<std::string>().c_str(), 128);
			enemy->battler.hp = enemy->battler.maxHp = (int)obj["maxHp"].get<double>();
			enemy->battler.mp = enemy->battler.maxMp = (int)obj["maxMp"].get<double>();
			enemy->battler.offense = (int)obj["offense"].get<double>();
			enemy->battler.speed = (int)obj["speed"].get<double>();
			strncpy(enemy->imageFilename, obj["image"].get<std::string>().c_str(), FILENAME_MAX);
			picojson::array actionArr = obj["actions"].get<picojson::array>();
			picojson::array::iterator actionArrIt;
			for (actionArrIt = actionArr.begin(); actionArrIt != actionArr.end(); actionArrIt++) {
				picojson::object actionObj = actionArrIt->get<picojson::object>();
				EnemyAction *action = new EnemyAction;
				if(actionObj["start_turn"].is<double>()){
					action->startTurn = (int)actionObj["start_turn"].get<double>();
				}else{
					action->startTurn = 0;
				}
				if(actionObj["every_turn"].is<double>()){
					action->everyTurn = (int)actionObj["every_turn"].get<double>();
				}else{
					action->everyTurn = 1;
				}
				if(actionObj["switch"].is<double>()){
					action->switchID = (int)actionObj["switch"].get<double>();
				}else{
					action->switchID = -1;
				}
				if(actionObj["hp_under_percent"].is<double>()){
					action->hpUnderPercent = (int)actionObj["hp_under_percent"].get<double>();
				}else{
					action->hpUnderPercent = 100;
				}
				if(actionObj["base_action"].is<double>()){
					action->baseAction = (int)actionObj["base_action"].get<double>();
				}else{
					action->baseAction = 0;
				}
				if(actionObj["skill"].is<double>()){
					action->skillID = (int)actionObj["skill"].get<double>();
				}else{
					action->skillID = -1;
				}
				if(actionObj["rating"].is<double>()){
					action->rating = (int)actionObj["rating"].get<double>();
				}else{
					action->rating = -1;
				}
				if(action->rating > 5){
					action->rating = 5;
				}else if(action->rating < 1){
					action->rating = 1;
				}
				enemy->actions.push_back(action);
			}
			enemy->image = LoadTexture(enemy->imageFilename);
			enemies.push_back(enemy);
		}
	}else{
		char text[512];
		sprintf(text, "File:%s\nfailed parse json: %s", "enemy.json", err.c_str());
		ErrorMessage(text);
	}
}

void CleanupEnemy()
{
	for(int i = 0; i < enemies.size(); i++){
		for(int j = 0; j < enemies[i]->actions.size(); j++){
			delete enemies[i]->actions[j];
		}
		delete enemies[i];
	}
	enemies.clear();
}

Enemy *GetEnemy(int enemyNo)
{
	if(enemyNo < 0 || enemyNo >= enemies.size()){
		return NULL;
	}
	return enemies[enemyNo];
}