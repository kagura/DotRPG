#ifndef GLOBAL_H
#define GLOBAL_H

enum Scope
{
	SCOPE_ONE_ENEMY,
	SCOPE_ONE_FRIEND,
	SCOPE_ALL_ENEMY,
	SCOPE_ALL_FRIEND,
	SCOPE_ALL,
	SCOPE_MAX,
};

struct Item
{
	char name[128];
	char description[512];
	Scope scope;
	int hpVary; //hp�̑�����
	int mpVary; //mp�̑�����
	bool recoveryDead; //�퓬�s�\�����
};

struct Skill
{
	char name[128];
	char description[512];
	Scope scope;
	int damage;
	int decreaseMP; //����MP
};

int GetScreenWidth();
int GetScreenHeight();

#endif