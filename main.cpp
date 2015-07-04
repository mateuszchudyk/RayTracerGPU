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

#include <CL/cl.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_keyboard.h>

#pragma comment (lib, "OpenCl.lib")
#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "opengl32.lib")

#include "raytracer.h"

using namespace std;

#define PI 3.1415926535897932384626433832795

// GLOBAL VARIABLES
const unsigned WIDTH = 1060;
const unsigned HEIGHT = 600;
const size_t AREA = WIDTH*HEIGHT;
const char *TITLE = "Raytracer";
const bool FULLSCREEN = false;
const unsigned SAMPLES = 16;
SDL_Window *window;
SDL_Renderer *renderer;
OpenCLManager *manager;
OpenCLKernel *kernel;

CVector3D position, lookAt, up;
float *sampler;
cl_mem outputB;
cl_mem samplerB;

CVector3D xVec, yVec;
int coefX, coefY;

// FUNCTIONS
void update(float dt);
void render();

// MAIN FUNCTION
#ifdef main
	#undef main
#endif

int main(int argc, char* argv[]) {
	cout << "/----------------------------------------------------------\\" << endl;
	cout << "|                                                          |" << endl;
	cout << "|                     RayTracerGPU v1.0                    |" << endl;
	cout << "|                                                          |" << endl;
	cout << "|                  Author:  Mateusz Chudyk                 |" << endl;
	cout << "|                                                          |" << endl;
	cout << "\\----------------------------------------------------------/" << endl << endl << endl << endl;

	// opencl
	cout << "-= CHOOSE PLATFORM/DEVICE =-" << endl;
	manager = Raytracer::createOpenCLManager();
	cout << endl << endl;

	cout << "-= LOGS =-" << endl;
	if (manager == NULL) {
		cout << "OpenCLManager can't create!" << endl;
		system("pause");
		return 1;
	}
	kernel = Raytracer::createOpenCLKernel(manager, "kernel.cl", "main");
	if (kernel == NULL) {
		cout << "OpenCLKernel can't create!" << endl;
		system("pause");
		return 1;
	}
	else if (kernel->isErrors()) {
		cout << "Compiletion failed!" << endl << kernel->getBuildInfo() << endl;
		system("pause");
		return 1;
	}

	// sdl window
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, (FULLSCREEN == true ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) | SDL_WINDOW_OPENGL, &window, &renderer);
	SDL_SetWindowTitle(window, "RayTracerGPU v1.0");
	SDL_GL_CreateContext(window);
	SDL_WarpMouseInWindow(window, WIDTH / 2, HEIGHT / 2);
	SDL_ShowCursor(0);
	SDL_GL_SetSwapInterval(1);

	// opengl
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 1, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// kernel parameters
	position = 2*CVector3D(7, 5, 7);
	lookAt = CVector3D::normalize(CVector3D(0,2,0)-position);// CVector3D(0, 0, 1);
	up = CVector3D(0, 1, 0);
	xVec = yVec = CVector3D(0, 0, 0);
	coefX = coefY = 0;
	sampler = new float[2 * SAMPLES];
	for (int i = 0; i < SAMPLES; i++) {
		sampler[2 * i] = (rand() % 10)/10.0;
		sampler[2 * i + 1] = (rand() % 10) / 10.0;
	}

	cl_int error = CL_SUCCESS;
	outputB = clCreateBuffer(manager->getContext(), CL_MEM_WRITE_ONLY, AREA*sizeof(cl_float4), NULL, &error);
	if (error != CL_SUCCESS) {
		cout << "Buffer can't create!" << endl;
		system("pause");
		return 1;
	}

	samplerB = clCreateBuffer(manager->getContext(), CL_MEM_WRITE_ONLY, 2*SAMPLES*sizeof(cl_float), NULL, &error);
	if (error != CL_SUCCESS) {
		cout << "Buffer can't create!" << endl;
		system("pause");
		return 1;
	}

	// main loop
	float dt;
	int lastTicks = SDL_GetTicks();
	SDL_Event evt;
	bool loop = true;
	while(loop) {
		dt = 100000.0f / (SDL_GetTicks() - lastTicks);
		lastTicks = SDL_GetTicks();

		while (SDL_PollEvent(&evt)){
			if (evt.type == SDL_QUIT || (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE)) {
				loop = false;
				break;
			}

			if (evt.type == SDL_KEYDOWN) {
				switch (evt.key.keysym.sym) {
					case SDLK_LEFT:		coefX = 1;break;
					case SDLK_RIGHT:	coefX = -1;break;
					case SDLK_UP:		coefY = 1; break;
					case SDLK_DOWN:		coefY = -1;break;
					default:			break;
				}
			}
			else if (evt.type == SDL_KEYUP) {
				switch (evt.key.keysym.sym) {
					case SDLK_LEFT:		coefX = 0; break;
					case SDLK_RIGHT:	coefX = 0; break;
					case SDLK_UP:		coefY = 0; break;
					case SDLK_DOWN:		coefY = 0; break;
					default:			break;
				}
			}
			else if (evt.type == SDL_MOUSEMOTION) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				float dx = x - (int)WIDTH / 2;
				float dy = -(y - (int)HEIGHT / 2);
				SDL_WarpMouseInWindow(window, WIDTH / 2, HEIGHT / 2);

				float coef = 0.1;
				lookAt = CVector3D::rotate(lookAt, dx*coef*M_PI / 180.0f, CVector3D::normalize(CVector3D::cross(CVector3D::cross(lookAt, up), lookAt)));
				lookAt = CVector3D::rotate(lookAt, dy*coef*M_PI / 180.0f, CVector3D::normalize(CVector3D::cross(lookAt, up)));

				if (coefX != 0) {
					xVec = CVector3D::normalize(CVector3D::cross(lookAt, up));
				}
				if (coefY != 0) {
					yVec = lookAt;
				}
			}
		}

		update(dt);
		render();
	}

	if(clReleaseMemObject(outputB) != CL_SUCCESS) {
		cout << "Can't release mem object: outputB!" << endl;
		system("pause");
		return 1;
	}
	if (clReleaseMemObject(samplerB) != CL_SUCCESS) {
		cout << "Can't release mem object: outputB!" << endl;
		system("pause");
		return 1;
	}
	delete manager;
	delete kernel;
	SDL_ShowCursor(1);
	SDL_Quit();
	return 0;
}

void update(float dt) {
	if (coefX != 0)
		position += coefX*xVec*dt*0.0003f;
	if (coefY != 0)
		position += coefY*yVec*dt*0.0003f;
}

void render() {
	float pos[] = { position.x, position.y, position.z }; 
	float la[] = { position.x + lookAt.x, position.y + lookAt.y, position.z + lookAt.z };
	float u[] = { up.x, up.y, up.z };

	if (clSetKernelArg(kernel->getKernel(), 0, sizeof(outputB), (void*)&outputB) != CL_SUCCESS) {
		cout << "Set kernel arg: outputB!" << endl;
		system("pause");
		exit(1);
	}
	if (clSetKernelArg(kernel->getKernel(), 1, sizeof(cl_uint), (void*)&WIDTH) != CL_SUCCESS) {
		cout << "Set kernel arg: WIDTH!" << endl;
		system("pause");
		exit(1);
	}
	if (clSetKernelArg(kernel->getKernel(), 2, sizeof(cl_uint), (void*)&HEIGHT) != CL_SUCCESS) {
		cout << "Set kernel arg: HEIGHT!" << endl;
		system("pause");
		exit(1);
	}
	if (clSetKernelArg(kernel->getKernel(), 3, sizeof(cl_float3), (void*)&pos) != CL_SUCCESS) {
		cout << "Set kernel arg: HEIGHT!" << endl;
		system("pause");
		exit(1);
	}
	if (clSetKernelArg(kernel->getKernel(), 4, sizeof(cl_float3), (void*)&la) != CL_SUCCESS) {
		cout << "Set kernel arg: HEIGHT!" << endl;
		system("pause");
		exit(1);
	}
	if(clSetKernelArg(kernel->getKernel(), 5, sizeof(cl_float3), (void*)&up) != CL_SUCCESS) {
		cout << "Set kernel arg: HEIGHT!" << endl;
		system("pause");
		exit(1);
	}
	if (clSetKernelArg(kernel->getKernel(), 6, sizeof(cl_uint), (void*)&SAMPLES) != CL_SUCCESS) {
		cout << "Set kernel arg: samplerCount!" << endl;
		system("pause");
		exit(1);
	}
	if (clSetKernelArg(kernel->getKernel(), 7, sizeof(samplerB), (void*)&samplerB) != CL_SUCCESS) {
		cout << "Set kernel arg: sampler!" << endl;
		system("pause");
		exit(1);
	}

	cl_float *ptrSampler = (cl_float*)clEnqueueMapBuffer(manager->getQueue(), samplerB, CL_TRUE, CL_MAP_WRITE, 0, 2*SAMPLES * sizeof(cl_float), 0, NULL, NULL, NULL);
	memcpy(ptrSampler, sampler, sizeof(float)*2*SAMPLES);
	clEnqueueUnmapMemObject(manager->getQueue(), samplerB, ptrSampler, 0, 0, 0);


	cl_int error = CL_SUCCESS;
	error = clEnqueueNDRangeKernel(manager->getQueue(), kernel->getKernel(), 1, NULL, &AREA, NULL, 0, NULL, NULL);
	if(error != CL_SUCCESS) {
		cout << "clEnqueueNDRangeKernel!: " << error << endl;
		system("pause");
		exit(1);
	}

	cl_float4 *ptrOutput = (cl_float4*)clEnqueueMapBuffer(manager->getQueue(), outputB, CL_TRUE, CL_MAP_READ, 0, AREA * sizeof(cl_float4), 0, NULL, NULL, &error);
	if (error != CL_SUCCESS) {
		cout << "clEnqueueMapBuffer!" << endl;
		system("pause");
		exit(1);
	}

	unsigned char *output = new unsigned char[AREA * 4];
	for (int i = 0; i < AREA; i++) {
		output[i * 4] = (unsigned char)(ptrOutput[i].s[0] * 255);
		output[i * 4 + 1] = (unsigned char)(ptrOutput[i].s[1] * 255);
		output[i * 4 + 2] = (unsigned char)(ptrOutput[i].s[2] * 255);
		output[i * 4 + 3] = (unsigned char)255;
	}
	glEnable(GL_TEXTURE_2D);

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, output);

	delete[] output;

	glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, 1, 100);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(0, 0, -1);
		glTexCoord2f(0, 1);
		glVertex3f(0, 1, -1);
		glTexCoord2f(1, 1);
		glVertex3f(1, 1, -1);
		glTexCoord2f(1, 0);
		glVertex3f(1, 0, -1);
	glEnd();

	SDL_RenderPresent(renderer);
	SDL_GL_SwapWindow(window);
	if(clFinish(manager->getQueue()) != CL_SUCCESS) {
		cout << "clFinish!" << endl;
		system("pause");
		exit(1);
	}
	glDeleteTextures(1, &texture);
}
