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
#include "XnSymmetricMatrix3x3.h"
#include "XnMatrix3x3.h"

//#define USE_STABLE_EIGENVALUES
//#define ALWAYS_USE_SCALING_FOR_EIGENVALUES

namespace xnl
{

static XnFloat GetScaleFactorForEigenValues(const SymmetricMatrix3x3& mat)
{
	static const XnFloat target = 1e3f;
	XnFloat maxAbs = mat.MaxAbs();
	return maxAbs>0 ? target/maxAbs : 1.0f;
}

Vector3D SymmetricMatrix3x3::GetEigenValuesNoScaling() const
{
#ifdef USE_STABLE_EIGENVALUES
	Vector3D eigenvalues;
	Matrix3X3 eigenvectors;
	EigenDecomposition(*this, eigenvectors, eigenvalues);
	return eigenvalues;
#else
	XnFloat m = (XnFloat)Math::ONE_THIRD*Trace();
	XnFloat a00 = m_x00-m, a11 = m_x11-m, a22 = m_x22-m;
	XnFloat a01_sqr = Math::Sqr(m_x01);
	XnFloat a02_sqr = Math::Sqr(m_x02);
	XnFloat a12_sqr = Math::Sqr(m_x12);
	XnFloat p = (XnFloat)Math::ONE_SIXTH * (a00*a00+a11*a11+a22*a22 + 2*(a01_sqr+a02_sqr+a12_sqr));
	XnFloat q = XnFloat(0.5)*(a00*(a11*a22-a12_sqr)-a11*a02_sqr-a22*a01_sqr) + m_x01*m_x02*m_x12;
	XnFloat sqrt_p = Math::Sqrt(p), disc = p*p*p-q*q;
	XnFloat phi = XnFloat(Math::ONE_THIRD)*(XnFloat)atan2(Math::Sqrt(Math::Max((XnFloat)0,disc)),q);
	XnFloat c= (XnFloat)cos(phi) , s= (XnFloat)sin(phi);
	XnFloat sqrt_p_cos = sqrt_p*c, root_three_sqrt_p_sin = XnFloat(Math::ROOT_THREE)*sqrt_p*s;
	Vector3D lambda(m+2*sqrt_p_cos, m-sqrt_p_cos-root_three_sqrt_p_sin, m-sqrt_p_cos+root_three_sqrt_p_sin);
	Math::ExchangeSort(lambda.z, lambda.y, lambda.x);
	return lambda;
#endif
}

Vector3D SymmetricMatrix3x3::GetEigenValuesWithScaling() const
{
	XnFloat factor = GetScaleFactorForEigenValues(*this);
	SymmetricMatrix3x3 normalizedMatrix = *this;
	normalizedMatrix *= factor;
	return (1/factor)*normalizedMatrix.GetEigenValuesNoScaling();
}

Vector3D SymmetricMatrix3x3::GetEigenValues() const
{
#ifdef ALWAYS_USE_SCALING_FOR_EIGENVALUES
	return GetEigenValuesWithScaling();
#else
	return GetEigenValuesNoScaling();
#endif
}

void SymmetricMatrix3x3::GetEigenVectors(const Vector3D &lambda,Matrix3x3 &eigenvectors,XnFloat tolerance) const
{
	XnFloat tiny = tolerance * Math::MaxAbs(lambda.x,lambda.z);
	if(lambda.x-lambda.y<=tiny)
	{
		if(lambda.y-lambda.z<=tiny)
			eigenvectors = Matrix3x3::Identity();
		else {
			Vector3D v2;
			GetEigenVector(lambda.z,v2);
			Vector3D v1=v2.UnitOrthogonalVector();
			Vector3D v0;
			CrossProduct(v1,v2,v0);
            eigenvectors.Set(v0,v1,v2);
		}
	}
	else if(lambda.y-lambda.z<=tiny) {
		Vector3D v0;
		GetEigenVector(lambda.x,v0);
		Vector3D v1=v0.UnitOrthogonalVector();
		Vector3D v2;
		CrossProduct(v0,v1,v2);
		eigenvectors.Set(v0,v1,v2);
	}
	else {
		Vector3D v0;
		GetEigenVector(lambda.x,v0);
		Vector3D v2;
		GetEigenVector(lambda.z,v2);
		Vector3D v1;
		CrossProduct(v2,v0,v1);
		eigenvectors.Set(v0,v1,v2);
	}
}

void SymmetricMatrix3x3::GetEigenVector(XnFloat lambda,Vector3D &v) const
{
	SymmetricMatrix3x3 COMat((m_x11-lambda)*(m_x22-lambda) - m_x12*m_x12, m_x12*m_x02 - m_x01*(m_x22-lambda), m_x01*m_x12 - (m_x11-lambda)*m_x02,
								(m_x00-lambda)*(m_x22-lambda) - m_x02*m_x02, m_x01*m_x02 - (m_x00-lambda)*m_x12,
								(m_x00-lambda)*(m_x11-lambda) - m_x01*m_x01);
	XnFloat scales[3] = {Math::Sqr(COMat.m_x00) + Math::Sqr(COMat.m_x01) + Math::Sqr(COMat.m_x02), 
				   Math::Sqr(COMat.m_x01) + Math::Sqr(COMat.m_x11) + Math::Sqr(COMat.m_x12), 
				   Math::Sqr(COMat.m_x02) + Math::Sqr(COMat.m_x12) + Math::Sqr(COMat.m_x22)};
	//XnInt32 i = ArgMax(scales[0], scales[1], scales[2]);
	if (scales[0] > scales[1])
	{
		if (scales[0] > scales[2])
		{
			//assert(i==0);
			
			const XnFloat s=Math::OneOverSqrtHelper<XnFloat>::OneOverSqrt(scales[0]);
			v.Set(COMat.m_x00*s,COMat.m_x01*s,COMat.m_x02*s);
			return;
		}
		else
		{
			//assert(i==2);
			const XnFloat s=Math::OneOverSqrtHelper<XnFloat>::OneOverSqrt(scales[2]);
			v.Set(COMat.m_x02*s,COMat.m_x12*s,COMat.m_x22*s);
			return;
		}
	}
	else if (scales[1] > scales[2])
	{
		//assert(i==1);
		const XnFloat s=Math::OneOverSqrtHelper<XnFloat>::OneOverSqrt(scales[1]);
		v.Set(COMat.m_x01*s,COMat.m_x11*s,COMat.m_x12*s);
		return;
	}
	else
	{
		//assert(i==2);
		const XnFloat s=Math::OneOverSqrtHelper<XnFloat>::OneOverSqrt(scales[2]);
		v.Set(COMat.m_x02*s,COMat.m_x12*s,COMat.m_x22*s);
		return;
	}
	
}

// No scaling
void SymmetricMatrix3x3::SolveEigenProblemNoScaling(Vector3D &eigenvalues,Matrix3x3 &eigenvectors,XnFloat tolerance) const
{
#ifdef USE_STABLE_EIGENVALUES
    EigenDecomposition(*this, eigenvectors, eigenvalues);
#else
	eigenvalues = GetEigenValuesNoScaling(); // This function takes care of the scaling for us
    GetEigenVectors(eigenvalues,eigenvectors,tolerance);
#endif
}

// With scaling
void SymmetricMatrix3x3::SolveEigenProblemWithScaling(Vector3D &eigenvalues,Matrix3x3 &eigenvectors,XnFloat tolerance) const
{
#ifdef USE_STABLE_EIGENVALUES
    XnFloat factor = GetScaleFactorForEigenValues(*this);
    SymmetricMatrix3x3 normalizedMatrix(factor*x00,factor*x01,factor*x02,factor*x11,factor*x12,factor*x22);
    EigenDecomposition(normalizedMatrix, eigenvectors, eigenvalues);
    eigenvalues *= (1/factor);
#else
	eigenvalues = GetEigenValuesWithScaling();
    GetEigenVectors(eigenvalues,eigenvectors,tolerance);
#endif
}

// Uses scaling if ALWAYS_USE_SCALING_FOR_EIGENVALUES is defined
void SymmetricMatrix3x3::SolveEigenProblem(Vector3D& eigenvalues,Matrix3x3& eigenvectors,XnFloat tolerance) const
{
#ifdef ALWAYS_USE_SCALING_FOR_EIGENVALUES
    SolveEigenProblemWithScaling(eigenvalues, eigenvectors, tolerance);
#else
    SolveEigenProblemNoScaling(eigenvalues, eigenvectors, tolerance);
#endif
}

// Uses scaling if ALWAYS_USE_SCALING_FOR_EIGENVALUES is defined
void SymmetricMatrix3x3::SolveSingleEigenProblem(XnInt32 i,Vector3D& eigenvalues,Vector3D& eigenvector,XnFloat /*tolerance*/) const
{
#ifdef USE_STABLE_EIGENVALUES
    Matrix3x3 eigenvectors;
    SolveEigenProblem(eigenvalues, eigenvectors, tolerance); // This function takes care of the scaling for us
    eigenvector = eigenvectors.GetColumn(i);
#else
	eigenvalues = GetEigenValues(); // This function takes care of the scaling for us
    GetEigenVector(eigenvalues[i], eigenvector);
#endif
}

XnFloat SymmetricMatrix3x3::MaxAbs() const {
	return Math::Max(Math::MaxAbs(m_x00,m_x01,m_x02),Math::MaxAbs(m_x11,m_x12),(XnFloat)Math::Abs(m_x22));
}


} // XnLib
