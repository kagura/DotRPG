#include "database.h"
#include "util.h"
#include "picojson.h"
#include "gamefunc.h"
#include <stdlib.h>

static const char *SCOPE_NAMES[SCOPE_MAX] = {
	"one_enemy",
	"one_friend",
	"all_enemy",
	"all_friend",
	"all",
};

static std::vector<Skill *> skills;

void LoadDatabase()
{
	char *chars = LoadChars("skill.json");
	if(chars == NULL){
		printf("failed load skill.json\n");
		return;
	}
	picojson::value v;
    std::string err = picojson::parse(v, chars, chars + strlen(chars));
	if (err.empty()) {
		picojson::array arr = v.get<picojson::array>();
		picojson::array::iterator it;
		for (it = arr.begin(); it != arr.end(); it++) {
			picojson::object obj = it->get<picojson::object>();
			Skill *skill = new Skill;
			strncpy(skill->name, obj["name"].get<std::string>().c_str(), 128);
			strncpy(skill->description, obj["description"].get<std::string>().c_str(), 512);
			skill->scope = SCOPE_ONE_ENEMY;
			const char *scopeName = obj["scope"].get<std::string>().c_str();
			for(int i = 0; i < SCOPE_MAX; i++){
				if(strcmp(SCOPE_NAMES[i], scopeName) == 0){
					skill->scope = (Scope)i;
					break;
				}
			}
			skill->damage = (int)obj["damage"].get<double>();
			skill->decreaseMP = (int)obj["decreaseMP"].get<double>();
			/*
			printf("name=%s\n", skill->name);
			printf("description=%s\n", skill->description);
			printf("damage=%d\n", skill->damage);
			printf("decreaseMP=%d\n", skill->decreaseMP);
			*/
			skills.push_back(skill);
		}
	}else{
		char text[512];
		sprintf(text, "File:%s\nfailed parse json: %s", "skill.json", err.c_str());
		ErrorMessage(text);
	}
}

void CleanupDatabase()
{
	for(int i = 0; i < skills.size(); i++){
		delete skills[i];
	}
	skills.clear();
}

Skill *GetSkill(int skillNo)
{
	if(skillNo < 0 || skillNo >= skills.size()){
		return NULL;
	}
	return skills[skillNo];
}