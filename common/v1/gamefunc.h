//���s�����-1���Ԃ�
int LoadTexture(const char *filename);
int GetTextureWidth(int tex);
int GetTextureHeight(int tex);
void DrawTexture(int tex, int x, int y);
void DrawRectTexture(int tex, int x, int y, int srcX, int srcY, int srcWidth, int srcHeight);
void SetViewport(int x, int y, int width, int height);
void DrawString(int x, int y, const char *text);