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

#include "mathematics.h"
#include <iostream>
using namespace std;

const CVector3D CVector3D::ZERO = CVector3D(0, 0, 0);

float *createLookAtLH(const CVector3D &position, const CVector3D &lookAt, const CVector3D &up) {
	CVector3D z = CVector3D::normalize(lookAt - position);
	CVector3D x = CVector3D::normalize(CVector3D::cross(up, z));
	CVector3D y = -CVector3D::cross(z, x);

	float xdot = CVector3D::dot(x, position);
	float ydot = CVector3D::dot(y, position);
	float zdot = CVector3D::dot(z, position);

	float matrix[] = { x.x, y.x, z.x, 0,
					  x.y, y.y, z.y, 0,
					  x.z, y.z, z.z, 0,
					  xdot, ydot, zdot, 1 };

	float *result = new float[16];
	for (int i = 0; i < 16; i++)
		result[i] = matrix[i];
	return result;
}
float *createPerspective(float fovY, float aspect, float zn, float zf) {
	float yScale = atan(fovY / 2);
	float xScale = yScale / aspect;
	float matrix[] = { xScale, 0, 0, 0,
					   0, yScale, 0, 0,
					   0, 0, zf/(zf-zn), 1,
					   0, 0, -zn*zf/(zf-zn), 0};

	float *result = new float[16];
	for (int i = 0; i < 16; i++)
		result[i] = matrix[i];
	return result;
}

CVector3D::CVector3D() {
	CVector3D(0.0, 0.0, 0.0);
}
CVector3D::CVector3D(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}
CVector3D::CVector3D(const CVector3D &vec) {
	this->x = vec.x;
	this->y = vec.y;
	this->z = vec.z;
}
float CVector3D::length() const {
	return sqrt(x*x + y*y + z*z);
}
float CVector3D::lengthSq() const {
	return x*x + y*y + z*z;
}
CVector3D CVector3D::normalize(const CVector3D &vec) {
	CVector3D result(vec);
	float length = result.length();
	result /= length;
	return result;
}
float CVector3D::dot(const CVector3D &v1, const CVector3D &v2) {
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}
CVector3D CVector3D::cross(const CVector3D &v1, const CVector3D &v2) {
	return CVector3D(v1.y*v2.z - v1.z*v2.y, v1.z * v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
}
CVector3D CVector3D::reflect(const CVector3D &vector, const CVector3D &normal) {
	return -vector + 2 * CVector3D::dot(normal, vector)*normal;
}
CVector3D CVector3D::rotate(const CVector3D &vector, float angle, const CVector3D &rotationVec) {
	float c = cos(angle);
	float s = sin(angle);

	float matrix[3][3];
	matrix[0][0] = rotationVec.x*rotationVec.x*(1 - c) + c;
	matrix[0][1] = rotationVec.x*rotationVec.y*(1 - c) - rotationVec.z*s;
	matrix[0][2] = rotationVec.z*rotationVec.z*(1 - c) + rotationVec.y*s;

	matrix[1][0] = rotationVec.y*rotationVec.x*(1 - c) + rotationVec.z*s;
	matrix[1][1] = rotationVec.y*rotationVec.y*(1 - c) + c;
	matrix[1][2] = rotationVec.y*rotationVec.z*(1 - c) - rotationVec.x*s;

	matrix[2][0] = rotationVec.x*rotationVec.z*(1 - c) - rotationVec.y*s;
	matrix[2][1] = rotationVec.y*rotationVec.z*(1 - c) + rotationVec.x*s;
	matrix[2][2] = rotationVec.z*rotationVec.z*(1 - c) + c;

	CVector3D result;
	result.x = matrix[0][0] * vector.x + matrix[0][1] * vector.y + matrix[0][2] * vector.z;
	result.y = matrix[1][0] * vector.x + matrix[1][1] * vector.y + matrix[1][2] * vector.z;
	result.z = matrix[2][0] * vector.x + matrix[2][1] * vector.y + matrix[2][2] * vector.z;
	return result;
}
CVector3D &CVector3D::operator+=(const CVector3D &vector) {
	x += vector.x;
	y += vector.y;
	z += vector.z;
	return *this;
}
CVector3D &CVector3D::operator-=(const CVector3D &vector) {
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;
	return *this;
}
CVector3D &CVector3D::operator*=(float coef) {
	x *= coef;
	y *= coef;
	z *= coef;
	return *this;
}
CVector3D &CVector3D::operator/=(float coef) {
	x /= coef;
	y /= coef;
	z /= coef;
	return *this;
}

CVector3D operator+(const CVector3D &v1, const CVector3D &v2) {
	return CVector3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}
CVector3D operator-(const CVector3D &v1, const CVector3D &v2) {
	return CVector3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}
CVector3D operator-(const CVector3D &vector) {
	return CVector3D(-vector.x, -vector.y, -vector.z);
}
CVector3D operator*(const CVector3D &vector, float coef) {
	return CVector3D(coef*vector.x, coef*vector.y, coef*vector.z);
}
CVector3D operator*(float coef, const CVector3D &vector) {
	return CVector3D(coef*vector.x, coef*vector.y, coef*vector.z);
}
CVector3D operator/(const CVector3D &vector, float coef) {
	return CVector3D(vector.x / coef, vector.y / coef, vector.z / coef);
}
bool operator==(const CVector3D &v1, const CVector3D &v2) {
	return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
}
bool operator!=(const CVector3D &v1, const CVector3D &v2) {
	return !(v1 == v2);
}