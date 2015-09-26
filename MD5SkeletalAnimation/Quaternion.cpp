
#include <cmath>
#include "Quaternion.h"


///////////////////////////

Quaternion::Quaternion() {
	c[0] = c[1] = c[2] = 0.0f;
	c[3] = 1.0f;
}

///////////////////////////

Quaternion::Quaternion(float x, float y, float z, float w) {
	c[0] = x;
	c[1] = y;
	c[2] = z;
	c[3] = w;
}

///////////////////////////

// normalizes the quaternion
void Quaternion::normalize() {
	float len = length();
	c[0] /= len;
	c[1] /= len;
	c[2] /= len;
	c[3] /= len;
} // normalize()

///////////////////////////

// multiplies two quaternions and returns the result
Quaternion Quaternion::operator *(const Quaternion &q) const {
	Quaternion r;

	r.c[0] = c[3] * q.c[0] + c[0] * q.c[3] + c[1] * q.c[2] - c[2] * q.c[1];
	r.c[1] = c[3] * q.c[1] + c[1] * q.c[3] + c[2] * q.c[0] - c[0] * q.c[2];
	r.c[2] = c[3] * q.c[2] + c[2] * q.c[3] + c[0] * q.c[1] - c[1] * q.c[0];
	r.c[3] = c[3] * q.c[3] - c[0] * q.c[0] - c[1] * q.c[1] - c[2] * q.c[2];

	return r;
} // Quaternion::operator *

///////////////////////////

// sets the value of a quaternion to
// the result of itself multiplied by
// another quaternion, q
const Quaternion& Quaternion::operator *=(const Quaternion &q) {
	// temporary values
	float tx, ty, tz, tw;

	tw = c[3] * q.c[3] - c[0] * q.c[0] - c[1] * q.c[1] - c[2] * q.c[2];
	tx = c[3] * q.c[0] + c[0] * q.c[3] + c[1] * q.c[2] - c[2] * q.c[1];
	ty = c[3] * q.c[1] + c[1] * q.c[3] + c[2] * q.c[0] - c[0] * q.c[2];
	tz = c[3] * q.c[2] + c[2] * q.c[3] + c[0] * q.c[1] - c[1] * q.c[0];

	c[0] = tx; c[1] = ty; c[2] = tz; c[3] = tw;

	return (*this);
} // Quaternion::opertor *=

///////////////////////////

// convert quaternion to a 4x4 rotation matrix
void Quaternion::toMatrix(float matrix[]) const {
	// column 1
	matrix[0] = 1.0f - 2.0f * (c[1] * c[1] + c[2] * c[2]);
	matrix[1] = 2.0f * (c[0] * c[1] + c[2] * c[3]);
	matrix[2] = 2.0f * (c[0] * c[2] - c[1] * c[3]);
	matrix[3] = 0.0f;

	// column 2
	matrix[4] = 2.0f * (c[0] * c[1] - c[2] * c[3]);
	matrix[5] = 1.0f - 2.0f * (c[0] * c[0] + c[2] * c[2]);
	matrix[6] = 2.0f * (c[2] * c[1] + c[0] * c[3]);
	matrix[7] = 0.0f;

	// column 3
	matrix[8] = 2.0f * (c[0] * c[2] + c[1] * c[3]);
	matrix[9] = 2.0f * (c[1] * c[2] - c[0] * c[3]);
	matrix[10] = 1.0f - 2.0f * (c[0] * c[0] + c[1] * c[1]);
	matrix[11] = 0.0f;

	// column 4
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1.0f;
} // Quaternion::toMatrix()

///////////////////////////

// construct quaternion from an axis/angle pair
void Quaternion::buildFromAxisAngle(const float v[], float angle) {
	// convert to radians (angle should be in degrees)
	float radians = (angle / 180.0f)*3.14159f;

	// cache this, since it is used multiple times below
	float sinThetaDiv2 = (float)sin(double(radians / 2.0f));

	// now calculate the components of the quaternion	
	c[0] = v[0] * sinThetaDiv2;
	c[1] = v[1] * sinThetaDiv2;
	c[2] = v[2] * sinThetaDiv2;

	c[3] = (float)cos(double(radians / 2.0f));
	// 旋转需要翻转
	/*c[0] = -c[0];
	c[1] = -c[1];
	c[2] = -c[2];*/
} // Quaternion::buildFromAxisAngle()

///////////////////////////

Quaternion Quaternion::pow(float t) {
	Quaternion result = (*this);

	if (fabs(double(c[13])) < 0.9999) {
		float alpha = (float)acos(double(c[3]));
		float newAlpha = alpha*t;

		result.c[3] = (float)cos(double(newAlpha));
		float fact = float(sin(double(newAlpha)) / sin(double(alpha)));
		result.c[0] *= fact;
		result.c[1] *= fact;
		result.c[2] *= fact;
	}

	return result;
} // Quaternion::pow()

///////////////////////////

// returns the magnitude of the quaternion
float Quaternion::length() const {
	return (float)sqrt(double(c[0] * c[0] + c[1] * c[1] + c[2] * c[2] + c[3] * c[3]));
} // Quaternion::length()

///////////////////////////

Quaternion slerp(const Quaternion &q1, const Quaternion &q2, float t) {
	Quaternion result, _q2 = q2;

	float cosOmega = q1.c[3] * q2.c[3] + q1.c[0] * q2.c[0] + q1.c[1] * q2.c[1] + q1.c[2] * q2.c[2];

	if (cosOmega < 0.0f) {
		_q2.c[0] = -_q2.c[0];
		_q2.c[1] = -_q2.c[1];
		_q2.c[2] = -_q2.c[2];
		_q2.c[3] = -_q2.c[3];
		cosOmega = -cosOmega;
	}

	float k0, k1;
	if (cosOmega > 0.99999f) {
		k0 = 1.0f - t;
		k1 = t;
	}
	else {
		float sinOmega = (float)sqrt(double(1.0f - cosOmega*cosOmega));
		float omega = (float)atan2(double(sinOmega), double(cosOmega));

		float invSinOmega = 1.0f / sinOmega;

		k0 = float(sin(double((1.0f - t)*omega)))*invSinOmega;
		k1 = float(sin(double(t*omega)))*invSinOmega;
	}

	for (int i = 0; i < 4; i++)
		result[i] = q1[i] * k0 + _q2[i] * k1;

	return result;
}

///////////////////////////
