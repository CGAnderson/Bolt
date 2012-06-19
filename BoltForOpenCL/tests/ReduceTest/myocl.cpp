#include <iostream>
#include <fstream>
#include <algorithm>
#include <tchar.h>

#include "myocl.h"

void  CHECK_OPENCL_ERROR(cl_int err, const char * name)
{
	if (err != CL_SUCCESS)
	{
		std::cerr << "ERROR: " << name
			<< " (" << err << ")" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void printDevice(const cl::Device &d) {

	std::cout 
		<< d.getInfo<CL_DEVICE_NAME>()
		<< " CU="<< d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() 
		<< " Freq="<< d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "Mhz"
		<< "\n";
};


MyOclContext initOcl(cl_int clDeviceType, int deviceIndex) 
{
	MyOclContext ocl;

	cl_int err;
	std::vector< cl::Platform > platformList;
	cl::Platform::get(&platformList);
	CHECK_OPENCL_ERROR(platformList.size()!=0 ? CL_SUCCESS : -1, "cl::Platform::get");
	std::cerr << "info: Found: " << platformList.size() << " platforms." << std::endl;

	//FIXME - add support for multiple vendors here, right now just pick the first platform.
	std::string platformVendor;
	platformList[0].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor);
	std::cout << "info: platform is: " << platformVendor << "\n";
	cl_context_properties cprops[3] = 
	{CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 0};



	cl::Context context(
		clDeviceType, 
		cprops,
		NULL,
		NULL,
		&err);
	CHECK_OPENCL_ERROR(err, "Context::Context()");  
	ocl._context = context;


	std::vector<cl::Device> devices;
	devices = context.getInfo<CL_CONTEXT_DEVICES>();
	CHECK_OPENCL_ERROR(
		devices.size() > 0 ? CL_SUCCESS : -1, "devices.size() > 0");


	if (0) {
		int deviceCnt = 0;
		std::for_each(devices.begin(), devices.end(), [&] (cl::Device &d) {
			std::cout << "#" << deviceCnt << "  ";
			printDevice(d);
			deviceCnt ++;
		});
	}

	std::cout << "info: selected device #" << deviceIndex << "  ";
	printDevice(devices[deviceIndex]);
	ocl._device = devices[deviceIndex];

	cl::CommandQueue q(ocl._context, ocl._device);
	ocl._queue = q;

	return ocl;
};



cl::Kernel compileKernelCpp(const MyOclContext &ocl, const char *kernelFile, const char *kernelName, std::string compileOpt)
{
	cl::Program program;
	try 
	{
		std::ifstream infile(kernelFile);
		if (infile.fail()) {
			TCHAR cCurrentPath[FILENAME_MAX];
			if (_tgetcwd(cCurrentPath, sizeof(cCurrentPath) / sizeof(TCHAR))) {
				std::wcout <<  _T( "CWD=" ) << cCurrentPath << std::endl;
			};
			std::cout << "ERROR: can't open file '" << kernelFile << std::endl;
			throw;
		};
		std::string str((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());

		const char* kStr = str.c_str();
		cl::Program::Sources source(1, std::make_pair(kStr,strlen(kStr)));

		program = cl::Program(ocl._context, source);
		std::vector<cl::Device> devices;
		devices.push_back(ocl._device);
		compileOpt += " -x clc++";
		program.build(devices, compileOpt.c_str());

		cl::Kernel kernel(program, kernelName);

		return kernel;
	}
	catch (cl::Error err) {
		std::cerr << "ERROR: "<< err.what()<< "("<< err.err()<< ")"<< std::endl;
		std::cout << "Error building program\n";
		std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(ocl._device) << std::endl;
		std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(ocl._device) << std::endl;
		std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(ocl._device) << std::endl;
	}
};

