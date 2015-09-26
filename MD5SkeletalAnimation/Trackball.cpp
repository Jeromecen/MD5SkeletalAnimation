
// Quaternion Trackball, by A.J. Tavakoli

#include "Trackball.h"

/////////////////////////////////////

// move() should be called whenever the mouse is moving
void Trackball::move(int x, int y, int w, int h) {
	Vector3 currPos, d;

	// map x, y onto hemi-sphere
	pToV(x, y, w, h, currPos);

	// find change in position since last call
	d = currPos - lastPos;

	// create a quaternion to represent the rotation
	Quaternion currQuat;

	if (fabsf(d.c[0]) > 0.01f || fabs(d.c[1]) > 0.01f || fabsf(d.c[2]) > 0.01f) {
		// compute angle
		float angle = (float)acos(double(lastPos*currPos / (lastPos.mag()*currPos.mag())));

		// convert angle from radians to degrees
		angle *= 180.0f / 3.14f;

		// calculate axis to rotate about from last position vector and
		// current position vector
		Vector3 axis;
		axis.cross(lastPos, currPos);

		// set last position
		lastPos = currPos;

		// normalize axis of rotation
		axis.normalize();

		// build a quaternion from the axis, angle pair
		currQuat.buildFromAxisAngle(axis.c, angle);

		// store all rotations up to this point in
		// q
		q = currQuat*q;
	} // if
} // Trackball::move()

//////////////////////////////////

// stores trackball's transformation matrix in mat.
void Trackball::look(float *mat) {
	q.toMatrix(mat);
} // Trackball::look()

//////////////////////////////////

// project cursor position (x and y) onto trackball hemi-sphere
void Trackball::pToV(int x, int y, int w, int h, Vector3 &p) {
	float fX = (float)x, fY = (float)y,
		fW = (float)w, fH = (float)h;

	p.c[0] = (2.0f*fX - fW) / fW;
	p.c[1] = (fH - 2.0f*fY) / fH;
	//p.c[1] = (2.0f*fY - fH) / fH;

	// find distance from origin to selected x, y
	float d = (float)sqrt(double(p.c[0] * p.c[0] + p.c[1] * p.c[1]));

	// Find height of selected point on hemi-sphere.  If
	// the cursor is outside of the hemi-sphere (distance of
	// selected x, y from origin is greater than 1) just use
	// a distance of 1.  The cosine of this distance*PI/2 gives
	// the height of the selected point.
	p.c[2] = (float)cos(double((PI / 2.0f) * ((d < 1.0f) ? d : 1.0f)));

	p.normalize();
} // Trackball::pToV()

//////////////////////////////////

// This function should be called when you are starting
// to move the trackball.  x and y are the current cursor
// coordinates. w and h are the window dimensions.
void Trackball::startMotion(int x, int y, int w, int h) {
	pToV(x, y, w, h, lastPos);
} // Trackball::startMotion()

////////////////////////////
