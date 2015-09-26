
// MD5 Loader, by A.J. Tavakoli

#ifdef _WIN32
#include <windows.h> // yuck
#endif

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <cctype>

#include <GL/gl.h>

#include "MD5Model.h"

#define IS_WHITESPACE(c) (' ' == c || '\t' == c || '\r' ==c || '\n' == c )


MD5Model::MD5Model() :
numJoints(0),
numMeshes(0),
currAnim(-1),
currFrame(0),
animTime(0.0f) {
	m_pRenderVertex = NULL;
	m_pRenderVertexIndex = NULL;
	m_nRenderVertexCount = 0;
	m_nRenderIndexCount = 0;
}

MD5Model::~MD5Model() {
	clear();
}


void MD5Model::clear() {
	// delete meshes
	for (size_t i = 0; i < meshes.size(); i++)
		if (meshes[i])
			delete meshes[i];

	// delete animations
	for (size_t i = 0; i < anims.size(); i++)
		if (anims[i])
			delete anims[i];

	meshes.clear();
	anims.clear();
	joints.clear();

	if (m_pRenderVertex != NULL)
	{
		delete[] m_pRenderVertex;
		m_pRenderVertex = NULL;
	}

	if (m_pRenderVertexIndex != NULL)
	{
		delete[] m_pRenderVertexIndex;
		m_pRenderVertexIndex = NULL;
	}
}


void MD5Model::animate(float dt) {
	// sanity check #1
	if (currAnim < 0 || currAnim >= int(anims.size()) || !anims[currAnim])
		throw Exception("MD5Model::animate(): currAnim is invalid");

	Anim *anim = anims[currAnim];

	// sanity check #2
	if (currFrame < 0 || currFrame >= int(anim->numFrames))
		throw Exception("MD5Model::animate(): currFrame is invalid");

	// compute index of next frame
	int nextFrameIndex = currFrame >= anim->numFrames - 1 ? 0 : currFrame + 1;

	// update animation time
	animTime += dt*float(anim->frameRate);
	if (animTime > 1.0f) {
		while (animTime > 1.0f)
			animTime -= 1.0f;
		//setFrame(nextFrameIndex);
		currFrame = nextFrameIndex;
		nextFrameIndex = currFrame >= anim->numFrames - 1 ? 0 : currFrame + 1;
	}

	// make sure size of storage for interpolated frame is correct
	if (int(interpFrame.joints.size()) != numJoints)
		interpFrame.joints.resize(numJoints);

	///// now interpolate between the two frames /////

	Frame &frame = anim->frames[currFrame],
		&nextFrame = anim->frames[nextFrameIndex];

	// interpolate between the joints of the current frame and those of the next
	// frame and store them in interpFrame
	for (int i = 0; i < numJoints; i++) {
		Joint &interpJoint = interpFrame.joints[i];

		// linearly interpolate between joint positions
		float *pos1 = frame.joints[i].pos,
			*pos2 = nextFrame.joints[i].pos;
		interpJoint.pos[0] = pos1[0] + animTime*(pos2[0] - pos1[0]);
		interpJoint.pos[1] = pos1[1] + animTime*(pos2[1] - pos1[1]);
		interpJoint.pos[2] = pos1[2] + animTime*(pos2[2] - pos1[2]);

		interpJoint.quat = slerp(frame.joints[i].quat, nextFrame.joints[i].quat, animTime);
	}

	buildVerts(interpFrame);
	buildNormals();
	calculateRenderData();
}


// sets current animation
void MD5Model::setAnim(int animIndex, int frameIndex) {
	if (animIndex < 0 || animIndex >= int(anims.size()))
		throw Exception("MD5Model::setAnim(): invalid animation index");

	if (currAnim != animIndex) {
		currAnim = animIndex;
		setFrame(frameIndex);
	}
}


void MD5Model::setFrame(int frameIndex) {
	// sanity check #1
	if (anims.size() == 0 || currAnim < 0)
		throw Exception("MD5Model::setFrame(): no animation has beens set");

	// sanity check #2
	if (frameIndex < 0 || !anims[currAnim] ||
		anims[currAnim]->numFrames <= 0 ||
		anims[currAnim]->numFrames <= frameIndex)
		throw Exception("MD5Model::setFrame(): frame index is invalid");

	buildVerts(anims[currAnim]->frames[frameIndex]);
	buildNormals();
	calculateRenderData();
	currFrame = frameIndex;
	animTime = 0.0f;
}


//void MD5Model::render() {
//	glPushAttrib(GL_CURRENT_BIT);
//	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
//
//	glEnableClientState(GL_VERTEX_ARRAY);
//	glEnableClientState(GL_NORMAL_ARRAY);
//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glDisableClientState(GL_COLOR_ARRAY);
//
//	for (size_t i = 0; i < meshes.size(); i++) {
//		const Mesh *mesh = meshes[i];
//
//		glVertexPointer(3, GL_FLOAT, GLsizei(sizeof(Vertex)), mesh->verts[0].pos);
//		glNormalPointer(GL_FLOAT, GLsizei(sizeof(Vertex)), mesh->verts[0].n);
//		glTexCoordPointer(2, GL_FLOAT, GLsizei(sizeof(Vertex)), mesh->verts[0].tc);
//
//		glDrawElements(GL_TRIANGLES, GLsizei(mesh->tris.size() * 3), GL_UNSIGNED_INT, &mesh->tris[0]);
//	}
//
//	glPopClientAttrib();
//	glPopAttrib();
//}


void MD5Model::loadMesh(const char *filename) {
	// sanity check
	if (!filename)
		throw Exception("MD5Model::loadMesh(): filename is NULL");

	// attempt to open file for reading
	std::ifstream fin(filename, std::ifstream::in);

	// was open successful?
	if (!fin.is_open()) {
		std::string msg = std::string("MD5Model::loadMesh(): unable to open ") +
			std::string(filename) + std::string(" for reading");
		throw Exception(msg);
	}

	// read in file version
	std::string str;
	getNextToken(fin, &str);

	// token must read "MD5Version"
	if (str != "MD5Version")
		throw Exception("MD5Model::loadMesh(): expected 'MD5Version'");

	// get version #
	int ver = readInt(fin);
	// must be version 10
	if (ver != 10)
		throw Exception("MD5Model::loadMesh(): MD5Version must be 10");

	// clear any data before reading file
	clear();

	// read in all of the MD5Model elements
	readElements(fin);

	// close input file (should be done destructor anyway...)
	fin.close();

	// calculate vertex positions and normals from information in joints
	// ģ��pose�����У���������ڵ��ʼλ��+����weightλ�ñ任�������ڵ㿪ʼ��λ�� + weightPos����joint��Ԫ����ת��λ�ã�* Ȩ�� �Ľ����Ϊ�����λ�á�
	buildVerts();
	// ֱ�Ӳ�ˣ��淶��������λ�淨������Ϊÿ������ķ�����
	buildNormals();

	createRenderData();
}

void MD5Model::createRenderData()
{
	m_nRenderVertexCount = 0;
	m_nRenderIndexCount = 0;
	for (size_t i = 0; i < meshes.size();  i++)
	{
		m_nRenderVertexCount += int(meshes[i]->verts.size());
	}
	m_pRenderVertex = new RenderVertex[m_nRenderVertexCount];

	for (int k = 0; k < int(meshes.size()); k++)
	{
		m_nRenderIndexCount += 3 * int(meshes[k]->tris.size());
	}
	m_pRenderVertexIndex = new RenderVertexIndex[m_nRenderIndexCount];

	// �ȰѲ��������,����������ź�
	int nVertexIndex = 0;
	int nIndexCount = 0;
	int nPreMeshIndex = 0;
	for (size_t i = 0; i < meshes.size(); i++)
	{
		for (size_t j = 0; j < meshes[i]->verts.size(); j++)
		{
			for (int k = 0; k < 2; k++)
			{
				m_pRenderVertex[nVertexIndex].tc[k] = meshes[i]->verts[j].tc[k];
			}
			nVertexIndex++;
		}

		if (i > 0)
		{
			nPreMeshIndex += meshes[i - 1]->verts.size();
		}
		
		for (size_t m = 0; m < meshes[i]->tris.size(); m++)
		{
			for (int n = 0; n < 3; n++)
			{
				m_pRenderVertexIndex[nIndexCount].index = meshes[i]->tris[m].v[n] + nPreMeshIndex;
				nIndexCount++;
			}
		}
	}
}

int MD5Model::loadAnim(const char *filename) {
	// attempt to open file for reading
	std::ifstream fin(filename, std::ifstream::in);

	if (!fin.is_open()) {
		std::string msg = std::string("MD5Model::loadAnim(): unable to open ") +
			std::string(filename) + std::string(" for reading");
		throw Exception(msg);
	}

	// read in file version
	std::string str;
	getNextToken(fin, &str);

	// token must read "MD5Version"
	if (str != "MD5Version")
		throw Exception("MD5Model::loadAnim(): expected 'MD5Version'");

	// get version #
	int ver = readInt(fin);

	// must be version 10
	if (ver != 10)
		throw Exception("MD5Model::loadAnim(): MD5Version must be 10");

	Anim *anim = new Anim;
	if (!anim)
		throw Exception("MD5Model::loadAnim(): could not allocate storage for animation");

	readAnimElements(fin, *anim);

	// close file (should be done by destructor anyway)
	fin.close();

	// add to array of animations
	int animIndex = (int)anims.size();
	anims.push_back(anim);

	// build frames for this animation
	buildFrames(*anim);

	// make this the current animation
	setAnim(animIndex, 0);

	return animIndex;
}


void MD5Model::readElements(std::ifstream &fin) {
	while (!fin.eof()) {
		std::string str;
		TOKEN tok = getNextToken(fin, &str);

		// token is TOKEN_INVALID when end of file is reached
		if (TOKEN_INVALID == tok)
			break;

		if (str == "commandline")
			readCommandLineEl(fin);
		else if (str == "numJoints")
			readNumJointsEl(fin);
		else if (str == "numMeshes")
			readNumMeshesEl(fin);
		else if (str == "joints")
			readJointsEl(fin);
		else if (str == "mesh")
			readMeshEl(fin);
		else {
			// invalid element
			throw Exception(std::string("MD5Model::readElements(): unknown element type '") + str + "'");
		}
	} // while ( not EOF )
}


void MD5Model::readAnimElements(std::ifstream &fin, Anim &anim) {
	while (!fin.eof()) {
		std::string str;
		TOKEN tok = getNextToken(fin, &str);

		// token is TOKEN_INVALID when end of file is reached
		if (TOKEN_INVALID == tok)
			break;

		if (str == "commandline")
			readCommandLineEl(fin);
		else if (str == "numJoints") {
			// just make sure that the number of joints is the same as the number
			// specified in the mesh file
			int n = readInt(fin);

			if (n != numJoints)
				throw Exception("MD5Model::readAnimElements(): anim file does not match mesh");
		}
		else if (str == "numMeshes") {
			// just make sure that the number of meshes is the same as the number
			// specified in the mesh file
			int n = readInt(fin);

			if (n != numMeshes)
				throw Exception("MD5Model::readAnimElements(): anim file does not match mesh");
		}
		else if (str == "numFrames")
			readNumFramesEl(fin, anim);
		else if (str == "frameRate")
			readFrameRateEl(fin, anim);
		else if (str == "numAnimatedComponents")
			readNumAnimatedComponentsEl(fin, anim);
		else if (str == "hierarchy")
			readHierarchyEl(fin, anim);
		else if (str == "bounds")
			readBoundsEl(fin, anim);
		else if (str == "baseframe")
			readBaseframeEl(fin, anim);
		else if (str == "frame")
			readFrameEl(fin, anim);
		else {
			// invalid element
			throw Exception(std::string("MD5Model::readAnimElements(): unknown element type '") + str + "'");
		}
	}
}


void MD5Model::readCommandLineEl(std::ifstream &fin) {
	// commandline elements can be ignored, but make sure the syntax is correct
	if (getNextToken(fin) != TOKEN_STRING)
		throw Exception("MD5Model::readCommandLineEl(): expected string");
}


void MD5Model::readNumJointsEl(std::ifstream &fin) {
	// if number of joints has already been set, can't set it again
	if (numJoints != 0)
		throw Exception("MD5Model::readNumJointsEl(): numJoints has already been set");

	// read in next token, should be an integer
	int n = readInt(fin);

	if (n <= 0)
		throw Exception("MD5Model::readNumJointsEl(): numJoints must be greater than 0");

	numJoints = n;
	//joints.resize(numJoints);
}


void MD5Model::readNumMeshesEl(std::ifstream &fin) {
	// if number of meshes has already been set, can't set it again
	if (numMeshes != 0)
		throw Exception("MD5Model::readNumMeshesEl(): numMeshes has already been set");

	// read in next token, should be an integer
	int n = readInt(fin);

	if (n <= 0)
		throw Exception("MD5Model::readNumMeshesEl(): numMeshes must be greater than 0");

	numMeshes = n;
	//meshes.resize(numMeshes);
}


void MD5Model::readNumFramesEl(std::ifstream &fin, Anim &anim) {
	// if number of frames has already been set, can't set it again
	if (anim.numFrames != 0)
		throw Exception("MD5Model::readNumFramesEl(): numFrames has already been set");

	// read in next token, should be an integer
	int n = readInt(fin);

	if (n <= 0)
		throw Exception("MD5Model::readNumFramesEl(): numFrames must be greater than 0");

	anim.numFrames = n;
	// ����frame����
	anim.frames.resize(n);
}


void MD5Model::readFrameRateEl(std::ifstream &fin, Anim &anim) {
	// if framerate has already been set, can't set it again
	if (anim.frameRate != 0)
		throw Exception("MD5Model::readFrameRateEl(): frameRate has already been set");

	// read in next token, should be an integer
	int n = readInt(fin);

	if (n <= 0)
		throw Exception("MD5Model::readFrameRateEl(): frameRate must be a positive integer");

	anim.frameRate = n;
}


void MD5Model::readNumAnimatedComponentsEl(std::ifstream &fin, Anim &anim) {
	// make sure parameter hasn't been set, as has been done with all of the
	// others
	if (anim.numAnimatedComponents != 0)
		throw Exception("MD5Model::readNumAnimatedComponentsEl(): numAnimatedComponents has already been set");

	// read in next token, should be an integer
	int n = readInt(fin);

	if (n <= 0)
		throw Exception("MD5Model::readNumAnimatedComponentsEl(): numAnimatedComponents must be a positive integer");

	anim.numAnimatedComponents = n;
}


void MD5Model::readJointsEl(std::ifstream &fin) {
	// make sure numJoints has been set
	if (numJoints <= 0)
		throw Exception("MD5Model::readJointsEl(): numJoints needs to be set before 'joints' block");

	TOKEN t = getNextToken(fin);

	// expect an opening brace { to begin block of joints
	if (t != TOKEN_LBRACE)
		throw Exception("MD5Model::readJointsEl(): expected { to follow 'joints'");

	// read in each joint in block until '}' is hit
	// (if EOF is reached before this, then the read*() methods
	//  will close the file and throw an exception)
	std::string str;
	t = getNextToken(fin, &str);
	while (t != TOKEN_RBRACE) {
		Joint joint;

		// token should be name of joint (a string)
		if (t != TOKEN_STRING)
			throw Exception("MD5Model::readJointsEl(): expected joint name (string)'");

		// read in index of parent joint
		int parentIndex = readInt(fin);

		// read in joint position
		readVec(fin, joint.pos, 3);

		// read in first 3 components of quaternion (must compute 4th)
		float quat[4];
		readVec(fin, quat, 3);

		// ����1.0f - x*x - y*y - z*z;�����w
		joint.quat = buildQuat(quat[0], quat[1], quat[2]);

		// add index to parent's list of child joints (if it has a parent,
		// root joints have parent index -1)
		if (parentIndex >= 0) {
			if (size_t(parentIndex) >= joints.size())
				throw Exception("MD5Model::readJointsEl(): parent index is invalid");

			joints[parentIndex].children.push_back(int(joints.size()));
		}

		// add joint to vector of joints
		joints.push_back(joint);

		// get next token
		t = getNextToken(fin, &str);
	}
}


void MD5Model::readMeshEl(std::ifstream &fin) {
	// make sure numMeshes has been set
	if (numMeshes <= 0)
		throw Exception("MD5Model::readMeshesEl(): numMeshes needs to be set before 'mesh' block");

	TOKEN t = getNextToken(fin);

	// expect an opening brace { to begin block of joints
	if (t != TOKEN_LBRACE)
		throw Exception("MD5Model::readMeshEl(): expected { to follow 'mesh'");

	Mesh *mesh = new Mesh;

	// read in all mesh information in block until '}' is hit
	// (if EOF is reached before this, then the read*() methods
	//  will close the file and throw an exception)
	std::string str;
	t = getNextToken(fin, &str);
	while (t != TOKEN_RBRACE) {
		if (str == "vert") {
			// syntax: vert (vertex index) '(' (x) (y) (z) ')' (weight index) (weight count)
			Vertex vert;

			int index = readInt(fin);
			readVec(fin, vert.tc, 2);
			vert.weightIndex = readInt(fin);
			vert.weightCount = readInt(fin);

			// make sure index is within bounds of vector of vertices
			if (size_t(index) >= mesh->verts.size())
				throw Exception("MD5Model::readMeshEl(): vertex index >= numverts");

			mesh->verts[index] = vert;
		}
		else if (str == "tri") {
			// syntax: tri (triangle index) (v0 index) (v1 index) (v2 index)
			Tri tri;

			int index = readInt(fin);

			// make sure index is within bounds of vector of triangles
			if (size_t(index) >= mesh->tris.size())
				throw Exception("MD5Model::readMeshEl(): triangle index >= numtris");

			tri.v[0] = readInt(fin);
			tri.v[1] = readInt(fin);
			tri.v[2] = readInt(fin);
			mesh->tris[index] = tri;
		}
		else if (str == "weight") {
			Weight weight;

			int weightIndex = readInt(fin);
			weight.joint = readInt(fin);
			weight.w = readFloat(fin);
			readVec(fin, weight.pos, 3);

			if (size_t(weightIndex) >= mesh->weights.size())
				throw Exception("MD5Model::readMeshEl(): weight index >= numweights");

			mesh->weights[weightIndex] = weight;
		}
		else if (str == "shader") {
			// û��ʹ������
			std::string shader;
			if (getNextToken(fin, &shader) != TOKEN_STRING)
				throw Exception("MD5Model::readMeshEl(): expected string to follow 'shader'");
		}
		else if (str == "numverts") {
			if (mesh->verts.size() > 0)
				throw Exception("MD5Model::readMeshEl(): numverts has already been set");

			int n = readInt(fin);

			if (n <= 0)
				throw Exception("MD5Model::readMeshEl(): numverts must be greater than 0");

			mesh->verts.resize(n);
		}
		else if (str == "numtris") {
			if (mesh->tris.size() > 0)
				throw Exception("MD5Model::readMeshEl(): numtris has already been set");

			int n = readInt(fin);

			if (n <= 0)
				throw Exception("MD5Model::readMeshEl(): numtris must be greater than 0");

			mesh->tris.resize(n);
		}
		else if (str == "numweights") {
			if (mesh->weights.size() > 0)
				throw Exception("MD5Model::readMeshEl(): numweights has already been set");

			int n = readInt(fin);

			if (n <= 0)
				throw Exception("MD5Model::readMeshEl(): numweights must be greater than 0");

			mesh->weights.resize(n);
		}

		// read next token
		t = getNextToken(fin, &str);
	}

	meshes.push_back(mesh);
}


// reads in hierarchy block from anim file
void MD5Model::readHierarchyEl(std::ifstream &fin, Anim &anim) {
	TOKEN t = getNextToken(fin);

	// expect an opening brace { to begin hierarchy block
	if (t != TOKEN_LBRACE)
		throw Exception("MD5Model::readHierarchyEl(): expected { to follow 'hierarchy'");

	anim.jointInfo.clear();

	// read in each joint in block until '}' is hit
	// (if EOF is reached before this, then the read*() methods
	//  will close the file and throw an exception)
	std::string str;
	t = getNextToken(fin, &str);
	while (t != TOKEN_RBRACE) {
		JointInfo info;

		// token should be name of a joint (a string)
		if (t != TOKEN_STRING)
			throw Exception("MD5Model::readHierarchyEl(): expected name (string)");

		info.name = str;
		info.parentIndex = readInt(fin);

		info.flags = readInt(fin);
		info.startIndex = readInt(fin);

		anim.jointInfo.push_back(info);

		// get next token
		t = getNextToken(fin, &str);
	}

	if (int(anim.jointInfo.size()) != numJoints)
		throw Exception("MD5Model::readHierarchyEl(): number of entires in hierarchy block does not match numJoints");
}


// reads in bounds block from anim file
void MD5Model::readBoundsEl(std::ifstream &fin, Anim &anim) {
	TOKEN t = getNextToken(fin);

	// expect an opening brace { to begin block
	if (t != TOKEN_LBRACE)
		throw Exception("MD5Model::readBoundsEl(): expected { to follow 'bounds'");

	if (anim.numFrames != int(anim.frames.size()))
		throw Exception("MD5Model::readBoundsEl(): frames must be allocated before setting bounds");

	// read in bound for each frame
	for (int i = 0; i < anim.numFrames; i++) {
		readVec(fin, anim.frames[i].min, 3);
		readVec(fin, anim.frames[i].max, 3);
	}

	// next token must be a closing brace }
	t = getNextToken(fin);

	if (TOKEN_LPAREN == t)
		throw Exception("MD5Model::readBoundsEl(): number of bounds exceeds number of frames");

	// expect a closing brace } to end block
	if (t != TOKEN_RBRACE)
		throw Exception("MD5Model::readBoundsEl(): expected }");
}


void MD5Model::readBaseframeEl(std::ifstream &fin, Anim &anim) {
	TOKEN t = getNextToken(fin);

	// expect an opening brace { to begin block
	if (t != TOKEN_LBRACE)
		throw Exception("MD5Model::readBaseframeEl(): expected { to follow 'baseframe'");

	anim.baseJoints.resize(numJoints);

	int i;
	for (i = 0; i < numJoints; i++) {
		// read in joint position
		readVec(fin, anim.baseJoints[i].pos, 3);

		// read in first 3 components of quaternion (must compute 4th)
		float quat[3];
		readVec(fin, quat, 3);

		anim.baseJoints[i].quat = buildQuat(quat[0], quat[1], quat[2]);
	}

	if (i < numJoints - 1)
		throw Exception("MD5Model::readBaseframeEl(): not enough joints");

	// next token must be a closing brace }
	t = getNextToken(fin);

	if (TOKEN_LPAREN == t)
		throw Exception("MD5Model::readBaseframeEl(): too many joints");

	// expect a closing brace } to end block
	if (t != TOKEN_RBRACE)
		throw Exception("MD5Model::readBaseframeEl(): expected }");
}


void MD5Model::readFrameEl(std::ifstream &fin, Anim &anim) {
	// numAnimatedComponents has to have been set before frame element
	if (0 == anim.numAnimatedComponents)
		throw Exception("MD5Model::readFrameEl(): numAnimatedComponents must be set before 'frame' block");

	// read frame index
	int frameIndex = readInt(fin);

	if (frameIndex < 0 || frameIndex >= anim.numFrames)
		throw Exception("MD5Model::readFrameEl(): invalid frame index");

	// get reference to frame and set number of animated components
	Frame &frame = anim.frames[frameIndex];
	frame.animatedComponents.resize(anim.numAnimatedComponents);

	TOKEN t = getNextToken(fin);

	// expect an opening brace { to begin block
	if (t != TOKEN_LBRACE)
		throw Exception("MD5Model::readFrameEl(): expected { to follow frame index");

	for (int i = 0; i < anim.numAnimatedComponents; i++)
		frame.animatedComponents[i] = readFloat(fin); // �պö�ȡframe�е�λ�������Ϸ�λ����flaot��

	t = getNextToken(fin);

	// expect a closing brace } to end block
	if (t != TOKEN_RBRACE)
		throw Exception("MD5Model::readFrameEl(): expected }");
}


// reads in a string terminal and stores it in str
// (assumes opening " has already been read in)
void MD5Model::readString(std::ifstream &fin, std::string &str) {
	str = std::string();
	// read characters until closing " is found
	char c = '\0';
	do {
		fin.get(c);

		if (fin.eof())
			throw Exception("MD5Model::readString(): reached end of file before \" was found");

		if (c != '"')
			str += c;
	} while (c != '"');
}


// reads in an integer terminal and returns its value
int MD5Model::readInt(std::ifstream &fin) {
	std::string str;
	TOKEN t = getNextToken(fin, &str);

	if (t != TOKEN_INT)
		throw Exception("MD5Model::readInt(): expected integer");

	return atoi(str.c_str());
}


// reads in a float terminal and returns its value
float MD5Model::readFloat(std::ifstream &fin) {
	std::string str;
	TOKEN t = getNextToken(fin, &str);

	// integer tokens are just numbers with out a decimal point, so they'll
	// suffice here as well
	if (t != TOKEN_FLOAT && t != TOKEN_INT)
		throw Exception("MD5Model::readFloat(): expected float");

	float f = 0.0f;
	sscanf_s(str.c_str(), "%f", &f);

	return f;
}


// reads in sequence consisting of n floats enclosed by parentheses
void MD5Model::readVec(std::ifstream &fin, float *v, int n) {
	if (getNextToken(fin) != TOKEN_LPAREN)
		throw Exception("MD5Model::readVec(): expected '('");

	for (int i = 0; i < n; i++)
		v[i] = readFloat(fin);

	if (getNextToken(fin) != TOKEN_RPAREN)
		throw Exception("MD5Model::readVec(): expected ')'");
}


void MD5Model::skipComments(std::ifstream &fin) {
	char c;
	fin.get(c);

	if (c != '/')
		throw Exception("MD5Model::skipComments(): invalid comment, expected //");

	while (!fin.eof() && c != '\n')
		fin.get(c);

	// put back last character read
	fin.putback(c);
}


// reads until first non-whitespace character
void MD5Model::skipWhitespace(std::ifstream &fin) {
	char c = '\0';
	while (!fin.eof()) {
		fin.get(c);

		if (!IS_WHITESPACE(c)) {
			fin.putback(c);
			break;
		}
	}
}


// reads in next token from file and matches it to a token type,
// if tokStr is non-NULL then it will be set to the text of the token
MD5Model::TOKEN MD5Model::getNextToken(std::ifstream &fin, std::string *tokStr) {
	skipWhitespace(fin);
	std::string str;

	TOKEN t = TOKEN_INVALID;

	while (!fin.eof()) {
		char c = '\0';
		fin.get(c);

		// single character tokens
		if ('{' == c || '}' == c || '(' == c || ')' == c) {
			// if already reading in a token, treat this as a delimiter
			if (t != TOKEN_INVALID) {
				fin.putback(c);
				if (tokStr != NULL)
					(*tokStr) = str;
			}

			if ('{' == c)
				t = TOKEN_LBRACE;
			if ('}' == c)
				t = TOKEN_RBRACE;
			if ('(' == c)
				t = TOKEN_LPAREN;
			if (')' == c)
				t = TOKEN_RPAREN;

			if (tokStr) {
				(*tokStr) = std::string();
				(*tokStr) += c;
			}
			return t;
		}
		if (isdigit(c)) {
			str += c;
			if (TOKEN_INVALID == t)
				t = TOKEN_INT;
			else if (t != TOKEN_INT && t != TOKEN_FLOAT && t != TOKEN_KEYWORD) {
				std::string msg("MD5Model::getNextToken(): invalid token '");
				msg += str + "'";
				throw Exception(msg);
			}
		}
		if ('-' == c) {
			str += c;
			if (TOKEN_INVALID == t)
				t = TOKEN_INT;
			else {
				std::string msg("MD5Model::getNextToken(): invalid token '");
				msg += str + "'";
				throw Exception(msg);
			}
		}
		if (isalpha(c)) {
			str += c;
			if (TOKEN_INVALID == t)
				t = TOKEN_KEYWORD;
			else if (t != TOKEN_KEYWORD) {
				std::string msg("MD5Model::getNextToken(): invalid token '");
				msg += str + "'";
				throw Exception(msg);
			}
		}
		if ('"' == c) {
			// treat as a delimeter if already reading in a token
			if (t != TOKEN_INVALID) {
				fin.putback(c);
				if (tokStr != NULL)
					(*tokStr) = str;
				return t;
			}
			readString(fin, str);

			if (tokStr != NULL)
				(*tokStr) = str;

			return TOKEN_STRING;
		}
		if ('.' == c) {
			str += c;
			if (t != TOKEN_INT) {
				std::string msg("MD5Model::getNextToken(): invalid token '");
				msg += str + "'";
				throw Exception(msg);
			}
			t = TOKEN_FLOAT;
		}
		if ('/' == c) {
			// treat as a delimeter if already reading in a token
			if (t != TOKEN_INVALID) {
				if (tokStr != NULL)
					(*tokStr) = str;
				return t;
			}

			skipComments(fin);
			skipWhitespace(fin);
			continue;
		}

		// treat whitespace as a delimeter
		if (IS_WHITESPACE(c)) {
			if (tokStr != NULL)
				(*tokStr) = str;
			return t;
		}

		// at this point token type should be set, if it hasn't been then
		// token is invalid
		if (TOKEN_INVALID == t) {
			std::string msg("MD5Model::getNextToken(): invalid token '");
			str += c;
			msg += str + "'";
			throw Exception(msg);
		}
	}

	return TOKEN_INVALID;
}


// builds mesh when no animation has been set
void MD5Model::buildVerts() {
	for (size_t i = 0; i < meshes.size(); i++) {
		for (size_t j = 0; j < meshes[i]->verts.size(); j++) {
			Vertex &v = meshes[i]->verts[j];
			v.pos[0] = v.pos[1] = v.pos[2] = 0.0;

			for (int k = 0; k < v.weightCount; k++) {
				Weight &w = meshes[i]->weights[v.weightIndex + k];
				Joint &joint = joints[w.joint];

				Quaternion q(w.pos[0], w.pos[1], w.pos[2], 0.0f);
				Quaternion result = joint.quat*q*joint.quat.conjugate();
				v.pos[0] += (joint.pos[0] + result[0])*w.w;
				v.pos[1] += (joint.pos[1] + result[1])*w.w;
				v.pos[2] += (joint.pos[2] + result[2])*w.w;
				// d3d reverse
				//v.pos[1] = -v.pos[1];
			} // for (weights)
		} // for (mesh vertices)
	} // for (meshes)
}

void MD5Model::calculateRenderData()
{
	int nVertexIndex = 0;
	// ������������Ǹ������񣬶���D3D�������鶼Ҫ�������������������ε�������Ҫ������
	for (size_t i = 0; i < meshes.size(); i++)
	{
		for (size_t j = 0; j < meshes[i]->verts.size(); j++)
		{
			for (int k = 0; k < 3; k++)
			{
				m_pRenderVertex[nVertexIndex].pos[k] = meshes[i]->verts[j].pos[k];
				m_pRenderVertex[nVertexIndex].n[k] = meshes[i]->verts[j].n[k];
			}
			nVertexIndex++;
		}
	}
}

void MD5Model::buildVerts(Frame &frame) {
	for (size_t i = 0; i < meshes.size(); i++) {
		for (size_t j = 0; j < meshes[i]->verts.size(); j++) {
			Vertex &v = meshes[i]->verts[j];
			v.pos[0] = v.pos[1] = v.pos[2] = 0.0;

			for (int k = 0; k < v.weightCount; k++) {
				Weight &w = meshes[i]->weights[v.weightIndex + k];
				// �����������ڵ㣬�ǹؼ�֡N�������ڵ㣬ͨ�����ڵ��ϵ����JointInfo�Ĺ�ϵ��������ģ�
				// �Ǿ�����֡��ʼ֡���͸��ڵ���ת��������� λ�úͷ�λ��
				Joint &joint = frame.joints[w.joint];

				Quaternion q(w.pos[0], w.pos[1], w.pos[2], 0.0f);
				// �����ڵ�ķ�λ������ڸ��ڵ�ģ�Ҳ�������ģ������ϵ�ģ��Զ��������weight��������ת�任
				Quaternion result = joint.quat*q*joint.quat.conjugate();
				// �����ڵ��λ�����Ѿ�ת��������ڸ��ڵ�ģ�Ҳ�������ģ������ϵ�İ�����ƽ�ƺ���ת
				// joint.pos������ڸ��ڵ������ϵ����Ȼ����������ת�任����result[0]�������ģ��meshλ�õ�ģ������ϵ
				// weight������ڹ������ڵ��λ�ã�����Ҫ��weight����joint��λ�ı任�Ϳ����ˡ�
				v.pos[0] += (joint.pos[0] + result[0])*w.w;
				v.pos[1] += (joint.pos[1] + result[1])*w.w;
				v.pos[2] += (joint.pos[2] + result[2])*w.w;
			} // for (weights)
		} // for (mesh vertices)
	} // for (meshes)
}


void MD5Model::buildNormals() {
	// zero out the normals
	for (size_t i = 0; i < meshes.size(); i++) {
		for (size_t j = 0; j < meshes[i]->verts.size(); j++) {
			meshes[i]->verts[j].n[0] = 0.0;
			meshes[i]->verts[j].n[1] = 0.0;
			meshes[i]->verts[j].n[2] = 0.0;
		}

		// for each normal, add contribution to normal from every face that vertex
		// is part of
		for (size_t j = 0; j < meshes[i]->tris.size(); j++) {
			Vertex &v0 = meshes[i]->verts[meshes[i]->tris[j].v[0]];
			Vertex &v1 = meshes[i]->verts[meshes[i]->tris[j].v[1]];
			Vertex &v2 = meshes[i]->verts[meshes[i]->tris[j].v[2]];

			float Ax = v1.pos[0] - v0.pos[0];
			float Ay = v1.pos[1] - v0.pos[1];
			float Az = v1.pos[2] - v0.pos[2];

			float Bx = v2.pos[0] - v0.pos[0];
			float By = v2.pos[1] - v0.pos[1];
			float Bz = v2.pos[2] - v0.pos[2];

			float nx = Ay * Bz - By * Az;
			float ny = -(Ax * Bz - Bx * Az);
			float nz = Ax * By - Bx * Ay;

			v0.n[0] += nx;
			v0.n[1] += ny;
			v0.n[2] += nz;

			v1.n[0] += nx;
			v1.n[1] += ny;
			v1.n[2] += nz;

			v2.n[0] += nx;
			v2.n[1] += ny;
			v2.n[2] += nz;
		}

		// normalize each normal
		for (size_t j = 0; j < meshes[i]->verts.size(); j++) {
			Vertex &v = meshes[i]->verts[j];

			float mag = (float)sqrt(float(v.n[0] * v.n[0] + v.n[1] * v.n[1] + v.n[2] * v.n[2]));

			// avoid division by zero
			if (mag > 0.0001f) {
				v.n[0] /= mag;
				v.n[1] /= mag;
				v.n[2] /= mag;
			}
		}
	} // for (meshes)
}


// ÿ��KeyFrame�е�ÿ�������ڵ��λ�úͷ�λ���������ˣ�����hierarchy JointInfo��baseframe BaseJoint��Frame.
void MD5Model::buildFrames(Anim &anim) {
	for (int i = 0; i < anim.numFrames; i++) {
		// allocate storage for joints for this frame
		// ÿ֡ȷʵ����ô��Ĺ������պ���֡�е�����
		anim.frames[i].joints.resize(numJoints);
		// ÿ֡�µ�ÿ�������ڵ�
		for (int j = 0; j < numJoints; j++) {
			const Joint &joint = anim.baseJoints[j];

			float pos[3] = { joint.pos[0], joint.pos[1], joint.pos[2] };
			float orient[3] = { joint.quat[0], joint.quat[1], joint.quat[2] };

			// ��components�еĹ����ڵ�λ�úͷ�λ��ȡ������û����ȡbaseJoint��λ��
			// λ��
			int n = 0;
			for (int k = 0; k < 3; k++)
			{
				// 1����k,��1 10 100
				int nMoveK = 1 << k;
				// flags�Ǵ�keyFrame animatedComponents��ȡ��joint��Ϣ��λ��־��û�иñ�־��ôλ�úͷ�λ��ȡbaseFrame��
				if (anim.jointInfo[j].flags & (nMoveK/*1 << k*/)) {
					// startIndex��һ֡�иù����ڵ�posλ�õĿ�ʼ������ȡ��3��pos
					pos[k] = anim.frames[i].animatedComponents[anim.jointInfo[j].startIndex + n];
					n++;
				}
			}
			// ��λ
			for (int k = 0; k < 3; k++)
			{
				// 1����k,��1000 10000 100000
				int nMoveK = 8 << k;
				if (anim.jointInfo[j].flags & (nMoveK/*8 << k*/)) {
					orient[k] = anim.frames[i].animatedComponents[anim.jointInfo[j].startIndex + n];
					n++;
				}
			}

			Quaternion q = buildQuat(orient[0], orient[1], orient[2]);

			Joint &frameJoint = anim.frames[i].joints[j];
			frameJoint.name = anim.jointInfo[j].name;
			// ָ���˸��ڵ�����
			frameJoint.parent = anim.jointInfo[j].parentIndex;

			// root joint? ���ڵ���λ�úͷ�λ��Ϣ
			if (anim.jointInfo[j].parentIndex < 0) {
				frameJoint.pos[0] = pos[0];
				frameJoint.pos[1] = pos[1];
				frameJoint.pos[2] = pos[2];
				frameJoint.quat = q;
			}
			else {
				Joint &parent = anim.frames[i].joints[anim.jointInfo[j].parentIndex];

				// rotate position (qp is quaternion representation of position)
				Quaternion qp(pos[0], pos[1], pos[2], 0.0f);
				// ��Ϊ.md5anim�ļ��У�����jointInfo���򣬶���parentIndex��С����ģ�
				// ���Ե����ڵ�Ҳ��֦�ڵ�ʱ��֦�ڵ����Ѿ����������ڸ��ڵ�ı任�˵�, ����ֻ��Ҫ��������ǰ�ĸ��ڵ㼴�ɡ�
				// ������������ϵ����ʱ��ת������Ϊ q * v * (q^-1), ��������ϵ�е�˳ʱ��ת��Ϊ(q^-1)* v * q.
				// qp������ڸ��ڵ�ģ����Ը��ڵ��qp λ�ý�������ת�任��
				Quaternion result = parent.quat*qp*parent.quat.conjugate();
				frameJoint.pos[0] = result[0] + parent.pos[0];
				frameJoint.pos[1] = result[1] + parent.pos[1];
				frameJoint.pos[2] = result[2] + parent.pos[2];
				// �������������Ѳ����Ǽ��عؼ�֡���ݼ��������λ�ã�Ȼ�����������Ӧ�õ�������(��������Mvo*(Mbo^-1)*Mbn)��
				// ���������У�1.���ӱ任��ϵ���ۻ��任�� 2.һ��������������������Ҫ�ۼ�Ȩ�ء�3.ƽ�ƣ����ţ���ת���ֱ任�ķֽ���ۺϡ�
				// 4.�������ϵ��������ϵ���ת����ϵ��ʵ�������еı任����ģ�ʹ���
				// �ڵ�����任�ǶԺ���ڵ����Ӱ�죬����λ��������λ�ú͸��ڵ�任Ӱ�죬
				// �ڵ�����任�Ǹ��ڵ������任���ۻ�������Ӱ���ӽڵ㡣��Ϊ���ڵ�Ҳ����λ�õģ���������ģ������ϵ����ô
				// �����任�������ǻ���ģ������ϵ�ġ�
				// �����ӽڵ���Ҫ�ܵ����ڵ㣨���ۻ����任����Ҫ���ϸ��ڵ�λ�ã��Ƿ�˫���ۻ��ˣ� û��˫���ۻ�����ô����أ�
				// 1)�����ڵ�ı任�������������ӽڵ�ġ�
				// 2)�任�ۼ�ЧӦ�ǹ����ڵ�ĺ����ӽڵ㶼���ܵ�Ӱ�죬��Ҫ���б任��
				// 3)�������ϵ���⣬����ӽڵ���������ĸ��ڵ���ôֻ�Ǳ任��һ�Σ��������ڸ��ڵ�ĸ��ڵ���ô�ۼ������Σ��Դ����ơ�
				// ��ת�����Ż������ۼӣ�ƽ�Ʋ��������ۼӣ��ӽڵ�ƽ�ƻ��Ѿ�����һ�㲻ƽ�ơ�
				// �ܽ᣺��ת��Ҫ������ȥ�������ۼƶ��Ƿֱ����任��ӣ���תΪ��(parentNodeOritation*position) + ParentNodePos;
				// ����parentNodeOritation���ۼ����и��ڵ�ķ�λ��ת������Ҳ�����Ƶ�һ��������Ҫ�ۻ��ı���ͬ�����Ĵ�С���
				// ƽ���ǲ��ɴ��ݵģ����ڵ���ƽ����ô�ӽڵ㲻������Ը��ڵ��ƽ�ƣ�ֱ�Ӽ��ϸ��ڵ�λ�ü��ɣ�
				// �ڼ����ӹ���λ��ʱ��ͼ�������λ��ʱ���ã��ӹ���������������ڱ��ؽڵ������λ�ã���
				// �����ӽڵ�λ����Ը��ڵ㣺mDerivedPosition = parentOrientation * (parentScale * mPosition) +  mParent->_getDerivedPosition();
				//               mDerivedOrientation = parentOrientation * mOrientation;
				//               mDerivedScale = parentScale * mScale;
				// ���񶥵�����������ӽڵ㣺 
				//void MD5Model::buildVerts() {
				//	for (size_t i = 0; i < meshes.size(); i++) {
				//		for (size_t j = 0; j < meshes[i]->verts.size(); j++) {
				//			Vertex &v = meshes[i]->verts[j];
				//			v.pos[0] = v.pos[1] = v.pos[2] = 0.0;

				//			for (int k = 0; k < v.weightCount; k++) {
				//				Weight &w = meshes[i]->weights[v.weightIndex + k];
				//				Joint &joint = joints[w.joint];

				//				Quaternion q(w.pos[0], w.pos[1], w.pos[2], 0.0f);
				//				Quaternion result = joint.quat*q*joint.quat.conjugate();
				//				v.pos[0] += (joint.pos[0] + result[0])*w.w;
				//				v.pos[1] += (joint.pos[1] + result[1])*w.w;
				//				v.pos[2] += (joint.pos[2] + result[2])*w.w;
				//			} // for (weights)
				//		} // for (mesh vertices)
				//	} // for (meshes)
				//}
				// OGRE����_getOffsetTransform������󶨵��ڵ�ı��ر任��ȡ��
				// Ȼ������Mesh::softwareVertexBlend����Ӧ�õ�������

				// store orientation of this joint
				// ��λҲ������ڸ��ڵ�ķ�λ��������Ҫ���ڵ���������任��������λ��һ��λ�ã���(a*b)v(��a*b)^-1)��ʽ��Ӧ���ǣ�
				// ��������ϵ�е�˳ʱ��ת��Ϊ(q^-1)* v * q �ı任����Ϊ��((ab)^-1)v(ab)�����Դ�ŵ���Ԫ��������Ԫ����

				// 5.��Ƥ���������任��ı任����Ϊ�����ڵ������ģ������ϵԭ�㣬���Զ����ͬ�������������ı任��������������һ������Ϳ��Թ���ǰ����������
				// ������ȷ�����������������ϵ�����񶥵�Ψһ�õı任�������Լ���mBoneMatricesǰÿ�������ڵ���Ҫȥ�����ڵ��
				// ���ţ���ת����ƽ�Ʊ任��Ȼ����ù����ڵ�����ڸ��ڵ�ı任����mBoneMatrices��������Ƥ������mBoneMatrices�任��
				// ����Ҫ���ݹ����ĸ��ڵ�Ҳ����Entity�����б任��
				frameJoint.quat = parent.quat*q;
				frameJoint.quat.normalize();
			} // else
		} // for
	} // for
}


Quaternion MD5Model::buildQuat(float x, float y, float z) const {
	// compute the 4th component of the quaternion
	float w = 1.0f - x*x - y*y - z*z;
	/*http://www.braynzarsoft.net/index.php?p=D3D11MD51 MD5��ʽ�������Ԫ��ת���ĸ�ʽ
	Knowing this, we can compute the w component for the quaternion like this:
	float t = 1.0f - (x * x) - (y * y) - (z * z);
	if (t < 0.0f)
	w = 0.0f;
	else
	w = -sqrtf(t);*/
	// ���ǽ���������ϵ����ʱ����ת��Ϊ��˳ʱ��任������������³��Թ����ڵ��ı任����Ԫ����, ������ʱ�����ԭ�ɣ�
	// �պ���(-w,v)�ڱ任����ʱ��ȼ��ڣ�w,-v)Ҳ���Ǹ���Ԫ����
	w = w < 0.0 ? 0.0f : (float)-sqrt(double(w));

	Quaternion q(x, y, z, w);// �ȼ���w= sqrt(w)ʱ���q(-x, -y, -z, w)
	q.normalize();

	return q;
}

MD5Model::Anim::Anim() :
numFrames(0),
frameRate(0),
numAnimatedComponents(0) {
}

RenderVertex* MD5Model::getRenderVertex(int &nSize)
{
	nSize = m_nRenderVertexCount;
	return m_pRenderVertex;
}

RenderVertexIndex* MD5Model::getRenderVertexIndex(int &nSize)
{
	nSize = m_nRenderIndexCount;
	return m_pRenderVertexIndex;
}

