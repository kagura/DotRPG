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
	int hpVary; //hpÌ¸Ê
	int mpVary; //mpÌ¸Ê
	bool recoveryDead; //í¬s\©çñ
};

struct Skill
{
	char name[128];
	char description[512];
	Scope scope;
	int damage;
	int decreaseMP; //ÁïMP
};

int GetScreenWidth();
int GetScreenHeight();

#endif