//-----------------------------------------------------------------------------
// Platinumが出力する*.fmfファイルを読み出すクラス
// 可読性重視に書いてあるので効率を重視したい場合は書き換えて使用してください。
//-----------------------------------------------------------------------------
#ifndef __CLASS_FMFMAP_H__
#define __CLASS_FMFMAP_H__
#pragma once

//#include <windows.h>
#include <stdio.h>

// FMFファイルヘッダ (20 bytes)
typedef struct tag_FMFHeader
{
	unsigned long int	dwIdentifier;	// ファイル識別子 'FMF_'
	unsigned long int	dwSize;			// ヘッダを除いたデータサイズ
	unsigned long int	dwWidth;		// マップの横幅
	unsigned long int	dwHeight;		// マップの高さ
	char	byChipWidth;	// マップチップ1つの幅(pixel)
	char	byChipHeight;	// マップチップ１つの高さ(pixel)
	char	byLayerCount;	// レイヤーの数
	char	byBitCount;		// レイヤデータのビットカウント
}FMFHEADER;

class CFmfMap
{
public:
	// 構築/消滅
	CFmfMap(void);
	~CFmfMap();

	// マップを開いてデータを読み込む
	bool Open(const char *szFilePath);

	// マップが開かれているか
	bool IsOpen() const;

	// マップメモリを開放
	void Close(void);
		
	// 指定レイヤーの先頭アドレスを得る
	void* GetLayerAddr(char byLayerIndex) const;
	
	// レイヤ番号と座標を指定して直接データを貰う
	int GetValue(char byLayerIndex, unsigned long int dwX, unsigned long int dwY) const;

	// レイヤ番号と座標を指定してデータをセット
	void SetValue(char byLayerIndex, unsigned long int dwX, unsigned long int dwY, int nValue);

	// ヘッダの情報を得る
	unsigned long int GetMapWidth(void) const;
	unsigned long int GetMapHeight(void) const;
	char GetChipWidth(void) const;
	char GetChipHeight(void) const;
	char GetLayerCount(void) const;
	char GetLayerBitCount(void) const;
protected:
	// FMFファイルヘッダ構造体
	FMFHEADER	m_fmfHeader;
	// レイヤーデータへのポインタ
	char* 		m_pLayerAddr;
};

/*	サンプル
int Hoge(const char *filePath)
{
	CFmfMap map;
	char* pLayer;

	if (!map.Open(filePath))
	{
		// マップが開けない。
		return 1;
	}

	// 0番（一番下のレイヤー）のアドレスを貰う
	pLayer = (char*)map.GetLayerAddr(0)
	if (lpLayer == NULL)
	{
		map.Close();
		return 1;
	}

	unsigned long int width = map.GetMapWidth();
	unsigned long int height = map.GetMapHeight();
	unsigned long int cWidth = map.GetChipWidth();
	unsigned long int cHeight = map.GetChipHeight();
	int srcX, srcY;
	char index;	
	
	// マップの描画
	for (unsigned long int y = 0; y < height; y++)
	{
		for (unsigned long int x = 0; x < width ; x++)
		{
			index = *(pLayer + y * width + x);
			// または
			index = map.GetValue(0, x, y);
			
			// indexにはマップ座標(x, y)のマップデータが入ってるので
			// パーツ画像(srcHDC)からvalueに見合う矩形を算出して描画処理を行う。
			// マップが8bitの場合パーツのアラインメントは16、16bitなら256。
			srcX = (index % 16) * cWidth;
			srcY = (index / 16) * cHeight;
			BitBlt(	dstHDC, x * cWidth, y * cHeight, cWidth, cHeight,
					srcHDC, srcX, srcY, SRCCOPY);
		}
	}

	// 座標(15,10)のデータを取り出す場合
	value = *(pLayer + 10 * width + 15);
	
	// 閉じる
	map.Close();
	
	return 0;
}
*/

#endif