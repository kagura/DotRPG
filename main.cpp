// �}�b�v�X�N���[����{
#include "DxLib.h"
#include "../common/gamemain.h"

const int SCREEN_W = 640;
const int SCREEN_H = 480;

static int ColorToDXColor(unsigned color)
{
	return GetColor((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

//////////////////////////////////////////////////////////////////////
//gmaefunc.h �̎���

int LoadTexture(const char *filename)
{
	return LoadGraph(filename);
}

void DeleteTexture(int tex)
{
	DeleteGraph(tex);
}

void DrawTexture(int tex, int x, int y)
{
	DrawGraph(x, y, tex, TRUE);
}

void DrawRectTexture(int tex, int x, int y, int srcX, int srcY, int srcWidth, int srcHeight)
{
	DrawRectGraph(
		x, y,
		srcX, srcY, srcWidth, srcHeight,
		tex, TRUE, FALSE
	);
}

void SetViewport(int x, int y, int width, int height)
{
	SetDrawArea(x, y, x + width, y + height);
}

int GetTextureWidth(int tex)
{
	int w, h;
	GetGraphSize(tex, &w, &h);
	return w;
}

int GetTextureHeight(int tex)
{
	int w, h;
	GetGraphSize(tex, &w, &h);
	return h;
}

void DrawString(int x, int y, const char *text, unsigned int color)
{
	DrawString(x, y, text, ColorToDXColor(color));
}

int GetDrawStringWidth(const char *text)
{
	return GetDrawStringWidth(text, strlen(text));
}

void DrawRect(int x1, int y1, int x2, int y2, unsigned int color)
{
	DrawBox(x1, y1 ,x2 ,y2 ,ColorToDXColor(color), FALSE);
}

void FillRect(int x1, int y1, int x2, int y2, unsigned int color)
{
	DrawBox(x1, y1 ,x2 ,y2 ,ColorToDXColor(color), TRUE);
}

void ErrorMessage(const char *msg)
{
	MessageBox(NULL, msg, "Message", MB_OK);
}

//////////////////////////////////////////////////////////////////////

//static bool spaceKeyPress = false;
static InputKeyState keyStates[KEY_MAX];

// �v�����l�������֐�
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
						 LPSTR lpCmdLine, int nCmdShow )
{
	freopen("stdout.txt", "w", stdout);
	
	SetCurrentDirectory("../common/data");
	
	ChangeWindowMode(TRUE);
	SetGraphMode( SCREEN_W , SCREEN_H , 16 ) ;
	if( DxLib_Init() == -1 )	// �c�w���C�u��������������
	{
		 return -1;				// �G���[���N�����璼���ɏI��
	}

	// �`����ʂ𗠉�ʂɂ���
	SetDrawScreen( DX_SCREEN_BACK ) ;
	
	GameInit();
	GameScreenResized(SCREEN_W, SCREEN_H);
	
	for(int i = 0; i < KEY_MAX; i++){
		keyStates[i] = KEY_STATE_FREE;
	}

	// ���[�v
	while( ProcessMessage() == 0 && CheckHitKey( KEY_INPUT_ESCAPE ) == 0 )
	{
		// ��ʂ�������
		ClearDrawScreen() ;
		
		for(int i = 0; i < KEY_MAX; i++){
			bool press = false;
			switch((InputKey)i){
			case KEY_LEFT:
				press = CheckHitKey(KEY_INPUT_LEFT);
				break;
			case KEY_RIGHT:
				press = CheckHitKey(KEY_INPUT_RIGHT);
				break;
			case KEY_UP:
				press = CheckHitKey(KEY_INPUT_UP);
				break;
			case KEY_DOWN:
				press = CheckHitKey(KEY_INPUT_DOWN);
				break;
			case KEY_DECIDE:
				press = CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_RETURN);
				break;
			case KEY_CANCEL:
				press = CheckHitKey(KEY_INPUT_X) || CheckHitKey(KEY_INPUT_BACK) || CheckHitKey(KEY_INPUT_ESCAPE);
				break;
			}
			if(press){
				if(keyStates[i] == KEY_STATE_FREE){
					GameInputKey((InputKey)i, KEY_STATE_DOWN);
					keyStates[i] = KEY_STATE_PRESS;
				}else{
					GameInputKey((InputKey)i, KEY_STATE_PRESS);
				}
			}else{
				if(keyStates[i] == KEY_STATE_PRESS ||
					keyStates[i] == KEY_STATE_DOWN){
					GameInputKey((InputKey)i, KEY_STATE_UP);
					keyStates[i] = KEY_STATE_FREE;
				}
			}
		}
		
		// �L�[���͂𓾂�
		/*
		int Key = GetJoypadInputState( DX_INPUT_KEY_PAD1 ) ;

		// �L�[���͂ɉ����ăv���C���[�̍��W���ړ�
		if( Key & PAD_INPUT_LEFT)
		{
			GameInputKey(KEY_LEFT, KEY_STATE_PRESS);
		}
		if( Key & PAD_INPUT_RIGHT)
		{
			GameInputKey(KEY_RIGHT, KEY_STATE_PRESS);
		}
		if( Key & PAD_INPUT_UP)
		{
			GameInputKey(KEY_UP, KEY_STATE_PRESS);
		}
		if( Key & PAD_INPUT_DOWN)
		{
			GameInputKey(KEY_DOWN, KEY_STATE_PRESS);
		}
		if( CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_RETURN) )
		{
			if(spaceKeyPress){
				GameInputKey(KEY_DECIDE, KEY_STATE_PRESS);
			}else{
				GameInputKey(KEY_DECIDE, KEY_STATE_DOWN);
			}
			spaceKeyPress = true;
		}else{
			spaceKeyPress = false;
		}
		*/
		
		GameUpdate();
		GameRender();

		// ����ʂ̓��e��\��ʂɉf��
		ScreenFlip() ;
	}
	
	GameDestroy();

	DxLib_End() ;				// �c�w���C�u�����g�p�̏I������

	return 0 ;					// �\�t�g�̏I��
}