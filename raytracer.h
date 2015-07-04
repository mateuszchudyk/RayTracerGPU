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

#ifndef COMMON_H
#define COMMON_H

#include <CL/cl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "mathematics.h"

class Raytracer;

class OpenCLManager {
	friend Raytracer;

	private:
		OpenCLManager(){}
		OpenCLManager(const OpenCLManager&){}
		OpenCLManager& operator=(OpenCLManager &x){ return x; }

		cl_context context;
		cl_command_queue queue;
		cl_platform_id platform;
		cl_device_id device;

	public:
		~OpenCLManager();
		cl_context getContext() const;
		cl_command_queue getQueue() const;
		cl_platform_id getPlatformId() const;
		cl_device_id getDeviceId() const;
};

class OpenCLKernel {
	friend Raytracer;

	private:
		OpenCLKernel(){}
		OpenCLKernel(const OpenCLKernel&){}
		OpenCLKernel& operator=(OpenCLKernel &x){ return x; }
		bool create(OpenCLManager *manager, std::string filename, std::string kernelName);

		cl_program program;
		cl_kernel kernel;
		char *logs;

	public:
		~OpenCLKernel();
		cl_program getProgram() const;
		cl_kernel getKernel() const;
		const char *getBuildInfo() const;
		bool isErrors() const;
};

class Raytracer {
	public:
		static OpenCLManager *createOpenCLManager(); 
		static OpenCLManager *createOpenCLManager(char *filename);
		static OpenCLManager *createOpenCLManager(unsigned platform, unsigned device);
		static bool saveOpenCLManager(OpenCLManager *manager);
		static OpenCLKernel *createOpenCLKernel(OpenCLManager *manager, std::string filename, std::string kernelName);
};


#endif