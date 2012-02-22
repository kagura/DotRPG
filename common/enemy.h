#ifndef ENEMY_H
#define ENEMY_H

#include "battler.h"
#include <stdio.h>
#include <vector>

struct EnemyAction
{
	int startTurn;
	int everyTurn;
	int hpUnderPercent;
	int switchID;
	int rating;
	int baseAction;
	int skillID;
};

struct Enemy
{
	Battler battler;
	char imageFilename[FILENAME_MAX];
	int image;
	std::vector<EnemyAction *> actions; 
};

void LoadEnemy();
void CleanupEnemy();
Enemy *GetEnemy(int enemyNo);

#endif