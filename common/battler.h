#ifndef BATTLER_H
#define BATTLER_H

struct Battler
{
	char name[128];
	int maxHp;
	int maxMp;
	int hp;
	int mp;
	int offense; //�U����
	int speed;
};

#endif