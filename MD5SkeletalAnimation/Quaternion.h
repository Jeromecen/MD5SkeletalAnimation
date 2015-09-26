
#ifndef __Quaternion_h_
#define __Quaternion_h_

// This class stores a quaternion.
class Quaternion {
public:
	float c[4];

	Quaternion();
	Quaternion(float x, float y, float z, float w);

	// overloaded operators for quaternion multiplication
	Quaternion operator *(const Quaternion &q) const;
	const Quaternion & operator *= (const Quaternion &q);

	void buildFromAxisAngle(const float v[], float angle);
	inline Quaternion conjugate() const { return Quaternion(-c[0], -c[1], -c[2], c[3]); }
	float length() const;
	void normalize();
	Quaternion pow(float t);
	void toMatrix(float mat[]) const;

	inline float& operator [] (int i)       { return c[i]; }
	inline float  operator [] (int i) const { return c[i]; }
}; // Quaternion

// 标准的球面插值
Quaternion slerp(const Quaternion &q1, const Quaternion &q2, float t);


#endif
