#ifndef __D3D_Render_h__
#define __D3D_Render_h__
namespace  d3d
{
	// ��Ⱦ��������
	struct RenderVertex
	{
		float pos[3];
		float tc[2];
		// normal�Ǽ��������
		float n[3];
		RenderVertex()
		{
			pos[0] = pos[1] = pos[3] = 0.0f;
			tc[0] = tc[1] = tc[2] = 0.0f;
			n[0] = n[1] = n[2] = 0.0f;
		}
	};

	// ������������
	struct RenderVertexIndex
	{
		int index;
		RenderVertexIndex()
		{
			index = 0;
		}
	};
}
#endif