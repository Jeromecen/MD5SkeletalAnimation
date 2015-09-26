/*
Author:  JeromeCen
Date:  2015.8.24
Description: D3D库引入的工具头文件，源自龙书，感谢原书作者Frank D.Luna和Rod Lopez.
*/
#ifndef D3D_UTILITY_H_
#define D3D_UTILITY_H_
/*LPDIRECT3D9等加入*/
#include <d3d9.h>
#include <d3dx9.h>
#include <time.h>
#include <tchar.h>

/*D3DXMATRIX加入*/
#include <d3dx9math.h>
/*LPDIRECT3D9等加入*/
#pragma comment(lib,"d3d9.lib")
/*D3DXMatrixPerspectiveFovLH加入*/
#pragma comment(lib,"d3dx9.lib")
// 媒体相关的库，用于抵挡的游戏音频和游戏手柄
#pragma comment(lib, "winmm.lib")
// 工程属性->清单工具->输入输出->嵌入清单->是改为否
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