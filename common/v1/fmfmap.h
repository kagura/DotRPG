//-----------------------------------------------------------------------------
// Platinum���o�͂���*.fmf�t�@�C����ǂݏo���N���X
// �ǐ��d���ɏ����Ă���̂Ō������d���������ꍇ�͏��������Ďg�p���Ă��������B
//-----------------------------------------------------------------------------
#ifndef __CLASS_FMFMAP_H__
#define __CLASS_FMFMAP_H__
#pragma once

//#include <windows.h>
#include <stdio.h>

// FMF�t�@�C���w�b�_ (20 bytes)
typedef struct tag_FMFHeader
{
	unsigned long int	dwIdentifier;	// �t�@�C�����ʎq 'FMF_'
	unsigned long int	dwSize;			// �w�b�_���������f�[�^�T�C�Y
	unsigned long int	dwWidth;		// �}�b�v�̉���
	unsigned long int	dwHeight;		// �}�b�v�̍���
	char	byChipWidth;	// �}�b�v�`�b�v1�̕�(pixel)
	char	byChipHeight;	// �}�b�v�`�b�v�P�̍���(pixel)
	char	byLayerCount;	// ���C���[�̐�
	char	byBitCount;		// ���C���f�[�^�̃r�b�g�J�E���g
}FMFHEADER;

class CFmfMap
{
public:
	// �\�z/����
	CFmfMap(void);
	~CFmfMap();

	// �}�b�v���J���ăf�[�^��ǂݍ���
	bool Open(const char *szFilePath);

	// �}�b�v���J����Ă��邩
	bool IsOpen() const;

	// �}�b�v���������J��
	void Close(void);
		
	// �w�背�C���[�̐擪�A�h���X�𓾂�
	void* GetLayerAddr(char byLayerIndex) const;
	
	// ���C���ԍ��ƍ��W���w�肵�Ē��ڃf�[�^��Ⴄ
	int GetValue(char byLayerIndex, unsigned long int dwX, unsigned long int dwY) const;

	// ���C���ԍ��ƍ��W���w�肵�ăf�[�^���Z�b�g
	void SetValue(char byLayerIndex, unsigned long int dwX, unsigned long int dwY, int nValue);

	// �w�b�_�̏��𓾂�
	unsigned long int GetMapWidth(void) const;
	unsigned long int GetMapHeight(void) const;
	char GetChipWidth(void) const;
	char GetChipHeight(void) const;
	char GetLayerCount(void) const;
	char GetLayerBitCount(void) const;
protected:
	// FMF�t�@�C���w�b�_�\����
	FMFHEADER	m_fmfHeader;
	// ���C���[�f�[�^�ւ̃|�C���^
	char* 		m_pLayerAddr;
};

/*	�T���v��
int Hoge(const char *filePath)
{
	CFmfMap map;
	char* pLayer;

	if (!map.Open(filePath))
	{
		// �}�b�v���J���Ȃ��B
		return 1;
	}

	// 0�ԁi��ԉ��̃��C���[�j�̃A�h���X��Ⴄ
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
	
	// �}�b�v�̕`��
	for (unsigned long int y = 0; y < height; y++)
	{
		for (unsigned long int x = 0; x < width ; x++)
		{
			index = *(pLayer + y * width + x);
			// �܂���
			index = map.GetValue(0, x, y);
			
			// index�ɂ̓}�b�v���W(x, y)�̃}�b�v�f�[�^�������Ă�̂�
			// �p�[�c�摜(srcHDC)����value�Ɍ�������`���Z�o���ĕ`�揈�����s���B
			// �}�b�v��8bit�̏ꍇ�p�[�c�̃A���C�������g��16�A16bit�Ȃ�256�B
			srcX = (index % 16) * cWidth;
			srcY = (index / 16) * cHeight;
			BitBlt(	dstHDC, x * cWidth, y * cHeight, cWidth, cHeight,
					srcHDC, srcX, srcY, SRCCOPY);
		}
	}

	// ���W(15,10)�̃f�[�^�����o���ꍇ
	value = *(pLayer + 10 * width + 15);
	
	// ����
	map.Close();
	
	return 0;
}
*/

#endif