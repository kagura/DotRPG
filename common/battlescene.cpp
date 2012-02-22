#include "battlescene.h"
#include "party.h"
#include "global.h"
#include "enemy.h"
#include "gamefunc.h"
#include "database.h"
#include <vector>
#include <stack>
#include <math.h>
#include <assert.h>

enum BattleAction
{
	BA_UNKNOWN,
	BA_ATTACK,
	BA_SKILL,
	BA_ITEM,
	BA_ESCAPE,
};

enum BattlePhase
{
	BP_STARTING,
	BP_COMMAND_CHOICE,
	BP_SKILL_CHOICE,
	BP_ENEMY_CHOICE,
	BP_FRIEND_CHOICE,
	BP_ACTION_ANIME,
	BP_ACTION_APPLY,
	BP_WIN,
	BP_DEFEAT,
};

enum ApplyAnimePhase
{
	APPLY_ANIME_PHASE_DAMAGE,
	APPLY_ANIME_PHASE_DEAD
};

enum BattleResult
{
	BATTLE_RESULT_UNKNOWN,
	BATTLE_RESULT_WIN,
	BATTLE_RESULT_DEFEAT,
};

struct EnemyView
{
	bool visible;
};

struct PartyStateWindow
{
	bool damaged;
	int damagedCount;
};

static const int PARTY_MIN_ID = 1000;
static const int SKILL_CHOICE_XSIZE = 2;
static const int SKILL_CHOICE_YSIZE = 3;
static const char *commandChoiceTexts[] = {
	"こうげき",
	"特技",
	"アイテム",
	"逃げる"
};
static const int PARTY_DAMAGE_ANIME_DURATION = 5;

static BattlePhase phase = BP_COMMAND_CHOICE;
static int commandChoiceIndex = 0;
static char helpText[512] = "";
static BattleAction action = BA_UNKNOWN; 
static int phaseCount = 0;
static std::vector<Battler> enemyBattlers;
static std::vector<int> enemyNoList;
static std::vector<EnemyView> enemyViews; //添え字はorderNo
static std::vector<int> orders;
static std::vector<int> receivers; //攻撃を受けるバトラーのorderID
static std::vector<PartyStateWindow> partyStateWindows;
static int orderIndex = 0;
static int selectedSkillIndex = 0; //パーティのスキル選択のとき使用
static int selectedSkillID = 0;
static int currentReceiverIndex = 0;
static BattleResult battleResult = BATTLE_RESULT_UNKNOWN;
static int selectedEnemyIndex = 0;
static int enemyChoiceIconTex = 0;
static int applyAnimeCount = 0; //ダメージ(または回復)アニメのカウント
static int selectedFriendIndex = 0;
static ApplyAnimePhase applyAnimePhase = APPLY_ANIME_PHASE_DAMAGE;
static int currentDamage = 0;
static int turnCount = 0;

static void ChangePhase(BattlePhase nextPhase);
static Battler *GetCurrentBattler();
static Battler *GetBattler(int orderNo);
static Enemy *GetCurrentEnemy();
static PartyMember *GetCurrentPartyMember();
static Skill *GetSelectedSkill();
static Skill *GetPartySelectedSkill();
static void SortOrder();
static void ApplyAction(int orderNo);
static void SetApplyActionHelp(int orderNo);
static void StartDamagedAnime(int orderNo);
static void Judge();
static bool IsOrderNoEnemy(int ordeNo);
static void NextTurn();

void AddBattleEnemy(int enemyNo)
{
	enemyBattlers.push_back(GetEnemy(enemyNo)->battler);
	Battler *battler = &enemyBattlers[enemyBattlers.size() - 1];
	battler->hp = battler->maxHp;
	battler->mp = battler->maxMp;
	enemyNoList.push_back(enemyNo);
	EnemyView enemyView;
	enemyView.visible = true;
	enemyViews.push_back(enemyView);
	orders.push_back(enemyNoList.size() - 1);
	int sameKindEnemyNum = 0;
	for(int i = 0; i < enemyNoList.size() - 1; i++){
		if(enemyNoList[i] == enemyNo){
			sameKindEnemyNum++;
		}
	}
	char nameExt[128];
	sprintf(nameExt, " %c", 'A' + sameKindEnemyNum);
	strcat(battler->name, nameExt);
}

static void BattleScene_Exit()
{
	DeleteTexture(enemyChoiceIconTex);
	ResumeInterpreter();
	enemyBattlers.clear();
	enemyNoList.clear();
	enemyViews.clear();
	orders.clear();
	partyStateWindows.clear();
	receivers.clear();
	PopScene();
}

void BattleScene_Init()
{
	orderIndex = 0;
	commandChoiceIndex = 0;
	selectedSkillIndex = 0;
	selectedSkillID = 0;
	selectedEnemyIndex = 0;
	applyAnimeCount = 0;
	selectedFriendIndex = 0;
	currentDamage = 0;
	turnCount = 0;
	enemyChoiceIconTex = LoadTexture("enemyChoiceIcon.tga");
	CloseMessageWindow();
	PauseInterpreter();
	strcpy(helpText, "");
	for(int i = 0; i < GetPartyMemberCount(); i++){
		orders.push_back(PARTY_MIN_ID + i);
		PartyStateWindow partyStateWin;
		partyStateWin.damaged = false;
		partyStateWin.damagedCount = 0;
		partyStateWindows.push_back(partyStateWin);
	}
	SortOrder();
	for(int i = 0; i < orders.size(); i++){
		printf("%s が戦闘に参加 speed=%d\n", GetBattler(orders[i])->name, GetBattler(orders[i])->speed);
	}
	ChangePhase(BP_STARTING);
}

void BattleScene_InputKey(InputKey key, InputKeyState state)
{
	switch(phase){
	case BP_COMMAND_CHOICE:
		if(state == KEY_STATE_DOWN){
			switch(key){
			case KEY_RIGHT:
				commandChoiceIndex++;
				if(commandChoiceIndex >= 4) commandChoiceIndex = 3;
				break;
			case KEY_LEFT:
				commandChoiceIndex--;
				if(commandChoiceIndex < 0) commandChoiceIndex = 0;
				break;
			case KEY_DECIDE:
				switch(commandChoiceIndex){
				case 0:
					action = BA_ATTACK;
					ChangePhase(BP_ENEMY_CHOICE);
					break;
				case 1:
					action = BA_SKILL;
					ChangePhase(BP_SKILL_CHOICE);
					break;
				case 2:
					//action = BA_ITEM;
					//ChangePhase(BP_ACTION_APPLY);
					break;
				case 3:
					BattleScene_Exit();
					break;
				}
				break;
			}
		}
		break;
	case BP_SKILL_CHOICE:
		if(state == KEY_STATE_DOWN){
			PartyMember *member = GetCurrentPartyMember();
			bool skillSelectChanged = false;
			switch(key){
			case KEY_RIGHT:
				selectedSkillIndex += SKILL_CHOICE_YSIZE;
				if(selectedSkillIndex >= SKILL_CHOICE_XSIZE * SKILL_CHOICE_YSIZE){
					selectedSkillIndex %= SKILL_CHOICE_YSIZE;
				}
				skillSelectChanged = true;
				break;
			case KEY_LEFT:
				selectedSkillIndex -= SKILL_CHOICE_YSIZE;
				if(selectedSkillIndex < 0){
					selectedSkillIndex = SKILL_CHOICE_YSIZE + selectedSkillIndex;
				}
				skillSelectChanged = true;
				break;
			case KEY_UP:
				selectedSkillIndex -= 1;
				if(selectedSkillIndex < 0){
					selectedSkillIndex = SKILL_CHOICE_XSIZE * SKILL_CHOICE_YSIZE - 1;
				}
				skillSelectChanged = true;
				break;
			case KEY_DOWN:
				selectedSkillIndex += 1;
				if(selectedSkillIndex >= SKILL_CHOICE_XSIZE * SKILL_CHOICE_YSIZE){
					selectedSkillIndex = 0;
				}
				skillSelectChanged = true;
				break;
			case KEY_DECIDE:
				if(GetPartySelectedSkill() != NULL && GetCurrentBattler()->mp >= GetPartySelectedSkill()->decreaseMP){
					selectedSkillID = GetCurrentPartyMember()->skillIDs[selectedSkillIndex];
					switch(GetPartySelectedSkill()->scope){
					case SCOPE_ONE_ENEMY:
						ChangePhase(BP_ENEMY_CHOICE);
						break;
					case SCOPE_ONE_FRIEND:
						ChangePhase(BP_FRIEND_CHOICE);
						break;
					case SCOPE_ALL_ENEMY:
						{
							for(int i = 0; i < enemyBattlers.size(); i++){
								if(enemyBattlers[i].hp != 0){
									receivers.push_back(i);
								}
							}
							ChangePhase(BP_ACTION_ANIME);
						}
						break;
					case SCOPE_ALL_FRIEND:
						{
							for(int i = 0; i < GetPartyMemberCount(); i++){
								if(GetPartyMember(i)->battler.hp != 0){
									receivers.push_back(i + PARTY_MIN_ID);
								}
							}
							ChangePhase(BP_ACTION_ANIME);
						}
						break;
					}
				}
				break;
			case KEY_CANCEL:
				ChangePhase(BP_COMMAND_CHOICE);
				break;
			}
			if(skillSelectChanged){
				if(GetPartySelectedSkill() != NULL){
					strcpy(helpText, GetPartySelectedSkill()->description);
				}else{
					strcpy(helpText, "");
				}
			}
		}
		break;
	case BP_ENEMY_CHOICE:
		if(state == KEY_STATE_DOWN){
			switch(key){
			case KEY_RIGHT:
				selectedEnemyIndex++;
				if(selectedEnemyIndex >= enemyBattlers.size()){
					selectedEnemyIndex = 0;
				}
				while(enemyBattlers[selectedEnemyIndex].hp == 0){
					selectedEnemyIndex++;
					if(selectedEnemyIndex >= enemyBattlers.size()){
						selectedEnemyIndex = 0;
					}
				}
				strcpy(helpText, enemyBattlers[selectedEnemyIndex].name);
				break;
			case KEY_LEFT:
				selectedEnemyIndex--;
				if(selectedEnemyIndex < 0){
					selectedEnemyIndex = enemyBattlers.size() - 1;
				}
				while(enemyBattlers[selectedEnemyIndex].hp == 0){
					selectedEnemyIndex--;
					if(selectedEnemyIndex < 0){
						selectedEnemyIndex = enemyBattlers.size() - 1;
					}
				}
				strcpy(helpText, enemyBattlers[selectedEnemyIndex].name);
				break;
			case KEY_DECIDE:
				receivers.push_back(selectedEnemyIndex);
				ChangePhase(BP_ACTION_ANIME);
				break;
			case KEY_CANCEL:
				switch(action){
				case BA_ATTACK:
					ChangePhase(BP_COMMAND_CHOICE);
					break;
				case BA_SKILL:
					ChangePhase(BP_SKILL_CHOICE);
					break;
				}
				break;
			}
		}
		break;
	case BP_FRIEND_CHOICE:
		if(state == KEY_STATE_DOWN){
			switch(key){
			case KEY_RIGHT:
				selectedFriendIndex++;
				if(selectedFriendIndex >= GetPartyMemberCount()){
					selectedFriendIndex = 0;
				}
				while(GetPartyMember(selectedFriendIndex)->battler.hp == 0){
					selectedFriendIndex++;
					if(selectedFriendIndex >= GetPartyMemberCount()){
						selectedFriendIndex = 0;
					}
				}
				break;
			case KEY_LEFT:
				selectedFriendIndex--;
				if(selectedFriendIndex < 0){
					selectedFriendIndex = GetPartyMemberCount() - 1;
				}
				while(GetPartyMember(selectedFriendIndex)->battler.hp == 0){
					selectedFriendIndex--;
					if(selectedFriendIndex < 0){
						selectedFriendIndex = GetPartyMemberCount() - 1;
					}
				}
				break;
			case KEY_DECIDE:
				receivers.push_back(selectedFriendIndex + PARTY_MIN_ID);
				ChangePhase(BP_ACTION_ANIME);
				break;
			case KEY_CANCEL:
				switch(action){
				case BA_ATTACK:
					ChangePhase(BP_COMMAND_CHOICE);
					break;
				case BA_SKILL:
					ChangePhase(BP_SKILL_CHOICE);
					break;
				}
				break;
			}
		}
		break;
	}
}

void BattleScene_Update()
{
	switch(phase){
	case BP_STARTING:
		if(phaseCount == 60){
			ChangePhase(BP_COMMAND_CHOICE);
		}
	case BP_COMMAND_CHOICE:
		break;
	case BP_SKILL_CHOICE:
		break;
	case BP_ACTION_ANIME:
		if(phaseCount >= 30){
			ChangePhase(BP_ACTION_APPLY);
		}
		break;
	case BP_ACTION_APPLY:
		{
			bool applyAnimeEnded = false;
			int receiverNo = receivers[currentReceiverIndex];
			bool isEnemy = IsOrderNoEnemy(receiverNo);
			Battler *receiverBattler = GetBattler(receiverNo);
			switch(applyAnimePhase){
			case APPLY_ANIME_PHASE_DAMAGE:
				if(isEnemy && (applyAnimeCount % 5) == 0 && applyAnimeCount < 30){
					enemyViews[receiverNo].visible = !enemyViews[receiverNo].visible;
				}
				if(applyAnimeCount == 30){
					if(isEnemy && receiverBattler->hp == 0){
						enemyViews[receiverNo].visible = false;
						applyAnimePhase = APPLY_ANIME_PHASE_DEAD;
						applyAnimeCount = 0;
						sprintf(helpText, "%s を倒した!", GetBattler(receiverNo)->name);
					}
					if(isEnemy && receiverBattler->hp != 0){
						enemyViews[receiverNo].visible = true;
					}
				}
				if(applyAnimeCount == 60){
					applyAnimeEnded = true;
				}
				break;
			case APPLY_ANIME_PHASE_DEAD:
				if(applyAnimeCount >= 60){
					applyAnimeEnded = true;
				}
				break;
			}
			if(applyAnimeEnded){
				currentReceiverIndex++;
				if(currentReceiverIndex >= receivers.size()){
					Judge();
					if(battleResult != BATTLE_RESULT_UNKNOWN){
						if(battleResult == BATTLE_RESULT_WIN){
							ChangePhase(BP_WIN);
						}else{
							ChangePhase(BP_DEFEAT);
						}
					}else{
						NextTurn();
					}
				}else{
					applyAnimePhase = APPLY_ANIME_PHASE_DAMAGE;
					applyAnimeCount = 0;
					ApplyAction(receivers[currentReceiverIndex]);
					SetApplyActionHelp(receivers[currentReceiverIndex]);
					StartDamagedAnime(receivers[currentReceiverIndex]);
				}
			}
		}
		applyAnimeCount++;
		break;
	case BP_WIN:
		if(phaseCount == 60){
			BattleScene_Exit();
		}
		break;
	case BP_DEFEAT:
		if(phaseCount == 60){
			BattleScene_Exit();
		}
		break;
	}
	for(int i = 0; i < partyStateWindows.size(); i++){
		if(partyStateWindows[i].damaged){
			partyStateWindows[i].damagedCount++;
			if(partyStateWindows[i].damagedCount >= PARTY_DAMAGE_ANIME_DURATION){
				partyStateWindows[i].damaged = 0;
				partyStateWindows[i].damagedCount = 0;
			}
		}
	}
	phaseCount++;
}

void BattleScene_RenderCommandChoices()
{
	int winY = 320;
	FillRect(100, winY, GetScreenWidth() - 100, winY + 35, Color(60, 60, 60));
	FillRect(100 + commandChoiceIndex * 100, winY, 100 + (commandChoiceIndex+1) * 100 + 20, winY + 35, Color(255, 0, 0));
	for(int i = 0; i < 4; i++){
		DrawString(120 + i * 100, winY + 8, commandChoiceTexts[i]);
	}
}

void BattleScene_RenderSkillChoicesWindow()
{
	if(phase != BP_SKILL_CHOICE) return;
	int winY = 320 + 40;
	FillRect(100, winY, GetScreenWidth() - 100, GetScreenHeight() - 5, Color(60, 60, 60));
	PartyMember *member = GetCurrentPartyMember();
	if(member == NULL) return;
	int itemW = 200;
	for(int i = 0; i < SKILL_CHOICE_XSIZE * SKILL_CHOICE_YSIZE; i++){
		int itemX = 110 + i / SKILL_CHOICE_YSIZE * itemW;
		int itemY = winY + 10 + (i % SKILL_CHOICE_YSIZE) * 32; 
		if(i == selectedSkillIndex){
			FillRect(
				itemX,
				itemY,
				itemX + itemW,
				itemY + 32,
				Color(255, 0, 0)
			);
		}
		if(i < member->skillIDs.size()){
			Skill *skill = GetSkill(member->skillIDs[i]);
			if(skill != NULL){
				unsigned int color = GetCurrentBattler()->mp >= skill->decreaseMP ? Color(255, 255, 255) : Color(128, 128, 128);
				DrawString(itemX, itemY, skill->name, color);
				char text[128];
				sprintf(text, "%d", skill->decreaseMP);
				DrawString(itemX + itemW - GetDrawStringWidth(text), itemY, text, color);
			}
		}
	}
}

void BattleScene_RenderHelpWindow()
{
	if(helpText[0] == '\0') return;
	FillRect(100, 40, GetScreenWidth() - 100, 40 + 40, Color(60, 60, 60));
	DrawString(120, 50, helpText);
}

void BattleScene_RenderPartyStateWindow()
{
	int winW = 110;
	int partyX = GetScreenWidth() / 2 - (winW * (GetPartyMemberCount() * 0.5f)) - (20 * ((GetPartyMemberCount() - 1) / 2));
	for(int i = 0; i < GetPartyMemberCount(); i++){
		PartyMember *member = GetPartyMember(i);
		int winX = partyX + (winW + 20) * i;
		int winY = 320 + 50;
		if(!IsOrderNoEnemy(orders[orderIndex]) && 
			(phase != BP_FRIEND_CHOICE && i == orders[orderIndex] - PARTY_MIN_ID || 
			(phase == BP_FRIEND_CHOICE && i == selectedFriendIndex))){
			winY -= 10;
		}
		if(partyStateWindows[i].damaged){
			winY += sin(partyStateWindows[i].damagedCount / (float)PARTY_DAMAGE_ANIME_DURATION * 3.141592f) * 30;
		}
		FillRect(
			winX,
			winY, 
			winX + winW,
			winY + 110,
			Color(60, 60, 60)
		);
		if(!IsOrderNoEnemy(orders[orderIndex]) && 
			(phase != BP_FRIEND_CHOICE && i == orders[orderIndex] - PARTY_MIN_ID || 
			(phase == BP_FRIEND_CHOICE && i == selectedFriendIndex))){
			DrawRect(
			winX,
				winY, 
				winX + winW,
				winY + 110,
				Color(255, 0, 0)
			);
		}
		char text[128];
		DrawString(winX + 5, winY + 15, member->battler.name);
		sprintf(text, "HP %d", member->battler.hp);
		DrawString(winX + 5, winY + 45, text);
		sprintf(text, "MP %d", member->battler.mp);
		DrawString(winX + 5, winY + 75, text);
	}
}

void BattleScene_RenderEnemies()
{
	for(int i = 0; i < enemyNoList.size(); i++){
		if(enemyViews[i].visible){
			int image = GetEnemy(enemyNoList[i])->image;
			int y = GetScreenHeight() / 2 - GetTextureHeight(image) / 2;
			if(phase == BP_ENEMY_CHOICE && i == selectedEnemyIndex){
				y -= 16;
			}
			DrawTexture(
				image, 
				GetScreenWidth() / 2 - 75 * (enemyNoList.size() / 2) + 150 * i - GetTextureWidth(image) / 2,
				y
			);
			/*
			if(phase == BP_ENEMY_CHOICE && i == selectedEnemyIndex){
				DrawTexture(
					enemyChoiceIconTex, 
					GetScreenWidth() / 2 - 75 * (enemyNoList.size() / 2) + 150 * i - GetTextureWidth(enemyChoiceIconTex) / 2,
					280
				);
			}
			*/
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

static void Judge()
{
	bool partyDestroied = true;
	for(int i = 0; i < GetPartyMemberCount(); i++){
		if(GetPartyMember(i)->battler.hp > 0){
			partyDestroied = false;
			break;
		}
	}
	if(partyDestroied){
		battleResult = BATTLE_RESULT_DEFEAT;
		return;
	}
	bool enemiesDestroied = true;
	for(int i = 0; i < enemyBattlers.size(); i++){
		if(enemyBattlers[i].hp > 0){
			enemiesDestroied = false;
			break;
		}
	}
	if(enemiesDestroied){
		battleResult = BATTLE_RESULT_WIN;
	}
}

//敵の行動を決定
static void EnemyThink()
{
	assert(GetCurrentEnemy() != NULL);
	EnemyAction *selectedEnemyAction = NULL;
	EnemyAction *enemyAction = NULL;
	std::vector<EnemyAction *> ratingActions;
	for(int i = 0; i < GetCurrentEnemy()->actions.size(); i++){
		enemyAction = GetCurrentEnemy()->actions[i];
		if(turnCount >= enemyAction->startTurn &&
			((turnCount - enemyAction->startTurn) % enemyAction->everyTurn) == 0){
			for(int j = 0; j < enemyAction->rating; j++){
				ratingActions.push_back(enemyAction);
			}
		}
	}
	assert(ratingActions.size() != 0);
	selectedEnemyAction = ratingActions[rand() % ratingActions.size()];
	action = (BattleAction)(selectedEnemyAction->baseAction + 1);
	printf("selectedEnemyAction->skillID=%d\n", selectedEnemyAction->skillID);
	if(action == BA_SKILL || selectedEnemyAction->skillID != -1){
		assert(GetSkill(selectedEnemyAction->skillID) != NULL);
		selectedSkillID = selectedEnemyAction->skillID;
		action = BA_SKILL;
	}
	if(action == BA_ATTACK ||
		(action == BA_SKILL && GetSkill(selectedEnemyAction->skillID)->scope == SCOPE_ONE_ENEMY)){
		std::vector<int> targetIDs;
		for(int i = 0; i < orders.size(); i++){
			if(!IsOrderNoEnemy(orders[i]) && GetBattler(orders[i])->hp != 0){
				targetIDs.push_back(orders[i]);
			}
		}
		assert(targetIDs.size() != 0);
		receivers.push_back(targetIDs[rand() % targetIDs.size()]);
	}
	assert(receivers.size() != 0);
}

static void ChangePhase(BattlePhase nextPhase)
{
	phase = nextPhase;
	phaseCount = 0;
	strcpy(helpText, "");
	switch(phase){
	case BP_STARTING:
		sprintf(helpText, "魔物が現れた!");
		break;
	case BP_COMMAND_CHOICE:
		if(orders[orderIndex] >= PARTY_MIN_ID){
			commandChoiceIndex = 0;
		}else{
			EnemyThink();
			ChangePhase(BP_ACTION_ANIME);
		}
		break;
	case BP_SKILL_CHOICE:
		selectedSkillIndex = 0;
		if(GetPartySelectedSkill() != NULL){
			strcpy(helpText, GetPartySelectedSkill()->description);
		}
		break;
	case BP_ENEMY_CHOICE:
		selectedEnemyIndex = 0;
		//生きている敵を選択
		while(enemyBattlers[selectedEnemyIndex].hp == 0){
			selectedEnemyIndex++;
			if(selectedEnemyIndex >= enemyBattlers.size()){
				selectedEnemyIndex = 0;
			}
		}
		strcpy(helpText, enemyBattlers[selectedEnemyIndex].name);
		break;
	case BP_ACTION_ANIME:
		assert(GetCurrentBattler() != NULL);
		switch(action){
		case BA_ATTACK:
			sprintf(helpText, "%s の攻撃!", GetCurrentBattler()->name);
			break;
		case BA_SKILL:
			assert(GetSelectedSkill() != NULL);
			sprintf(helpText, "%s の%s!", GetCurrentBattler()->name, GetSelectedSkill()->name);
			break;
		}
		break;
	case BP_ACTION_APPLY:
		if(action == BA_SKILL){
			GetCurrentBattler()->mp -= GetSelectedSkill()->decreaseMP; //MPを消費
		}
		currentReceiverIndex = 0;
		ApplyAction(receivers[currentReceiverIndex]);
		SetApplyActionHelp(receivers[currentReceiverIndex]);
		StartDamagedAnime(receivers[currentReceiverIndex]);
		break;
	case BP_WIN:
		strcpy(helpText, "魔物たちを倒した!");
		break;
	case BP_DEFEAT:
		strcpy(helpText, "戦いに敗れた...");
		break;
	}
}

static void ApplyAction(int orderNo)
{
	switch(action){
	case BA_ATTACK:
		currentDamage = GetCurrentBattler()->offense;
		GetBattler(orderNo)->hp -= currentDamage;
		if(GetBattler(orderNo)->hp < 0){
			GetBattler(orderNo)->hp = 0;
		}
		break;
	case BA_SKILL:
		{
			Skill *skill = GetSelectedSkill();
			currentDamage = skill->damage;
			if(skill->damage > 0){
				GetBattler(orderNo)->hp -= currentDamage;
			}else if(skill->damage < 0){
				GetBattler(orderNo)->hp -= currentDamage;
			}
			if(GetBattler(orderNo)->hp > GetBattler(orderNo)->maxHp){
				GetBattler(orderNo)->hp = GetBattler(orderNo)->maxHp;
			}
			if(GetBattler(orderNo)->hp < 0){
				GetBattler(orderNo)->hp = 0;
			}
		}
		break;
	}
}

static void SetApplyActionHelp(int orderNo)
{
	switch(action){
	case BA_ATTACK:
		sprintf(helpText, "%s に %d のダメージ!", GetBattler(orderNo)->name, currentDamage);
		break;
	case BA_SKILL:
		{
			if(currentDamage > 0){
				sprintf(helpText, "%s に %d のダメージ!", GetBattler(orderNo)->name, currentDamage);
			}else if(currentDamage < 0){
				sprintf(helpText, "%s は %d 回復!", GetBattler(orderNo)->name, abs(currentDamage));
			}
		}
		break;
	}
}

static void StartDamagedAnime(int orderNo)
{
	if(!IsOrderNoEnemy(orderNo) && currentDamage > 0){
		PartyStateWindow *partyStateWin = &partyStateWindows[orderNo - PARTY_MIN_ID];
		partyStateWin->damaged = true;
		partyStateWin->damagedCount = 0;
	}
	applyAnimeCount = 0;
	applyAnimePhase = APPLY_ANIME_PHASE_DAMAGE;
}

static Battler *GetCurrentBattler()
{
	return GetBattler(orders[orderIndex]);
}

static Battler *GetBattler(int orderNo)
{
	if(orderNo >= PARTY_MIN_ID){
		return &GetPartyMember(orderNo - PARTY_MIN_ID)->battler;
	}else{
		return &enemyBattlers[orderNo];
	}
}

static Enemy *GetCurrentEnemy()
{
	if(orders[orderIndex] >= PARTY_MIN_ID){
		return NULL;
	}
	return GetEnemy(enemyNoList[orders[orderIndex]]);
}

static PartyMember *GetCurrentPartyMember()
{
	return GetPartyMember(orders[orderIndex] - PARTY_MIN_ID);
}

static Skill *GetPartySelectedSkill()
{
	if(GetCurrentPartyMember() == NULL) return NULL;
	if(selectedSkillIndex < GetCurrentPartyMember()->skillIDs.size()){
		return GetSkill(GetCurrentPartyMember()->skillIDs[selectedSkillIndex]);
	}else{
		return NULL;
	}
}

static Skill *GetSelectedSkill()
{
	return GetSkill(selectedSkillID);
}

static void SortOrder()
{
	for(int i = 0; i < orders.size() - 1; i++){
		for(int j = 0; j < orders.size() - i - 1; j++){
			Battler *a = GetBattler(orders[j]);
			Battler *b = GetBattler(orders[j + 1]);
			if(a->speed < b->speed){
				printf("%d <-> %d\n", orders[j], orders[j+1]);
				int temp = orders[j];
				orders[j] = orders[j+1];
				orders[j+1] = temp;
			}
		}
	}
}

static bool IsOrderNoEnemy(int orderNo)
{
	return orderNo < PARTY_MIN_ID;
}

static void NextTurn()
{
	receivers.clear();
	orderIndex++;
	if(orderIndex >= orders.size()){
		orderIndex = 0;
	}
	while(GetBattler(orders[orderIndex])->hp == 0){
		orderIndex++;
		turnCount++;
		if(orderIndex >= orders.size()){
			orderIndex = 0;
			break;
		}
	}
	ChangePhase(BP_COMMAND_CHOICE);
}
