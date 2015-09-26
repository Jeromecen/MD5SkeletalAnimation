
// Quaternion Trackball, by A.J. Tavakoli

#ifndef TRACKBALL_H
#define TRACKBALL_H
#include <cmath>
#include "Vector3.h"
#include "Quaternion.h"

#define PI 3.14159265f
class Trackball {
public:
	Trackball() : lastPos(0.0f, 0.0f, 0.0f) { }
	~Trackball() { }

	void move(int x, int y, int w, int h);
	void look(float *mat);
	void startMotion(int x, int y, int w, int h);
private:
	void pToV(int x, int y, int w, int h, Vector3 &p);

	Vector3 lastPos;
	Quaternion q;
}; // Trackball


#endif
