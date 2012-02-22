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
//�ǉ������p�[�e�B�����o�̔ԍ���Ԃ�
//�ǉ��Ɏ��s������-1��Ԃ�
int AddPartyMember(const char *name);
//�p�[�e�B�ɓ����Ă��郁���o�[��Ԃ�
PartyMember *GetPartyMember(int index);
//
int GetPartyMemberCount();

#endif