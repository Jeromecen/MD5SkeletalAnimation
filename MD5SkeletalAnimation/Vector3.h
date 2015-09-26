
#ifndef __Vector3_h__
#define __Vector3_h__

// minimal vector class
class Vector3 {
public:
	// components of vector
	float c[3];

	Vector3() { }
	Vector3(float x, float y, float z) {
		c[0] = x;
		c[1] = y;
		c[2] = z;
	}

	inline Vector3 operator - (const Vector3 &v) const {
		return Vector3(c[0] - v.c[0], c[1] - v.c[1], c[2] - v.c[2]);
	} // operator -

	// dot product
	inline float operator *(const Vector3 &v) const {
		return c[0] * v.c[0] + c[1] * v.c[1] + c[2] * v.c[2];
	} // operator *

	// sets this vector to the 
	// cross product of u and v
	inline void cross(const Vector3 & u,
		const Vector3 & v) {
		c[0] = u.c[1] * v.c[2] - u.c[2] * v.c[1];
		c[1] = -1.0f*(u.c[0] * v.c[2] - u.c[2] * v.c[0]);
		c[2] = u.c[0] * v.c[1] - u.c[1] * v.c[0];
	} // cross()

	// returns magnitude of vector
	inline float mag() const {
		return (float)sqrt(double(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]));
	} // mag()

	// normalize vector
	void normalize() {
		float m = mag();
		c[0] /= m;
		c[1] /= m;
		c[2] /= m;
	} // normalize()
}; // Vector3

#endif
