//#ifdef _USING_OPENCL
#include "Platform.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "main_openmp.h"

#include "DllExport.h"
#include "CL/opencl.h"
#include "spng.h"

#define CL_DEVICE_PCI_BUS_ID_NV 0x4008
#define CL_DEVICE_PCI_SLOT_ID_NV 0x4009

#define CL_VENDOR_ID_NV 4318
#define CL_VENDOR_ID_AMD 0x1002

#define SLZ_CONTINUE_ON_FAIL_MSG(rcode, message) if (rcode != 0) { printf message; continue; }
#define SLZ_RETURN_ON_FAIL_MSG(rcode, message) if (rcode != 0) { printf message; return rcode; }

struct ClDeviceSort
{
	int priority;
	cl_platform_id platformId;
	cl_device_id deviceId;
};

struct AmdDeviceTopology
{
	cl_uint rawType;
	cl_uint rawData[5];
	cl_uint pcieType;
	cl_char pcieUnused[17];
	cl_char pcieBus;
	cl_char pcieDevice;
	cl_char pcieFunction;
};


cl_context s_ClCtx = NULL;


DLLEXPORT INT32 STDCALL TestOpenClLoad()
{
	spng_ctx* ctx = spng_ctx_new(0);
	struct spng_ihdr ihdr = {};
	int result = spng_get_ihdr( ctx, &ihdr);

	printf("[SLZ TexProc] Start OpenCL Initialization\n");
	// Find available platforms
	cl_uint numPlatforms = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS) return status;
	printf("[SLZ TexProc] Found %d Platforms", numPlatforms);
	cl_platform_id platformIDs[numPlatforms];
	status = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
	SLZ_RETURN_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform IDs\n"));
	


	// Iterate over the available platforms and get the number of devices
	for (int p = 0; p < numPlatforms; p++)
	{
		// Print platform's index and vendor name, don't bother with openCL return code error checking for the vendor name
		size_t vendorNameLen;
		clGetPlatformInfo(platformIDs[p], CL_PLATFORM_VENDOR, 0, NULL, &vendorNameLen);
		char vendorName[max(1, vendorNameLen)];
		clGetPlatformInfo(platformIDs[p], CL_PLATFORM_VENDOR, vendorNameLen, vendorName, NULL);
		printf("[SLZ TexProc] Platform %d: %s, \n", p, vendorName);

		// Get devices
		cl_uint numDevices = 0;
		clGetDeviceIDs(platformIDs[p], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
		printf("[SLZ TexProc] Platform %d has %d devices, \n", p, numDevices);

		if (numDevices <= 0) continue;

		cl_device_id deviceArray[numDevices];
		status = clGetDeviceIDs(platformIDs[p], CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, numDevices, deviceArray, 0);
		SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device IDs, error code %d\n", p, status));

		//Iterate over all devices for the current platform
		for (int d = 0; d < numDevices; d++)
		{
			printf("[SLZ TexProc] Platform %d Device %d:\n", p, d);
			size_t nameLen = 0;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_NAME, 0, NULL, &nameLen);
			char name[max(1, nameLen)];
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_NAME, nameLen, name, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d name, error code %d\n", p, d, status));
			printf("[SLZ TexProc]     Device Name:   %.*s\n", (int)nameLen, name);

			// Get device availability
			cl_bool deviceAvailable = false;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_AVAILABLE, sizeof(deviceAvailable), &deviceAvailable, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d availablity, error code %d\n", p, d, status));
			printf("[SLZ TexProc]     Available:     %s\n", deviceAvailable ? "true" : "false");

			// Get the vendor ID
			cl_uint vendorID = 0;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_VENDOR_ID, sizeof(vendorID), &vendorID, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d vendor ID, error code %d\n", p, d, status));
			printf("[SLZ TexProc]     Vendor ID:     %u\n", vendorID);

			size_t verLen = 0;
			status = clGetDeviceInfo(deviceArray[d], CL_DRIVER_VERSION, 0, NULL, &verLen);
			char version[max(1, nameLen)];
			status = clGetDeviceInfo(deviceArray[d], CL_DRIVER_VERSION, verLen, version, 0);
			printf("[SLZ TexProc]     Driver Version:   %.*s\n", (int)verLen, version);

			UINT64 deviceID = 0;
			if (vendorID == CL_VENDOR_ID_NV)
			{
				UINT32 nvBusID = 0;
				UINT32 nvSlotID = 0;
				status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_PCI_BUS_ID_NV, sizeof(nvBusID), &nvBusID, 0);
				if (status == 0)
				{
					status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_PCI_SLOT_ID_NV, sizeof(nvSlotID), &nvSlotID, 0);
					deviceID = ((UINT64)nvBusID) | ((UINT64)nvSlotID << 32);
					printf("[SLZ TexProc]     Device ID:     %lluX\n", deviceID);
				}
			}

			if (vendorID == CL_VENDOR_ID_AMD)
			{
				struct AmdDeviceTopology amdTopo;
				status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_TOPOLOGY_AMD, sizeof(amdTopo), &amdTopo, 0);
				if (status == 0)
				{
					deviceID = (UINT64)amdTopo.pcieDevice | ((UINT64)amdTopo.pcieBus << 32);
					printf("[SLZ TexProc]     Device ID:     %lluX\n", deviceID);
				}
			}
			// Get device type
			cl_device_type deviceType = 0;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d type, error code %d\n", p, d, status));
			printf("[SLZ TexProc]     Type:          ");
			// Note: OpenCL says a device can be multiple types simultaneously, and cl_device_type is a bitfield. So we need to check against every type
			if (deviceType & CL_DEVICE_TYPE_DEFAULT) printf("CL_DEVICE_TYPE_DEFAULT ");
			if (deviceType & CL_DEVICE_TYPE_CPU) printf("CL_DEVICE_TYPE_CPU ");
			if (deviceType & CL_DEVICE_TYPE_GPU) printf("CL_DEVICE_TYPE_GPU ");
			if (deviceType & CL_DEVICE_TYPE_ACCELERATOR) printf("CL_DEVICE_TYPE_ACCELERATOR ");
			if (deviceType & CL_DEVICE_TYPE_CUSTOM) printf("CL_DEVICE_TYPE_CUSTOM ");
			printf("\n");

			cl_bool imageSupport = 0;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_IMAGE_SUPPORT, sizeof(imageSupport), &imageSupport, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d image support, error code %d\n", p, d, status));
			printf("[SLZ TexProc]     Image Support: %s\n", imageSupport ? "true" : "false");
			
		}
	}
	return 0;
}

DLLEXPORT INT32 STDCALL InitCLContext()
{
	cl_uint numPlatforms = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS) return status;
	cl_platform_id platformIDs[numPlatforms];
	status = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
	SLZ_RETURN_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform IDs\n"));

	bool foundSuitableDevice = false;
	cl_platform_id selectedPlatform = 0;
	cl_device_id selectedDevice = 0;
	int selectedPlatformIdx = 0;
	int selectedDeviceIdx = 0;
	// Iterate over the available platforms and get the number of devices
	for (int p = 0; p < numPlatforms; p++)
	{
		// Get devices
		cl_uint numDevices = 0;
		status = clGetDeviceIDs(platformIDs[p], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
		if (status != 0 || numDevices <= 0) continue;

		cl_device_id deviceArray[numDevices];
		status = clGetDeviceIDs(platformIDs[p], CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, numDevices, deviceArray, 0);
		SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device IDs, error code %d\n", p, status));

		//Iterate over all devices for the current platform
		for (int d = 0; d < numDevices; d++)
		{
			
			// Get device availability
			cl_bool deviceAvailable = false;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_AVAILABLE, sizeof(deviceAvailable), &deviceAvailable, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d availablity, error code %d\n", p, d, status));
			if (!deviceAvailable)
			{
				printf("[SLZ TexProc] Platform %d device %d is unavailable", p, d);
			}
		
			cl_bool imageSupport = 0;
			status = clGetDeviceInfo(deviceArray[d], CL_DEVICE_IMAGE_SUPPORT, sizeof(imageSupport), &imageSupport, 0);
			SLZ_CONTINUE_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to get platform %d device %d image support, error code %d\n", p, d, status));
			if (!imageSupport)
			{
				printf("[SLZ TexProc] Platform %d device %d  has no image support", p, d);
			}

			if (deviceAvailable && imageSupport)
			{
				foundSuitableDevice = true;
				selectedPlatform = platformIDs[p];
				selectedPlatformIdx = p;
				selectedDevice = deviceArray[d];
				selectedDeviceIdx = d;
				break;
			}
		}
		if (foundSuitableDevice)
		{
			break;
		}
	}

	if (!foundSuitableDevice)
	{
		printf("[SLZ TexProc] FATAL: Failed to find an available OpenCL device with image support!");
		return 1;
	}
	cl_context_properties ctxProps[] = {
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)selectedPlatform,
		0,
		0,
	};
	s_ClCtx = clCreateContext(ctxProps, 1, &selectedDevice, NULL, NULL, &status);
	SLZ_RETURN_ON_FAIL_MSG(status, ("[SLZ TexProc] Failed to create OpenCL context, Error code: %d\n", status));
	cl_uint numFormats = 0;
	status = clGetSupportedImageFormats(s_ClCtx, CL_MEM_KERNEL_READ_AND_WRITE, CL_MEM_OBJECT_IMAGE2D, 0, NULL, &numFormats);
	if (numFormats == 0)
	{
		printf("OpenCL device (Platform %d, device %d) does not support read-write images. Bad!\n", selectedPlatformIdx, selectedDeviceIdx);
	}
	else
	{
		printf("OpenCL device (Platform %d, device %d) supports %d read-write image formats\n", selectedPlatformIdx, selectedDeviceIdx, numFormats);
	}
	status = clGetSupportedImageFormats(s_ClCtx, CL_MEM_READ_ONLY, CL_MEM_OBJECT_IMAGE2D, 0, NULL, &numFormats);
	if (numFormats > 0)
	{
		cl_image_format formats[numFormats];
		status = clGetSupportedImageFormats(s_ClCtx, CL_MEM_READ_ONLY, CL_MEM_OBJECT_IMAGE2D, numFormats, formats, NULL);
		for (int fIdx = 0; fIdx < numFormats; fIdx++)
		{
			printf("Format 0x%X: channel order 0x%X\n", formats[fIdx].image_channel_data_type, formats[fIdx].image_channel_order);
		}
		//CL_SNORM_INT16
	}
	return 0;
}

DLLEXPORT void STDCALL FuckYou()
{
	TestScaleImage(NULL, (int2) { 0, 0 }, NULL, (int2) { 0, 0 });
}


DLLEXPORT int STDCALL TestOpenCl(void* imagePointer, int width, int height)
{
	if (s_ClCtx == NULL)
	{
		int result = InitCLContext();
		if (result != 0)
			return 1;
	}



	return 0;
}

DLLEXPORT void STDCALL ReleaseCLContext()
{
	if (s_ClCtx != NULL)
	{
		clReleaseContext(s_ClCtx);
	}
}

DLLEXPORT void STDCALL TestImg()
{
	InitializeLibPath();

}
//#endif