/*
总结:
1.变换在物体坐标系和坐标体系中的理解：物体连续变换和坐标系变换差异，不同坐标系叉乘和四元数旋转不一样，观察相对位置不同。
骨骼父子节点关系中四元数旋转变换理解，左手坐标系如果不变换标准乘法那么需要(ba) v(ba)-1从后到先来旋转，
右手坐标系在标准乘法下顺时针变则需要(ab)-1v(ab)连续变换即可。
世界坐标系中变换骨骼矩阵整体表达：Mskin-world * (Mbone-world ^ -1) * Mcurrentbone-world的顺序来变换皮肤网格顶点。

2.父子节点变换传递性：父子骨骼节点中的整体变换，可以分解为缩放旋转平移，只有缩放旋转可传递（累加变换），平移不传递（不累加变换，只加结果）。
无论变换子节点还是绑定在节点中的网格顶点变换顺序都是先缩放在旋转，最后平移。

3.蒙皮权重和相对位置：顶点附加到骨骼节点之前还有一个权重，需要累加。在OGRE中动画集合（状态）下面每个动画有多个骨骼节点轨迹，每个节点轨迹有多个keyFrame,
一个动画的一瞬间就有多个动画节点轨迹（其实是一个frame)确定整体骨骼节点位置。OGRE中的蒙皮附加到骨骼节点，骨骼节点的变换是
相对于根骨骼节点的也就是相对于Local Model Space的，没有用相对于世界坐标系，目的是为了插入中间变换层，方便多个实体对象共用
一套相同的骨骼变换。

4.四元数新的理解：
虽然四元数的逆，无论是轴取反还是旋转取反，结果都是（w,-v),但是在变换向量时候，(w,-v)和（-w,v)等价的，证明见：
 (-w1,v1) x (-w1,-v1) = (w1*w1 + v1*v1, w1*v1 -w1*v1 - v1xv1);
 (w1,-v1) x  (w1,v1) = (w1*w1 + v1*v1, w1v1 - w1v1 -v1xv1)
 故在.md5mesh和.md5anim存放的四元数，为了配合骨骼节点右手坐标系中顺时针连续变换(ab)-1v(ab)，则四元数取反为：
 Quaternion MD5Model::buildQuat(float x, float y, float z) const {
	float w = 1.0f - x*x - y*y - z*z;
	w = w < 0.0 ? 0.0f : (float)-sqrt(double(w));
	Quaternion q(x, y, z, w);// 等价于w= sqrt(w)时候的q(-x, -y, -z, w)
	q.normalize();

	return q;
 }
*/
// MD5 Loader, by A.J. Tavakoli
#ifndef __MD5Model_h__
#define __MD5Model_h__

#include <string>
#include <stdexcept>
#include <vector>
#include <fstream>
#include "Quaternion.h"
#include "d3d_render.h"
#include "Log.h"
using namespace d3d;

class MD5Model {

public:
	MD5Model();
	~MD5Model();

	void clear();
	void loadMesh(const char *filename);
	int  loadAnim(const char *filename);
	void setAnim(int animIndex, int frameIndex = 0);
	void setFrame(int frameIndex);
	void animate(float dt);
	//void render();
	inline int getNumAnims() const { return int(anims.size()); }
	RenderVertex* getRenderVertex(int &nSize);
	RenderVertexIndex* getRenderVertexIndex(int &nSize);

	class Exception : public std::runtime_error {
	public:
		Exception(const std::string &msg) : std::runtime_error(msg) { }
	};

protected:
	// 解析.md5mesh .md5anim用的标志
	enum TOKEN {
		TOKEN_KEYWORD,
		TOKEN_INT,
		TOKEN_FLOAT,
		TOKEN_STRING,
		TOKEN_LBRACE,
		TOKEN_RBRACE,
		TOKEN_LPAREN,
		TOKEN_RPAREN,
		TOKEN_INVALID
	};

	class Vertex {
	public:
		// weight关联joint，一个顶点可以关联多个权重
		int weightIndex;
		// 是否是表示，weightIndex开始连续的weightIndex数？
		int weightCount;
		float pos[3];
		float tc[2];
		// normal是计算出来的
		float n[3];
	};

	// mesh中的joint
	class Joint {
	public:
		// name需要和animate里面的name关联
		std::string name;
		float pos[3];
		// quat的w值默认是0，后面需要计算填充
		Quaternion quat;
		int parent;
		// child是构建出来的
		std::vector<int> children;
	};

	class Tri {
	public:
		int v[3]; // vertex indices
	};

	class Weight {
	public:
		// 一个权重关联一个骨骼节点
		int joint;
		// 权重值
		float w;
		// 权重的位置，需要用骨骼的位置来计算
		float pos[3];
	};

	// 一、真正存放网格的数据结构
	class Mesh {
	public:
		std::string texture;
		// weightindex weightcout, textureCordinate 本身就没有pos值
		std::vector<Vertex> verts;
		// tri索引都读取进去了
		std::vector<Tri> tris;
		// 权重数不与顶点数或骨骼节点相等，存放了顶点的位置，通过weight关联的joint和weight值动态变化
		std::vector<Weight> weights;
	};

	// 动画文件，单帧数据信息，无论是关键帧还是插值帧
	class Frame {
	public:
		// 每帧的components数量？其实是每帧的pos + quat的float数，一行是6个float
		std::vector<float> animatedComponents;
		// 每帧有N个joint信息，其实frame信息就是joint的；无论是baseFrame的（相对于父节点的），还是后面的frame的（相于对baseFrame)。
		std::vector<Joint> joints;
		// bounds 字段中的信息，静态是关键帧的，每个帧一个包围盒
		float min[3]; // min point of bounding box
		float max[3]; // max point of bounding box
	};

	// .anim中的骨骼节点信息和.mesh中的joint个数一样，这里的骨骼有位置的话相对于父节点坐标
	class JointInfo {
	public:
		std::string name;
		// 父节点索引
		int parentIndex;
		// numComp - a flag that defines what components are keyframed.？
		// flags是从keyFrame animatedComponents中取得joint信息的位标志，没有该标志那么位置和方位就取baseFrame的
		int flags;
		// index into the frame data, pointing to the first animated component of this bone
		// startIndex是一个frame中该骨骼节点pos位置的开始索引，取得6个pos
		int startIndex;
	};

	// stores data from an anim file
	// 二、真正存放动画的数据结构
	class Anim {
	public:
		Anim();
		int numFrames;
		int frameRate;
		// keyFrame中的float个数，float存放的是位置和方位在frames中，需要用JointInfo hierarchy来索引读取
		int numAnimatedComponents;
		// 一个动画集合，里面的关键帧数
		std::vector<Frame> frames;
		// 关键帧对应的基础关节位置和方位
		std::vector<Joint> baseJoints;
		// 关节帧和frame之间的读取关系
		std::vector<JointInfo> jointInfo;
	};

	// methods for parser
	void  readElements(std::ifstream &fin);
	void  readAnimElements(std::ifstream &fin, Anim &anim);
	void  readCommandLineEl(std::ifstream &fin);
	void  readNumMeshesEl(std::ifstream &fin);
	void  readNumJointsEl(std::ifstream &fin);
	void  readNumFramesEl(std::ifstream &fin, Anim &anim);
	void  readFrameRateEl(std::ifstream &fin, Anim &anim);
	void  readNumAnimatedComponentsEl(std::ifstream &fin, Anim &anim);
	void  readJointsEl(std::ifstream &fin);
	void  readMeshEl(std::ifstream &fin);
	void  readHierarchyEl(std::ifstream &fin, Anim &anim);
	void  readBoundsEl(std::ifstream &fin, Anim &anim);
	void  readBaseframeEl(std::ifstream &fin, Anim &anim);
	void  readFrameEl(std::ifstream &fin, Anim &anim);
	int   readInt(std::ifstream &fin);
	float readFloat(std::ifstream &fin);
	void  readVec(std::ifstream &fin, float *v, int n);

	// methods for lexer
	void readString(std::ifstream &fin, std::string &str);
	void skipComments(std::ifstream &fin);
	void skipWhitespace(std::ifstream &fin);
	TOKEN getNextToken(std::ifstream &fin, std::string *tokStr = NULL);

	// utility method to compute w component of a normalized quaternion
	Quaternion buildQuat(float x, float y, float z) const;

	void createRenderData();
	void calculateRenderData();
	void buildVerts();
	void buildVerts(Frame &frame);
	void buildNormals();
	void buildFrames(Anim &anim);

	// .mesh中的节点个数
	int numJoints;
	// .mesh中的mesh数
	int numMeshes;

	// 当前anim用于播放不同的动画
	int currAnim;
	// 当前帧用于播放不同的帧
	int currFrame;

	// 当前流逝累计的时间，用于插帧
	float animTime;
	// 在关键帧间插值的帧
	Frame interpFrame; // used to interpolate between frames

	std::string textureName;

	// 静态网格数据，vertex包含了weight索引，weight中包含了joint索引；
	// vertex中的weightcout应该是从weightIndex开始,连续受到多个weight的影响，vertex中的normal和pos应该是通过joint weight动态计算出来的。
	std::vector<Mesh*> meshes;
	// 骨骼节点集合，来自于.mesh文件中的，位置和方位相对于模型坐标系
	// 骨骼节点的数据结构，一对多的多维结构父子节点存储的是索引，从父节点也可以遍历到叶子节点，但是从子节点却不可以往上遍历
	// 真正的节点存储在一维数据里面。
	std::vector<Joint> joints;
	// 动画集合，一个动画有N个keyFrame,每个KeyFrame里面有多个Joint的位置和方位信息相对于父节点坐标系
	std::vector<Anim*> anims;

	// 用于渲染的顶点
	RenderVertex* m_pRenderVertex;
	RenderVertexIndex* m_pRenderVertexIndex;
	int m_nRenderVertexCount;
	int m_nRenderIndexCount;
};

#endif
