#include "d3d_utility.h"
#include "MD5Model.h"
// Globals
IDirect3DDevice9* Device = 0; 
MD5Model *pMD5 = NULL;
const int Width  = 640;
const int Height = 480;
static  float g_lastTime = 0.0f;

//IDirect3DVertexBuffer9* VB = NULL; // vertex buffer to store
                                      // our triangle data.
//IDirect3DIndexBuffer9* IB = NULL;


static const char *pMeshFile = "qshambler.md5mesh";
static const char *pAnimateFile_Attack = "qshambler_attack01.md5anim";
static const char *pAnimateFile_Idle = "qshambler_idle02.md5anim";
static const char *pAnimateFile_Walk = "qshambler_walk.md5anim";

IDirect3DTexture9 * pTexture = NULL;
static const char *pTextureFile = "sydney.bmp";

// 顶点格式:
const DWORD FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
//
// Framework Functions
//
bool Setup()
{
	pMD5 = new MD5Model();
	if (pMD5 == NULL)
	{
		return false;
	}
	pMD5->loadMesh(pMeshFile);
	pMD5->loadAnim(pAnimateFile_Attack);
	pMD5->loadAnim(pAnimateFile_Idle);
	pMD5->loadAnim(pAnimateFile_Walk);
	pMD5->setAnim(0);
	
	D3DXMATRIX WMatrix;
	D3DXMatrixIdentity(&WMatrix);
	Device->SetTransform(D3DTS_WORLD, &WMatrix);

	D3DXMATRIX V;
	D3DXVECTOR3 eye(100.0f, 50.0f, -100.0f);
	D3DXVECTOR3 look(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&V, &eye, &look, &up);
	Device->SetTransform(D3DTS_VIEW, &V);

	// Set the projection matrix.
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
			&proj,                        // result
			D3DX_PI * 0.5f,               // 90 - degrees
			(float)Width / (float)Height, // aspect ratio
			1.0f,                         // near plane
			1000.0f);                     // far plane

	Device->SetTransform(D3DTS_PROJECTION, &proj);
	// 禁用光照
	Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	// 绕序
	Device->SetRenderState(D3DRS_CULLMODE, /*D3DCULL_CW*/D3DCULL_CW);

	/*Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	Device->SetRenderState(D3DRS_ZENABLE, TRUE);*/
	if (FAILED(D3DXCreateTextureFromFile(Device, pTextureFile, &pTexture)))
	{
		return false;
	}

	g_lastTime = (float)timeGetTime();
	return true;
}
void Cleanup()
{
	if( pMD5 != NULL )
	{
		pMD5->clear();
	}
}

bool Display(float curTime)
{
	if( Device )
	{
		static int nCurAnimate = 0; 
		if (nCurAnimate != m_nAnimation)
		{
			pMD5->setAnim(m_nAnimation);
			nCurAnimate = m_nAnimation;
		}
		float curTime = (float)timeGetTime();

		if (curTime - g_lastTime > 1)
		{
			float dt = float(curTime - g_lastTime) * 0.001f;
			g_lastTime = curTime;

			// if there is at least 1 animation, then animate
			if (pMD5->getNumAnims() > 0)
				pMD5->animate(dt);
		}
		float lookatMatrix[16];
		d3d::pTrack->look(lookatMatrix);
		D3DXMATRIX vMatrix = lookatMatrix;
		D3DXMATRIX tTranslate;
		D3DXMatrixTranslation(&tTranslate, 15.0f, -50.0f, 90.0f);
		D3DXMATRIX vvMatrix;
		D3DXMatrixMultiply(&vvMatrix, &vMatrix, &tTranslate);
		Device->SetTransform(D3DTS_VIEW, &vvMatrix);

	    D3DCOLOR uColor = D3DCOLOR_ARGB(255, 100,101, 140);
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, uColor, 1.0f, 0);
		Device->BeginScene();
		
		Device->SetFVF(FVF);
		Device->SetTexture(0, pTexture);
		int nVertexCount, nVertexIndexCount;
		RenderVertex* pVertexBuffer = NULL;
		RenderVertexIndex* pVertexIndexBuffer = NULL;
		pVertexBuffer = pMD5->getRenderVertex(nVertexCount);
		pVertexIndexBuffer = pMD5->getRenderVertexIndex(nVertexIndexCount);

		Device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
			0,
			nVertexCount,
			nVertexIndexCount / 3,
			pVertexIndexBuffer,
			D3DFMT_INDEX32,
			pVertexBuffer,
			sizeof(RenderVertex));
		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}


//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
		
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow(hwnd);
		break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_REF/*D3DDEVTYPE_HAL*/, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}
		
	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop( Display );

	Cleanup();

	Device->Release();

	return 0;
}