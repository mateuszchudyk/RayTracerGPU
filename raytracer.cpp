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

#include "raytracer.h"

using namespace std;

// OPENCLMANAGER
OpenCLManager::~OpenCLManager() {
	clReleaseContext(context);
	clReleaseCommandQueue(queue);
}
cl_context OpenCLManager::getContext() const {
	return context;
}
cl_command_queue OpenCLManager::getQueue() const {
	return queue;
}
cl_platform_id OpenCLManager::getPlatformId() const {
	return platform;
}
cl_device_id OpenCLManager::getDeviceId() const {
	return device;
}

// OPENCLKERNEL
bool OpenCLKernel::create(OpenCLManager *manager, std::string filename, std::string kernelName) {
	cl_int error = CL_SUCCESS; 
	logs = NULL;

	program = NULL;
	kernel = NULL;
	ifstream file(filename.c_str(), std::ifstream::binary);
	if (!file) {
		cout << "Can't open file '" << filename << "'!" << endl;
		return false;
	}

	string str(istreambuf_iterator<char>(file), (istreambuf_iterator<char>()));
	const char* source = str.c_str();
	size_t programSize = str.length();

	program = clCreateProgramWithSource(manager->getContext(), 1, &source, &programSize, &error);
	if (error != CL_SUCCESS) {
		cout << "clCreateProgramWithSource: " << error << "!" << endl;
		return false;
	}

	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (error != CL_SUCCESS) {
		size_t size;
		clGetProgramBuildInfo(program, manager->getDeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
		logs = new char[size + 1];
		clGetProgramBuildInfo(program, manager->getDeviceId(), CL_PROGRAM_BUILD_LOG, size + 1, logs, NULL);
	}
	kernel = clCreateKernel(program, kernelName.c_str(), &error);
	if (error != CL_SUCCESS) {
		cout << "clCreateKernel: " << error << "!" << endl;
		return false;
	}

	return true;
}
OpenCLKernel::~OpenCLKernel() {
	delete[] logs;
	clReleaseProgram(program);
	clReleaseKernel(kernel);
}
cl_program OpenCLKernel::getProgram() const {
	return program;
}
cl_kernel OpenCLKernel::getKernel() const {
	return kernel;
}
const char *OpenCLKernel::getBuildInfo() const {
	return logs;
}
bool OpenCLKernel::isErrors() const {
	return (logs != NULL);
}

// COMMON
OpenCLManager *Raytracer::createOpenCLManager() {
	OpenCLManager *manager = new OpenCLManager();

	cl_int error = CL_SUCCESS;
	cl_uint platformNumber = 0;
	cl_uint deviceNumber = 0;
	cl_uint platform;
	cl_uint device;
	char buffer[1024];

	// platforms
	error = clGetPlatformIDs(0, NULL, &platformNumber);
	if (platformNumber == 0) {
		return NULL;
	}

	cl_platform_id* platformIds = new cl_platform_id[platformNumber];
	error = clGetPlatformIDs(platformNumber, platformIds, NULL);

	unsigned length = 50;
	unsigned tab = 10;
	string headers[] = { "ID: ", "Name: ", "Vendor: ", "Max const buffer:" };

	string hr = "";
	for (int i = 0; i < length; i++)
		hr += "-";

	for (cl_uint i = 0; i < platformNumber; i++) {
		error = clGetDeviceIDs(platformIds[i], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceNumber);
		if (deviceNumber == 0)
			continue;

		cout << hr << endl << headers[0];
		for (unsigned j = headers[0].length(); j < tab; j++)
			cout << " ";
		cout << i << endl;

		cout << headers[1];
		for (unsigned j = headers[1].length(); j < tab; j++)
			cout << " ";
		error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 1024, &buffer, NULL);
		cout << buffer << endl;

		cout << headers[2];
		for (unsigned j = headers[2].length(); j < tab; j++)
			cout << " ";
		error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, 1024, &buffer, NULL);
		cout << buffer << endl;
	}
	cout << hr << endl;
	platform = -1;
	while (platform < 0 || platform >= platformNumber) {
		cout << "Platform: ";
		cin >> platform;
	}
	cout << endl;

	// devices
	if (clGetDeviceIDs(platformIds[platform], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceNumber) != CL_SUCCESS) {
		return NULL;
	}
	cl_device_id* deviceIds = new cl_device_id[deviceNumber];
	if (clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, deviceNumber, deviceIds, &deviceNumber) != CL_SUCCESS) {
		return NULL;
	}

	headers[2] = "Version";
	for (cl_uint i = 0; i < platformNumber; i++) {
		cout << hr << endl << headers[0];
		for (unsigned j = headers[0].length(); j < tab; j++)
			cout << " ";
		cout << i << endl;

		cout << headers[1];
		for (unsigned j = headers[1].length(); j < tab; j++)
			cout << " ";
		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, 1024, &buffer, NULL);
		cout << buffer << endl;

		cout << headers[2];
		for (unsigned j = headers[2].length(); j < tab; j++)
			cout << " ";
		error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_VERSION, 1024, &buffer, NULL);
		cout << buffer << endl;
	}
	cout << hr << endl;
	device = -1;
	while (device < 0 || device >= deviceNumber) {
		cout << "Device: ";
		cin >> device;
	}
	cout << endl;

	manager->context = clCreateContext(0, deviceNumber, deviceIds, NULL, NULL, NULL);
	if (manager->context == NULL)
		return NULL;

	manager->queue = clCreateCommandQueue(manager->context, deviceIds[device], 0, &error);
	if (manager->queue == NULL)
		return NULL;
	manager->platform = platformIds[platform];
	manager->device = deviceIds[device];

	delete[] platformIds;
	delete[] deviceIds;

	return manager;
}
OpenCLManager *Raytracer::createOpenCLManager(char *filename) {
	return NULL;
}
OpenCLManager *Raytracer::createOpenCLManager(unsigned platform, unsigned device) {
	OpenCLManager *manager = new OpenCLManager();

	cl_int error = CL_SUCCESS;
	cl_uint platformNumber = 0;
	cl_uint deviceNumber = 0;

	// platforms
	error = clGetPlatformIDs(0, NULL, &platformNumber);
	if (platformNumber == 0) {
		return NULL;
	}

	cl_platform_id* platformIds = new cl_platform_id[platformNumber];
	error = clGetPlatformIDs(platformNumber, platformIds, NULL);

	// devices
	error = clGetDeviceIDs(platformIds[platform], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceNumber);
	cl_device_id* deviceIds = new cl_device_id[deviceNumber];
	error = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, deviceNumber, deviceIds, &deviceNumber);

	manager->context = clCreateContext(0, deviceNumber, deviceIds, NULL, NULL, NULL);
	if (manager->context == NULL)
		return false;

	manager->queue = clCreateCommandQueue(manager->context, deviceIds[device], 0, &error);
	manager->platform = platformIds[platform];
	manager->device = deviceIds[device];

	delete[] platformIds;
	delete[] deviceIds;

	return manager;
}
bool saveOpenCLManager(OpenCLManager *manager) {
	return true;
}
OpenCLKernel *Raytracer::createOpenCLKernel(OpenCLManager *manager, std::string filename, std::string kernelName) {
	OpenCLKernel *kernel = new OpenCLKernel();
	bool result = kernel->create(manager, filename, kernelName);
	if (result == false && kernel->isErrors() == false)
		return NULL;
	else
		return kernel;
}