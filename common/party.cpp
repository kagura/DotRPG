#include "party.h"
#include "util.h"
#include "gamefunc.h"

static std::vector<PartyMember *> partyMembers;
static std::vector<PartyMember *> currentPartyMembers;
static PartyMember *player = NULL;

void LoadParty()
{
	char filename[] = "party.txt";
	char *str = LoadChars(filename);
	if(str == NULL){
		printf("failed open event script '%s'\n", filename);
		return;
	}
	std::vector<char *> tokens;
	std::vector<char *> tokens2;
	char line[512];
	while(sgets(line, &str) != NULL){
		SplitStr(line, " ", &tokens);
		bool isMember = strcmp(tokens[0], "member") == 0;
		if(isMember){
			PartyMember *member = new PartyMember;
			member->battler.maxHp = member->battler.hp = 100;
			member->battler.maxMp = member->battler.mp = 30;
			member->battler.offense = 10;
			member->battler.speed = 10;
			member->skillIDs.push_back(0);
			member->skillIDs.push_back(1);
			member->skillIDs.push_back(2);
			partyMembers.push_back(member);
			if(tokens.size() >= 2){
				strcpy(member->name, tokens[1]);
				strcpy(member->battler.name, tokens[1]);
			}
			while(sgets(line, &str) != NULL){
				if(line[0] != '\t' || line[1] == '\0'){
					break;
				}
				SplitStr(line + 1, " ", &tokens2); //Å‰‚Ìƒ^ƒu‚ð”ò‚Î‚·
				if(strcmp(tokens2[0], "image") == 0 && tokens2.size() >= 2){
					strcpy(member->imageFilename, tokens2[1]);
					member->image = LoadTexture(member->imageFilename);
				}
				DeleteSplitStr(&tokens2);
			}
		}
		DeleteSplitStr(&tokens);
	}
	delete[] str;
}

void CleanupParty()
{
	for(int i = 0; i < partyMembers.size(); i++){
		delete partyMembers[i];
	}
	partyMembers.clear();
	currentPartyMembers.clear();
}

PartyMember *GetPlayer()
{
	return player;
}

void SetPlayer(int partyMemberIndex)
{
	player = GetPartyMember(partyMemberIndex);
}

void GetPartyMemberDestPos(int memberIndex, int *destX, int *destY)
{
	if(memberIndex == 0){
		*destX = currentPartyMembers[memberIndex]->x;
		*destY = currentPartyMembers[memberIndex]->y;
		return;
	}
	*destX = currentPartyMembers[memberIndex - 1]->x;
	if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_LEFT){
		*destX = *destX + MAPCHIP_W;
	}else if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_RIGHT){
		*destX = *destX - MAPCHIP_W;
	}
	*destY = currentPartyMembers[memberIndex - 1]->y;
	if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_UP){
		*destY = *destY + MAPCHIP_H;
	}else if(currentPartyMembers[memberIndex - 1]->angle == CHARA_ANGLE_DOWN){
		*destY = *destY - MAPCHIP_H;
	}
}

int AddPartyMember(const char *name)
{
	for(int i = 0; i < currentPartyMembers.size(); i++){
		if(strcmp(currentPartyMembers[i]->name, name) == 0){
			printf("party member '%s' aleady exist\n", name);
			return -1;
		}
	}
	for(int i = 0; i < partyMembers.size(); i++){
		if(strcmp(partyMembers[i]->name, name) == 0){
			currentPartyMembers.push_back(partyMembers[i]);
			partyMembers[i]->x = 0;
			partyMembers[i]->y = 0;
			partyMembers[i]->dirX = 0;
			partyMembers[i]->dirY = 0;
			partyMembers[i]->angle = CHARA_ANGLE_DOWN;
			return currentPartyMembers.size() - 1;
		}
	}
	printf("party member '%s' not found\n", name);
	return -1;
}

PartyMember *GetPartyMember(int index)
{
	if(index < 0 || index >= currentPartyMembers.size()) return NULL;
	return currentPartyMembers[index];
}

int GetPartyMemberCount()
{
	return currentPartyMembers.size();
}