#ifndef GLOBAL_SCENE_H
#define GLOBAL_SCENE_H

#include "script.h"

Interpreter *GetCurrentInterpreter();
void ShowMessageWindow(const char *text, bool closeWhenExit);
void CloseMessageWindow();
bool IsMessageShowEnded();
void SetSpeakerName(const char *name);
void PauseInterpreter();
void ResumeInterpreter();
void PopScene();
void AddBattleEnemy(int enemyNo);

#endif