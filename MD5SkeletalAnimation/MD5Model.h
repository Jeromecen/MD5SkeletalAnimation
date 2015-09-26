/*
�ܽ�:
1.�任����������ϵ��������ϵ�е���⣺���������任������ϵ�任���죬��ͬ����ϵ��˺���Ԫ����ת��һ�����۲����λ�ò�ͬ��
�������ӽڵ��ϵ����Ԫ����ת�任��⣬��������ϵ������任��׼�˷���ô��Ҫ(ba) v(ba)-1�Ӻ�������ת��
��������ϵ�ڱ�׼�˷���˳ʱ�������Ҫ(ab)-1v(ab)�����任���ɡ�
��������ϵ�б任�������������Mskin-world * (Mbone-world ^ -1) * Mcurrentbone-world��˳�����任Ƥ�����񶥵㡣

2.���ӽڵ�任�����ԣ����ӹ����ڵ��е�����任�����Էֽ�Ϊ������תƽ�ƣ�ֻ��������ת�ɴ��ݣ��ۼӱ任����ƽ�Ʋ����ݣ����ۼӱ任��ֻ�ӽ������
���۱任�ӽڵ㻹�ǰ��ڽڵ��е����񶥵�任˳��������������ת�����ƽ�ơ�

3.��ƤȨ�غ����λ�ã����㸽�ӵ������ڵ�֮ǰ����һ��Ȩ�أ���Ҫ�ۼӡ���OGRE�ж������ϣ�״̬������ÿ�������ж�������ڵ�켣��ÿ���ڵ�켣�ж��keyFrame,
һ��������һ˲����ж�������ڵ�켣����ʵ��һ��frame)ȷ����������ڵ�λ�á�OGRE�е���Ƥ���ӵ������ڵ㣬�����ڵ�ı任��
����ڸ������ڵ��Ҳ���������Local Model Space�ģ�û�����������������ϵ��Ŀ����Ϊ�˲����м�任�㣬������ʵ�������
һ����ͬ�Ĺ����任��

4.��Ԫ���µ���⣺
��Ȼ��Ԫ�����棬��������ȡ��������תȡ����������ǣ�w,-v),�����ڱ任����ʱ��(w,-v)�ͣ�-w,v)�ȼ۵ģ�֤������
 (-w1,v1) x (-w1,-v1) = (w1*w1 + v1*v1, w1*v1 -w1*v1 - v1xv1);
 (w1,-v1) x  (w1,v1) = (w1*w1 + v1*v1, w1v1 - w1v1 -v1xv1)
 ����.md5mesh��.md5anim��ŵ���Ԫ����Ϊ����Ϲ����ڵ���������ϵ��˳ʱ�������任(ab)-1v(ab)������Ԫ��ȡ��Ϊ��
 Quaternion MD5Model::buildQuat(float x, float y, float z) const {
	float w = 1.0f - x*x - y*y - z*z;
	w = w < 0.0 ? 0.0f : (float)-sqrt(double(w));
	Quaternion q(x, y, z, w);// �ȼ���w= sqrt(w)ʱ���q(-x, -y, -z, w)
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
	// ����.md5mesh .md5anim�õı�־
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
		// weight����joint��һ��������Թ������Ȩ��
		int weightIndex;
		// �Ƿ��Ǳ�ʾ��weightIndex��ʼ������weightIndex����
		int weightCount;
		float pos[3];
		float tc[2];
		// normal�Ǽ��������
		float n[3];
	};

	// mesh�е�joint
	class Joint {
	public:
		// name��Ҫ��animate�����name����
		std::string name;
		float pos[3];
		// quat��wֵĬ����0��������Ҫ�������
		Quaternion quat;
		int parent;
		// child�ǹ���������
		std::vector<int> children;
	};

	class Tri {
	public:
		int v[3]; // vertex indices
	};

	class Weight {
	public:
		// һ��Ȩ�ع���һ�������ڵ�
		int joint;
		// Ȩ��ֵ
		float w;
		// Ȩ�ص�λ�ã���Ҫ�ù�����λ��������
		float pos[3];
	};

	// һ�����������������ݽṹ
	class Mesh {
	public:
		std::string texture;
		// weightindex weightcout, textureCordinate �����û��posֵ
		std::vector<Vertex> verts;
		// tri��������ȡ��ȥ��
		std::vector<Tri> tris;
		// Ȩ�������붥����������ڵ���ȣ�����˶����λ�ã�ͨ��weight������joint��weightֵ��̬�仯
		std::vector<Weight> weights;
	};

	// �����ļ�����֡������Ϣ�������ǹؼ�֡���ǲ�ֵ֡
	class Frame {
	public:
		// ÿ֡��components��������ʵ��ÿ֡��pos + quat��float����һ����6��float
		std::vector<float> animatedComponents;
		// ÿ֡��N��joint��Ϣ����ʵframe��Ϣ����joint�ģ�������baseFrame�ģ�����ڸ��ڵ�ģ������Ǻ����frame�ģ����ڶ�baseFrame)��
		std::vector<Joint> joints;
		// bounds �ֶ��е���Ϣ����̬�ǹؼ�֡�ģ�ÿ��֡һ����Χ��
		float min[3]; // min point of bounding box
		float max[3]; // max point of bounding box
	};

	// .anim�еĹ����ڵ���Ϣ��.mesh�е�joint����һ��������Ĺ�����λ�õĻ�����ڸ��ڵ�����
	class JointInfo {
	public:
		std::string name;
		// ���ڵ�����
		int parentIndex;
		// numComp - a flag that defines what components are keyframed.��
		// flags�Ǵ�keyFrame animatedComponents��ȡ��joint��Ϣ��λ��־��û�иñ�־��ôλ�úͷ�λ��ȡbaseFrame��
		int flags;
		// index into the frame data, pointing to the first animated component of this bone
		// startIndex��һ��frame�иù����ڵ�posλ�õĿ�ʼ������ȡ��6��pos
		int startIndex;
	};

	// stores data from an anim file
	// ����������Ŷ��������ݽṹ
	class Anim {
	public:
		Anim();
		int numFrames;
		int frameRate;
		// keyFrame�е�float������float��ŵ���λ�úͷ�λ��frames�У���Ҫ��JointInfo hierarchy��������ȡ
		int numAnimatedComponents;
		// һ���������ϣ�����Ĺؼ�֡��
		std::vector<Frame> frames;
		// �ؼ�֡��Ӧ�Ļ����ؽ�λ�úͷ�λ
		std::vector<Joint> baseJoints;
		// �ؽ�֡��frame֮��Ķ�ȡ��ϵ
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

	// .mesh�еĽڵ����
	int numJoints;
	// .mesh�е�mesh��
	int numMeshes;

	// ��ǰanim���ڲ��Ų�ͬ�Ķ���
	int currAnim;
	// ��ǰ֡���ڲ��Ų�ͬ��֡
	int currFrame;

	// ��ǰ�����ۼƵ�ʱ�䣬���ڲ�֡
	float animTime;
	// �ڹؼ�֡���ֵ��֡
	Frame interpFrame; // used to interpolate between frames

	std::string textureName;

	// ��̬�������ݣ�vertex������weight������weight�а�����joint������
	// vertex�е�weightcoutӦ���Ǵ�weightIndex��ʼ,�����ܵ����weight��Ӱ�죬vertex�е�normal��posӦ����ͨ��joint weight��̬��������ġ�
	std::vector<Mesh*> meshes;
	// �����ڵ㼯�ϣ�������.mesh�ļ��еģ�λ�úͷ�λ�����ģ������ϵ
	// �����ڵ�����ݽṹ��һ�Զ�Ķ�ά�ṹ���ӽڵ�洢�����������Ӹ��ڵ�Ҳ���Ա�����Ҷ�ӽڵ㣬���Ǵ��ӽڵ�ȴ���������ϱ���
	// �����Ľڵ�洢��һά�������档
	std::vector<Joint> joints;
	// �������ϣ�һ��������N��keyFrame,ÿ��KeyFrame�����ж��Joint��λ�úͷ�λ��Ϣ����ڸ��ڵ�����ϵ
	std::vector<Anim*> anims;

	// ������Ⱦ�Ķ���
	RenderVertex* m_pRenderVertex;
	RenderVertexIndex* m_pRenderVertexIndex;
	int m_nRenderVertexCount;
	int m_nRenderIndexCount;
};

#endif
