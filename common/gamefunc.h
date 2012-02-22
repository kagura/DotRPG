#ifndef GAME_FUNC_H
#define GAME_FUNC_H

#define Color(r, g, b) (r << 16) | (g << 8) | b

enum InputKey
{
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_DECIDE,
	KEY_CANCEL,
	KEY_MAX,
};


enum InputKeyState
{
	KEY_STATE_FREE,
	KEY_STATE_PRESS,
	KEY_STATE_DOWN,
	KEY_STATE_UP,
};

//Ž¸”s‚·‚é‚Æ-1‚ª•Ô‚é
int LoadTexture(const char *filename);
void DeleteTexture(int tex);
int GetTextureWidth(int tex);
int GetTextureHeight(int tex);
void DrawTexture(int tex, int x, int y);
void DrawRectTexture(int tex, int x, int y, int srcX, int srcY, int srcWidth, int srcHeight);
void SetViewport(int x, int y, int width, int height);
void DrawString(int x, int y, const char *text, unsigned int color = Color(255, 255, 255));
int GetDrawStringWidth(const char *text);
void DrawRect(int x1, int y1, int x2, int y2, unsigned int color);
void FillRect(int x1, int y1, int x2, int y2, unsigned int color);
void ErrorMessage(const char *msg);

#endif