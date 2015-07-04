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

// ======================================== CONST ======================================== //
const __constant double EPS = 0.000001f;
const __constant double MAX = 100000.0f;
const __constant int MAX_OBJECTS = 1000;
const __constant int MAX_LIGHTS = 1000;

const __constant float3	WHITE		= (float3)(1, 1, 1);
const __constant float3 BLACK		= (float3)(0, 0, 0);
const __constant float3 RED			= (float3)(1, 0, 0);
const __constant float3 GREEN		= (float3)(0, 1, 0);
const __constant float3 BLUE		= (float3)(0, 0, 1);
const __constant float3 GRAY		= (float3)(0.6, 0.6, 0.6);
const __constant float3 YELLOW		= (float3)(1.0, 1.0, 0);
const __constant float3 ORANGE		= (float3)(1.0, 0.7, 0.25);
const __constant float3 PINK		= (float3)(1.0, 0.46, 0.88);
const __constant float3 LIGHTGREEN	= (float3)(0.71, 0.9, 0.11);
const __constant float3 BLUESKY		= (float3)(0.8, 0.9, 0.95);

float3 matrixByVector(__global float *matrix, float3 *vector){ 
	float3 result;
	result.x = matrix[0]*(*vector).x + matrix[4]*(*vector).y + matrix[8]*(*vector).z + matrix[12];
	result.y = matrix[1]*(*vector).x + matrix[5]*(*vector).y + matrix[9]*(*vector).z + matrix[13];
	result.z = matrix[2]*(*vector).x + matrix[6]*(*vector).y + matrix[10]*(*vector).z + matrix[14];
	return result;
}

float3 reflect(float3 vector, float3 normal) {
	return -vector + 2*dot(normal, vector)*normal;
}

float3 clipColor(float3 color) {
	float3 result;
	result.x = max(0.0f, min(1.0f, color.x));
	result.y = max(0.0f, min(1.0f, color.y));
	result.z = max(0.0f, min(1.0f, color.z));
	return result;
}

// ================================= BASIC STRUCTURES ================================= //
struct Ray {
	float3 origin;
	float3 direction;
};

struct HitTestResult {
	bool hit;
	float t;
	float3 normal;
};

struct HitInfo {
	struct Scene *scene;
	struct Ray *ray;
	struct Object *object;
	float3 normal;
	float3 point;
	int depth;
};

struct Scene {
	struct Object *objects[MAX_OBJECTS];
	int countObj;
	struct Light *lights[MAX_LIGHTS];
	int countLight;
};

struct Scene createScene(int n) {
	struct Scene result;
	result.countObj = n;
	return result;
}

// ======================================= LIGHT ======================================= //
struct Light {
	float3 color;
	float3 position;
	float power;
};

// ======================================= OBJECTS =======================================//
struct Sphere {
	float3 center;
	float radius;
};

struct Sphere createSphere(float3 center, float radius) {
	struct Sphere result;
	result.center = center;
	result.radius = radius;
	return result;
}

struct HitTestResult testSphere(struct Sphere *sphere, struct Ray *ray) {
	struct HitTestResult result;
	result.hit = false;

	float3 distance = ray->origin - sphere->center;
	float a = length(ray->direction)*length(ray->direction);
	float b = dot(2*distance, ray->direction);
	float c = length(distance)*length(distance) - sphere->radius*sphere->radius;
	float delta = b*b - 4*a*c;

	if(delta < 0)
		return result;

	delta = sqrt(delta);
	float denominator = 2*a;
	float t;
	t = (-b - delta)/denominator;
	if(t < EPS) {
		t = (-b + delta)/denominator;
		if(t < EPS)
			return result;
	}
	result.hit = true;
	result.t = t;
	result.normal = normalize(ray->origin + ray->direction*t - sphere->center);
	return result;
}

struct Plane {
	float3 point;
	float3 normal;
};

struct Plane createPlane(float3 point, float3 normal) {
	struct Plane result;
	result.point = point;
	result.normal = normal;
	return result;
}

struct HitTestResult testPlane(struct Plane *plane, struct Ray *ray) {
	struct HitTestResult result;
	result.hit = false;

	float n = dot(ray->direction, plane->normal);
	if(n == 0)
		return result;

	float t = dot(plane->point - ray->origin, plane->normal) / n;
	if (t < EPS)
		return result;

	result.hit = true;
	result.t = t;
	result.normal = plane->normal;
	return result;
}

enum OBJECT_TYPE {
	SPHERE,
	PLANE
};

struct Object {
	struct Material *material;
	void *object;
	int type;
};

struct Object createObject(struct Material *material, int type, void *object) {
	struct Object result;
	result.material = material;
	result.object = object;
	result.type = type;
	return result;
}

struct HitTestResult testObject(struct Object *obj, struct Ray *ray) {
	struct HitTestResult result;
	result.hit = false;
	
	switch(obj->type) {
		case SPHERE	:	result = testSphere((struct Sphere*)(obj->object), ray);break;
		case PLANE	:	result = testPlane((struct Plane*)(obj->object), ray);break;
	}
	return result;
}

bool isAnyObstacleBetween(struct Scene *scene, struct Object *obj, float3 p1, float3 p2) {
	float3 vector = p2 - p1;
	float dist = length(vector);

	struct Ray ray;
	ray.origin = p1;
	ray.direction = normalize(vector); 

	struct HitTestResult result;
	for(int i = 0; i < scene->countObj; i++) {
		result = testObject(scene->objects[i], &ray);
		if(result.hit == true && result.t < dist && scene->objects[i] != obj)
			return true;
	}
	return false;
}

// ====================================== MATERIALS ======================================//
struct PerfectDiffuse {
	float3 color;

};

struct PerfectDiffuse createPerfectDiffuse(float3 color) {
	struct PerfectDiffuse result;
	result.color = color;
	return result;
}

float3 shadePerfectDiffuse(struct PerfectDiffuse *material, struct HitInfo *hitInfo) {
	float3 total = (float3)(0, 0, 0);

	for(int i = 0; i < hitInfo->scene->countLight; i++) {
		if(i >= MAX_LIGHTS) {
			return total;
		}
		struct Light *light = hitInfo->scene->lights[i];
		float3 direction = normalize(light->position-hitInfo->point);
		float d = dot(direction, hitInfo->normal);

		if(d >= 0 && !isAnyObstacleBetween(hitInfo->scene, hitInfo->object, light->position, hitInfo->point)) {
			total += d*light->power*(float3)(light->color.x*material->color.x, light->color.y*material->color.y, light->color.z*material->color.z);
		}
	}
	return clipColor(total);
}

struct Phong {
	float3 color;
	float diffuse;
	float specular;
	float specularExp;
};

struct Phong createPhong(float3 color, float diffuse, float specular, float specularExp) {
	struct Phong result;
	result.color = color;
	result.diffuse = diffuse;
	result.specular = specular;
	result.specularExp = specularExp;
	return result;
}

float3 shadePhong(struct Phong *material, struct HitInfo *hitInfo) {
	float3 total = (float3)(0, 0, 0);
	
	float3 N = normalize(hitInfo->normal);
	float3 V = normalize(-hitInfo->ray->direction);

	for(int i = 0; i < hitInfo->scene->countLight; i++) {
		if(i >= MAX_LIGHTS) {
			return total;
		}

		struct Light *light = hitInfo->scene->lights[i];

		float3 L = normalize(light->position-hitInfo->point);
		float3 R = reflect(L, N);
		float ln = dot(L, N);
		float rv = dot(R, V);

		if(ln >= 0 && !isAnyObstacleBetween(hitInfo->scene, hitInfo->object, light->position, hitInfo->point)) {
			float3 result = (ln*material->diffuse)*(float3)(light->color.x*material->color.x, light->color.y*material->color.y, light->color.z*material->color.z);
			float phong;
			if (rv <= 0) {
				phong = 0;
			}
			else {
				phong = pow(rv, material->specularExp);
			}
			if (phong != 0) {
				result += material->color * material->specular * phong;
			}
			
			total += result*light->power;
		}
	}

	return clipColor(total);
}

enum MATERIAL_TYPE {
	PERFFECT_DIFFUSE,
	PHONG
};

struct Material {
	void *material;
	int type;
};

struct Material createMaterial(int type, void *material) {
	struct Material result;
	result.material = material;
	result.type = type;
	return result;
}

float3 shadeMaterial(struct Material *mat, struct HitInfo *hitInfo) {
	float3 result = (float3)(0, 0, 0);
	
	if(mat->type == PERFFECT_DIFFUSE) {
		result = shadePerfectDiffuse((struct PerfectDiffuse*)(mat->material), hitInfo);
	}
	else if(mat->type == PHONG) {
		result = shadePhong((struct Phong*)(mat->material), hitInfo);
	}

	return result;
}

// =================================== MAIN FUNCTIONS ==================================== //
float3 shadeRay(struct HitInfo *hitInfo, int maxDepth) {
	if(hitInfo->depth > maxDepth) {
		return (float3)(0, 0, 0);
	}
	else {
		return shadeMaterial(hitInfo->object->material, hitInfo);
	}
}

float3 raytrace(struct Scene *scene, struct Ray *ray, int depth) {	
	struct HitInfo hitInfo;
	float minT = MAX;
	struct HitTestResult hitTestResult;

	hitInfo.scene = scene;
	hitInfo.ray = ray;
	for(int i = 0; i < scene->countObj; i++) {
		hitTestResult = testObject(scene->objects[i], ray);
		if(hitTestResult.hit == true && hitTestResult.t < minT) {
			minT = hitTestResult.t;
			hitInfo.object = scene->objects[i];
			hitInfo.normal = hitTestResult.normal;
			hitInfo.point = ray->origin + hitTestResult.t * ray->direction;
			hitInfo.depth = depth+1;
		}
	}

	if(minT == MAX) {
		return BLUESKY;
	}
	else {
		return shadeRay(&hitInfo, 5);
	}
}

// ====================================== KERNEL ======================================= //
__kernel void main(__global float4 *output, uint width, uint height, float3 position, float3 lookAt, float3 up, uint samplerCount, __global float *sampler) {	
	// scene
	struct Plane p1 = createPlane((float3)(0, 0, 0), (float3)(0, 1, 0));
	struct PerfectDiffuse p1pd = createPerfectDiffuse(WHITE);
	struct Material p1m = createMaterial(PERFFECT_DIFFUSE, (void*)&p1pd);
	struct Object plane = createObject(&p1m, PLANE, &p1);

	struct Phong phong1 = createPhong(RED, 1, 4, 10);
	struct Material material1 = createMaterial(PHONG, (void*)&phong1);
	struct Sphere sphere1 = createSphere((float3)(-7, 3, -7), 3);
	struct Object object1 = createObject(&material1, SPHERE, &sphere1);
	struct Phong phong2 = createPhong(GREEN, 0.5, 10, 80);
	struct Material material2 = createMaterial(PHONG, (void*)&phong2);
	struct Sphere sphere2 = createSphere((float3)(-7, 3, 7), 3);
	struct Object object2 = createObject(&material2, SPHERE, &sphere2);
	struct Phong phong3 = createPhong(BLUE, 0.5, 10, 80);
	struct Material material3 = createMaterial(PHONG, (void*)&phong3);
	struct Sphere sphere3 = createSphere((float3)(7, 3, -7), 3);
	struct Object object3 = createObject(&material3, SPHERE, &sphere3);
	struct Phong phong4 = createPhong(WHITE, 0.5, 1, 10);
	struct Material material4 = createMaterial(PHONG, (void*)&phong4);
	struct Sphere sphere4 = createSphere((float3)(0, 1, 0), 1);
	struct Object object4 = createObject(&material4, SPHERE, &sphere4);

	struct Scene scene;
	scene.countObj = 5;
	scene.objects[0] = &plane;
	scene.objects[1] = &object1;
	scene.objects[2] = &object2;
	scene.objects[3] = &object3;
	scene.objects[4] = &object4;

	// lights
	struct Light l1;
	l1.position = 10*(float3)(-10, 2, -0.71);
	l1.color = ORANGE;
	l1.power = 0.7;

	struct Light l2;
	l2.position = position+(float3)(0,100,0);
	l2.color = WHITE;
	l2.power = 0.6;

	struct Light l3;
	l3.position = 10*(float3)(0, 2, 1);
	l3.color = WHITE;
	l3.power = 0.3;

	scene.countLight = 3;
	scene.lights[0] = &l1;
	scene.lights[1] = &l2;
	scene.lights[2] = &l3;

	// camera
	float3 cameraZ = normalize(lookAt - position);
	float3 cameraX = normalize(cross(up, cameraZ));
	float3 cameraY = cross(cameraZ, cameraX);

	int minDimension = min(width, height);
	int n = get_global_id(0);
	struct Ray ray;
	ray.origin = position;

	output[get_global_id(0)] = (float4)(0, 0, 0, 0);
	for(int i = 0; i < samplerCount; i++) {
		float x = ((n % width) + sampler[2*i] - width * 0.5) / minDimension * 2;
		float y = ((n / width) + sampler[2*i+1] - height * 0.5) / minDimension * 2;
		ray.direction = cameraX*x + cameraY * y + cameraZ*1.8;
		output[get_global_id(0)] += (float4)(raytrace(&scene, &ray, 0)/samplerCount, 1);
	}
	output[get_global_id(0)].w = 1;
}