/*
	Copyright 2014 Mateusz Chudyk.

	This file is part of RayTracerGPU.

	RayTracerGPU is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	RayTracerGPU is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with RayTracerGPU; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef RAYTRACER_MATHEMATICS
#define RAYTRACER_MATHEMATICS

#include <cmath>

class CVector3D {
	public:
		float x, y, z;

		CVector3D();
		CVector3D(float x, float y, float z);
		CVector3D(const CVector3D &vec);

		float length() const;
		float lengthSq() const;

		static CVector3D normalize(const CVector3D &vec);
		static float dot(const CVector3D &v1, const CVector3D &v2);
		static CVector3D cross(const CVector3D &v1, const CVector3D &v2);
		static CVector3D reflect(const CVector3D &vector, const CVector3D &normal);
		static CVector3D rotate(const CVector3D &vector, float angle, const CVector3D &rotationVec);

		CVector3D &operator+=(const CVector3D &vector);
		CVector3D &operator-=(const CVector3D &vector);
		CVector3D &operator*=(float coef);
		CVector3D &operator/=(float coef);

		friend CVector3D operator+(const CVector3D &v1, const CVector3D &v2);
		friend CVector3D operator-(const CVector3D &v1, const CVector3D &v2);
		friend CVector3D operator-(const CVector3D &vector);
		friend CVector3D operator*(const CVector3D &vector, float coef); 
		friend CVector3D operator*(float coef, const CVector3D &vector);
		friend CVector3D operator/(const CVector3D &vector, float coef);
		friend bool operator==(const CVector3D &v1, const CVector3D &v2);
		friend bool operator!=(const CVector3D &v1, const CVector3D &v2);

	public:
		static const CVector3D ZERO;
};


float *createLookAtLH(const CVector3D &position, const CVector3D &lookAt, const CVector3D &up);
float *createPerspective(float fov, float aspect, float zn, float zf);

#endif