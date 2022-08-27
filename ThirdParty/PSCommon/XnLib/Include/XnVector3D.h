/*****************************************************************************
*                                                                            *
*  PrimeSense PSCommon Library                                               *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of PSCommon.                                            *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#ifndef _XN_VECTOR3D_H_
#define _XN_VECTOR3D_H_

#include "XnLib.h"
#include "XnMath.h"

namespace xnl
{
struct Point3D
{
	Point3D() : x(0), y(0), z(0) {}
	Point3D(const Point3D& other) : x(other.x), y(other.y), z(other.z) {}
	Point3D(XnFloat x, XnFloat y, XnFloat z) : x(x), y(y), z(z) {}
	Point3D& operator=(const Point3D& other) {x = other.x; y = other.y; z = other.z; return *this;}

	XnFloat x;
	XnFloat y;
	XnFloat z;
};

class Vector3D : public Point3D
{
public:
	static Vector3D Zero();
	static Vector3D All(XnFloat f);
	static Vector3D Up();
	static Vector3D Down();
	static Vector3D Right();
	static Vector3D Left();

	inline Vector3D();
	inline Vector3D(XnFloat x, XnFloat y, XnFloat z);
	inline Vector3D(const Point3D& point);
	inline Vector3D(const Vector3D& other);

	inline Vector3D& operator=(const Point3D& point);
	inline Vector3D& operator=(const Vector3D& other);

	inline XnFloat operator[](XnUInt32 index) const;
	inline XnFloat& operator[](XnUInt32 index);

	inline Vector3D& Set(const Vector3D& other);
	inline Vector3D& Set(const Point3D& point);
	inline Vector3D& Set(XnFloat x, XnFloat y, XnFloat z);

	inline Vector3D& SetZero();
	inline XnBool IsZero() const;

	inline XnBool operator==(const Vector3D& other) const;
	inline XnBool operator!=(const Vector3D& other) const;

	inline Vector3D operator-() const;
	inline Vector3D& Negate(const Vector3D& other);
	inline Vector3D& Negate();

	friend inline Vector3D operator*(XnFloat f, const Vector3D& other);
	inline Vector3D operator*(XnFloat f) const;
	inline Vector3D& operator*=(XnFloat f);
	inline Vector3D& Multiply(XnFloat f, const Vector3D& other);
	inline Vector3D& Multiply(const Vector3D& other, XnFloat f);

	inline Vector3D operator/(XnFloat f) const;
	inline Vector3D& operator/=(XnFloat f);
	inline Vector3D& Divide(const Vector3D& other, XnFloat f);

	inline Vector3D operator+(const Vector3D& other) const;
	inline Vector3D& operator+=(const Vector3D& other);
	inline Vector3D& Add(const Vector3D& left, const Vector3D& right);
	
	inline Vector3D operator-(const Vector3D& other) const;
	inline Vector3D& operator-=(const Vector3D& other);
	inline Vector3D& Subtract(const Vector3D& left, const Vector3D& right);

	inline XnFloat Magnitude() const;
	inline XnFloat MagnitudeSquared() const;

	inline XnFloat Distance(const Vector3D& other) const;
	inline XnFloat DistanceSquared(const Vector3D& other) const;

	inline XnFloat Normalize();

	inline Vector3D OrthogonalVector() const;
	inline Vector3D UnitOrthogonalVector() const;

	Vector3D ProjectedOnUnitVector(const Vector3D &unitVector) const
	{
		return DotProduct(*this,unitVector)*unitVector;
	}

	Vector3D ProjectedOnPlane(const Vector3D &unitNormal) const;

	// round vector terms
	Vector3D Round (void) const
	{
		return Vector3D((float)round(x), (float)round(y), (float)round(z)) ;
	}

	inline Vector3D operator^(const Vector3D& other);
	inline Vector3D& CrossProduct(const Vector3D& left, const Vector3D& right);

	inline XnFloat operator|(const Vector3D& other);
	friend inline XnFloat DotProduct(const Vector3D& left, const Vector3D& right);

	inline Vector3D& Interpolate(const Vector3D& vec1, const Vector3D& vec2, XnFloat alpha);

	inline XnBool IsSameDirection(const Vector3D& other) const;

	inline XnFloat GetTolerance() const;
	inline void SetTolerance(XnFloat tolerance);

protected:
	XnFloat m_tolerance;
};

Vector3D::Vector3D() : m_tolerance(1e-5f) {Set(0,0,0);}
Vector3D::Vector3D(XnFloat x, XnFloat y, XnFloat z) : m_tolerance(1e-5f) {Set(x, y, z);}
Vector3D::Vector3D(const Vector3D& other) {Set(other);}
Vector3D::Vector3D(const Point3D& point) {Set(point);}

Vector3D& Vector3D::operator=(const Vector3D& other) {m_tolerance = other.m_tolerance; return Set(other.x, other.y, other.z);}
Vector3D& Vector3D::operator=(const Point3D& point) {m_tolerance = 1e-5f; return Set(point.x, point.y, point.z);}

XnFloat Vector3D::operator[](XnUInt32 index) const
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	}
	XN_ASSERT(false);
	return z;
}
XnFloat &Vector3D::operator[](XnUInt32 index)
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	}
	XN_ASSERT(false);
	return z;
}

Vector3D& Vector3D::Set(const Vector3D& other) {*this = other; return *this;}
Vector3D& Vector3D::Set(const Point3D& point) {*this = point; return *this;}
Vector3D& Vector3D::Set(XnFloat _x, XnFloat _y, XnFloat _z) {this->x = _x; this->y = _y; this->z = _z; return *this;}

Vector3D& Vector3D::SetZero() {return Set(Zero());}
XnBool Vector3D::IsZero() const {return *this==Zero();}
//	{return (fabs(x) < m_tolerance && fabs(y) < m_tolerance && fabs(z) < m_tolerance);}

XnBool Vector3D::operator==(const Vector3D& other) const {return (Math::Abs(x-other.x) < m_tolerance && Math::Abs(y-other.y) < m_tolerance && Math::Abs(z-other.z) < m_tolerance);}
XnBool Vector3D::operator!=(const Vector3D& other) const {return !this->operator==(other);}

Vector3D Vector3D::operator-() const {return Vector3D(-x, -y, -z);}
Vector3D& Vector3D::Negate(const Vector3D& other) {return Set(-other.x, -other.y, -other.z);}
Vector3D& Vector3D::Negate() {return Set(-x, -y, -z);}

Vector3D operator*(XnFloat f, const Vector3D& rhs) {return Vector3D(f*rhs.x, f*rhs.y, f*rhs.z);}
Vector3D Vector3D::operator*(XnFloat f) const {return Vector3D(x*f, y*f, z*f);}
Vector3D& Vector3D::operator*=(XnFloat f) {return Set(x*f, y*f, z*f);}
Vector3D& Vector3D::Multiply(XnFloat f, const Vector3D& other) {return Set(f*other.x, f*other.y, f*other.z);}
Vector3D& Vector3D::Multiply(const Vector3D& other, XnFloat f) {return Set(other.x*f, other.y*f, other.z*f);}

Vector3D Vector3D::operator/(XnFloat f) const {return Vector3D(x/f, y/f, z/f);}
Vector3D& Vector3D::operator/=(XnFloat f) {return Set(x/f, y/f, z/f);}
Vector3D& Vector3D::Divide(const Vector3D& other, XnFloat f) {return Set(other.x/f, other.y/f, other.z/f);}

Vector3D Vector3D::operator+(const Vector3D& other) const {return Vector3D(x+other.x, y+other.y, z+other.z);}
Vector3D& Vector3D::operator+=(const Vector3D& other) {return Set(x+other.x, y+other.y, z+other.z);}
Vector3D& Vector3D::Add(const Vector3D& left, const Vector3D& right) {return Set(left.x+right.x, left.y+right.y, left.z+right.z);}

Vector3D Vector3D::operator-(const Vector3D& other) const {return Vector3D(x-other.x, y-other.y, z-other.z);}
Vector3D& Vector3D::operator-=(const Vector3D& other) {return Set(x-other.x, y-other.y, z-other.z);}
Vector3D& Vector3D::Subtract(const Vector3D& left, const Vector3D& right) {return Set(left.x-right.x, left.y-right.y, left.z-right.z);}

XnFloat Vector3D::Magnitude() const { return Math::Sqrt(MagnitudeSquared()); }
XnFloat Vector3D::MagnitudeSquared() const {return x*x+y*y+z*z;}

XnFloat Vector3D::Distance(const Vector3D& other) const {return Math::Sqrt(DistanceSquared(other));}
XnFloat Vector3D::DistanceSquared(const Vector3D& other) const {return (*this-other).MagnitudeSquared();}

XnFloat Vector3D::Normalize()
{
	XnFloat length = Magnitude();
	if (length > m_tolerance)
		*this /= length;
	else
		Set(1,0,0);
	return length;
}

Vector3D Vector3D::OrthogonalVector() const
{
	XnFloat absX = Math::Abs(x), absY = Math::Abs(y), absZ = Math::Abs(z);

	if (absX < absY)
		if (absX < absZ)
			return Vector3D(0, z, -y);
		else
			return Vector3D(y, -x, 0);
	else
		if (absY < absZ)
			return Vector3D(z, 0, x);
		else
			return Vector3D(y, -x, 0);
}
Vector3D Vector3D::UnitOrthogonalVector() const
{
	Vector3D vec = OrthogonalVector();
	vec.Normalize();
	return vec;
}

Vector3D Vector3D::operator^(const Vector3D& other)
{
	Vector3D result;
	result.CrossProduct(*this, other);
	return result;
}
Vector3D& Vector3D::CrossProduct(const Vector3D& left, const Vector3D& right)
{
	return Set(left.y*right.z - left.z*right.y,
				left.z*right.x - left.x*right.z,
				left.x*right.y - left.y*right.x);
}

XnFloat Vector3D::operator|(const Vector3D& other)
{
	return DotProduct(*this, other);
}
XnFloat DotProduct(const Vector3D& left, const Vector3D& right)
{
	return left.x*right.x + left.y*right.y + left.z*right.z;
}

Vector3D& Vector3D::Interpolate(const Vector3D& vec1, const Vector3D& vec2, XnFloat alpha)
{
	return Set(vec1.x+alpha*(vec2.x-vec1.x),
				vec1.y+alpha*(vec2.y-vec1.y),
				vec1.z+alpha*(vec2.z-vec1.z));
}

XnBool Vector3D::IsSameDirection(const Vector3D& other) const
{
	if (IsZero() || other.IsZero())
	{
		return true;
	}

	XnFloat ratio = 0;
	if (!Math::IsZero(x, m_tolerance) && !Math::IsZero(other.x, m_tolerance))
		ratio = other.x/x;
	else if (!Math::IsZero(y, m_tolerance) && !Math::IsZero(other.y, m_tolerance))
		ratio = other.y/y;
	else if (!Math::IsZero(z, m_tolerance) && !Math::IsZero(other.z, m_tolerance))
		ratio = other.z/z;
	else
	{
		return false;
	}

	if (other/ratio == *this)
		return true;

	return false;
}

inline Vector3D MaxAbs(const Vector3D &v1, const Vector3D &v2)
{
	return Vector3D(Math::MaxAbs(v1.x,v2.x),Math::MaxAbs(v1.y,v2.y),Math::MaxAbs(v1.z,v2.z));
}

inline Vector3D CrossProduct(const Vector3D &v1, const Vector3D &v2) {
	return Vector3D(v1.y*v2.z-v1.z*v2.y, v1.z*v2.x-v1.x*v2.z, v1.x*v2.y-v1.y*v2.x);
}

inline void CrossProduct(const Vector3D &v1, const Vector3D &v2, Vector3D &v3) 
{
	v3.Set(v1.y*v2.z-v1.z*v2.y, v1.z*v2.x-v1.x*v2.z, v1.x*v2.y-v1.y*v2.x);
}

inline XnFloat CrossProductMagnitudeSquared(const Vector3D &v1, const Vector3D &v2)
{
	return Math::Sqr(v1.y*v2.z-v1.z*v2.y)+Math::Sqr(v1.z*v2.x-v1.x*v2.z)+Math::Sqr(v1.x*v2.y-v1.y*v2.x);
}

inline XnFloat CrossProductMagnitude(const Vector3D &v1, const Vector3D &v2)
{
	return Math::Sqrt(CrossProductMagnitudeSquared(v1,v2));
}


} // xnl


#endif // _XN_VECTOR3D_H_
