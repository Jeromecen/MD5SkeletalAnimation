/*
Author:  JeromeCen
Date:  2015.8.24
Description: D3D������Ĺ���ͷ�ļ���Դ�����飬��лԭ������Frank D.Luna��Rod Lopez.
*/
#ifndef D3D_UTILITY_H_
#define D3D_UTILITY_H_
/*LPDIRECT3D9�ȼ���*/
#include <d3d9.h>
#include <d3dx9.h>
#include <time.h>
#include <tchar.h>

/*D3DXMATRIX����*/
#include <d3dx9math.h>
/*LPDIRECT3D9�ȼ���*/
#pragma comment(lib,"d3d9.lib")
/*D3DXMatrixPerspectiveFovLH����*/
#pragma comment(lib,"d3dx9.lib")
// ý����صĿ⣬���ڵֵ�����Ϸ��Ƶ����Ϸ�ֱ�
#pragma comment(lib, "winmm.lib")
// ��������->�嵥����->�������->Ƕ���嵥->�Ǹ�Ϊ��
#include <string>
#include "Trackball.h"

namespace d3d
{
	bool InitD3D(
		HINSTANCE hInstance,       // [in] Application instance.
		int width, int height,     // [in] Backbuffer dimensions.
		bool windowed,             // [in] Windowed (true)or full screen (false).
		D3DDEVTYPE deviceType,     // [in] HAL or REF
		IDirect3DDevice9** device);// [out]The created device.

	int EnterMsgLoop( 
		bool (*ptr_display)(float curTime));

	LRESULT CALLBACK WndProc(
		HWND hwnd,
		UINT msg, 
		WPARAM wParam,
		LPARAM lParam);

	template<class T> void Release(T t)
	{
		if( t )
		{
			t->Release();
			t = 0;
		}
	}

	template<class T> void Delete(T t)
	{
		if( t )
		{
			delete t;
			t = 0;
		}
	}

	extern Trackball *pTrack;
	extern int m_nAnimation;
}

#endif