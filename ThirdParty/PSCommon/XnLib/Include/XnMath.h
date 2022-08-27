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
#ifndef _XN_MATH_H_
#define _XN_MATH_H_

#include <math.h>
#include <XnPlatform.h>

namespace xnl
{

	namespace Math
	{
		inline XnInt32 Abs(XnInt32 i)
		{
			return abs(i);
		}
		inline XnFloat Abs(XnFloat f)
		{
			return (XnFloat)fabs(f);
		}
		template <class T>
		XnBool IsZero(T value, T tolerance)
		{
			return Abs(value) < tolerance;
		}
		inline XnFloat Sqr(XnFloat f)
		{
			return f*f;
		}
		inline XnFloat Sqrt(XnFloat f)
		{
			return (XnFloat)sqrt(f);
		}
		inline XnDouble Sqrt(XnDouble f)
		{
			return (XnFloat)sqrt(f);
		}
		template <class T>
		T Min(T value1, T value2)
		{
			return value1 < value2 ? value1 : value2;
		}
		template <class T>
		T Max(T value1, T value2)
		{
			return value1 > value2 ? value1 : value2;
		}
		template<class T>
		T Max(const T a, const T b, const T c)
		{
			return Max(a, Max(b,c));
		}
		template<class T>
		T Max(const T a, const T b, const T c, const T d)
		{
			return Max(a, Max(b,c,d));
		}

		template <class T>
		T Crop(T value, T upper, T lower)
		{
			return Min(upper, Max(lower, value));
		}
		template <class T>
		XnBool IsBetween(T value, T upper, T lower)
		{
			return value < upper && value > lower;
		}
		template<class T>
		XnInt32 ArgMax(const T a, const T b) {
			return (a>b) ? 0 : 1;
		}
		template<class T>
		XnInt32 ArgMax(const T a, const T b, const T c) {
			return (a>b) ? ((a>c) ? 0 : 2) : ((b>c) ? 1 : 2);
		}
		template<class T>
		XnInt32 ArgMax(const T a, const T b, const T c, const T d) {
			return (a>d) ? ArgMax(a,b,c) : ArgMax(b,c,d)+1;
		}
		template<class T>
		XnInt32 ArgMin(const T a, const T b) {
			return (a<b) ? 0 : 1;
		}
		template<class T>
		XnInt32 ArgMin(const T a, const T b, const T c) {
			return (a<b) ? ((a<c) ? 0 : 2) : ((b<c) ? 1 : 2);
		}

		template<class T> T MaxAbs(const T a, const T b) { return Max(Abs(a),Abs(b)); }
		template<class T> T MaxAbs(const T a, const T b, const T c) { return Max(Abs(a),Abs(b),Abs(c)); }
		template<class T> T MaxAbs(const T a, const T b, const T c, const T d) { return Max(Abs(a),Abs(b),Abs(c),Abs(d)); }

		template<class T>
		void Exchange(T &a, T &b) { T c=a; a=b; b=c; }

		template<class T>
		void Swap(T &a, T &b) { T c=a; a=b; b=c; }

		template<class T>
		void ExchangeSort(T &a, T &b)
		{
			if(a > b) Exchange(a,b);
		}

		template<class T>
		void ExchangeSort(T &a, T &b, T &c)
		{
			if(a > b) Exchange(a,b);
			if(b > c) Exchange(b,c);
			if(a > b) Exchange(a,b);
		}

		template<class T>
		struct OneOverSqrtHelper
		{
			static T OneOverSqrt(T MagSq) {return T(1.0)/Sqrt(MagSq); }
		};
		template<class T>
		inline XnBool IsNaN(const T& scalar)
		{
#if defined(_WIN32)
			return _isnan(scalar)!=0;
#elif defined(_ARC)
			return (scalar != scalar);
			//    return isnan(scalar);
#else
			return isnan(scalar);
#endif
		}

#define round(x) ((XnInt32)floor((x)+0.5f))

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
		static const XnFloat ONE_THIRD = (XnFloat)(1.0f/3.0f);
		static const XnFloat ONE_SIXTH = (XnFloat)(1.0f/6.0f);
		static const XnFloat ROOT_TWO = (XnFloat)Sqrt(2.0f);
		static const XnFloat ROOT_THREE = (XnFloat)Sqrt(3.0f);
		static const XnFloat PI = (XnFloat)M_PI;
		static const XnFloat HALF_PI = (XnFloat)(0.5f*M_PI);
		static const XnFloat TWO_PI = (XnFloat)(2.0f*M_PI);
		static const XnFloat ROOT_TWO_PI = (XnFloat)Sqrt(2.0f*M_PI);
		static const XnFloat DTR = (XnFloat)(M_PI / 180.0f);
		static const XnFloat RTD = (XnFloat)(180.0f / M_PI);
		static const XnFloat PHI = (XnFloat)((-1.0f + Sqrt(5.0f)) / 2.0f);

	} // Math
} // xnl

#endif // _XN_MATH_H_
