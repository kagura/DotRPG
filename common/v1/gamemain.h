enum InputKey
{
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_DECIDE,
};


enum InputKeyState
{
	KEY_STATE_PRESS,
	KEY_STATE_DOWN,
	KEY_STATE_UP,
};

void GameInputKey(InputKey key, InputKeyState state);
void GameUpdate();
void GameRender();
void GameInit();
void GameScreenResized(int width, int height);
void GameDestroy();