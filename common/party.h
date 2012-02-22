#ifndef PARTY_H
#define PARTY_H

#include <stdio.h>
#include "map.h"
#include "battler.h"

struct PartyMember
{
	Battler battler;
	char name[FILENAME_MAX];
	char imageFilename[FILENAME_MAX];
	int x, y;
	int dirX, dirY;
	CharaAngle angle;
	int image;
	std::vector<int> skillIDs;
};

void LoadParty();
void CleanupParty();
PartyMember *GetPlayer();
void SetPlayer(int partyMemberIndex);
void GetPartyMemberDestPos(int memberIndex, int *destX, int *destY);
//追加したパーティメンバの番号を返す
//追加に失敗したら-1を返す
int AddPartyMember(const char *name);
//パーティに入っているメンバーを返す
PartyMember *GetPartyMember(int index);
//
int GetPartyMemberCount();

#endif