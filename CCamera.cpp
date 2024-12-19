/*
 * CCameraSample.cpp
 *
 *  Created on: Oct 10, 2024
 *      Author: Edaet Boloban
 */

#include "CCamera.h"

int CCamera::instancesCreated = 0;

using namespace std;

void CCamera::initContext()
{
	// Initialize the DW Context
	CHECK_DW_ERROR(dwGetVersion(&m_sdkVersion));
	CHECK_DW_ERROR(dwInitialize(&m_context, m_sdkVersion, NULL));

	// Print start message
	cout << getCurrentTime().str() << "Initialize DriveWorks SDK v"
		 << m_sdkVersion.major << "." << m_sdkVersion.minor << "."
		 << m_sdkVersion.patch << endl
		 << endl;
}

void CCamera::initCamera()
{
	// Initialize the Sensor Abstraction layer
	CHECK_DW_ERROR(dwSAL_initialize(&m_sal, m_context));

	for (unsigned int i = 0; i < MAX_CAMS; i++)
	{
		// the complete configuration is automatically loaded by dwRig module
		CHECK_DW_ERROR(dwRig_initializeFromFile(&m_rigConfig, m_context, "rig_imx623.json"));

		// get rig parsed protocol from dwRig
		const char *protocol = nullptr;
		CHECK_DW_ERROR(dwRig_getSensorProtocol(&protocol, 0, m_rigConfig));
		const char *params = nullptr;
		CHECK_DW_ERROR(dwRig_getSensorParameterUpdatedPath(&params, 0, m_rigConfig));

		convertGroup2String(m_group);
		convertLink2String(convertInt2Link(i));

		size_t len = strlen(params) + strlen(m_linkAsString) + strlen(m_groupAsString) + 3;

		char *temp = new char[len];
		strcpy(temp, params);
		strcat(temp, ",");
		strcat(temp, m_linkAsString);
		strcat(temp, ",");
		strcat(temp, m_groupAsString);

		// Specifiy the gmsl protocol for the fourth camera
		m_senParams[i].parameters = temp;
		m_senParams[i].protocol = protocol;

		// Create the sensor (here: camera)
		CHECK_DW_ERROR(dwSAL_createSensor(&m_camera[i], m_senParams[i], m_sal));

		delete[] temp;
	}

	// Start the Sensor Abstraction Layer
	CHECK_DW_ERROR(dwSAL_start(m_sal));

	// Start the cameras
	for (int i = 0; i < MAX_CAMS; i++)
		CHECK_DW_ERROR(dwSensor_start(m_camera[i]));
}

void CCamera::initImageStreamer()
{
	for (int i = 0; i < MAX_CAMS; i++)
	{
		// Get the CUDA Image Properties
		dwImageProperties cudaProp{};
		CHECK_DW_ERROR(
			dwSensorCamera_getImageProperties(&cudaProp,
											  DW_CAMERA_OUTPUT_CUDA_RGBA_UINT8, m_camera[i]));

		// Initialize the ImageStreamer Module
		CHECK_DW_ERROR(
			dwImageStreamer_initialize(&m_streamerCUDA2CPU[i], &cudaProp,
									   DW_IMAGE_CPU, m_context));
	}
}

void CCamera::init()
{
	initContext();
	initCamera();
	initImageStreamer();
}

void CCamera::convertGroup2String(eGroup group)
{
	switch (group)
	{
	case GROUP_A:
		m_groupAsString = "interface=csi-ab";
		break;

	case GROUP_B:
		m_groupAsString = "interface=csi-cd";
		break;

	case GROUP_C:
		m_groupAsString = "interface=csi-ef";
		break;

	case GROUP_D:
		m_groupAsString = "interface=csi-gh";
		break;

	default:
		m_groupAsString = "interface=csi-ab";
		break;
	}
}

CCamera::CCamera(eGroup group)
{
	// If the class is called twice,
	if (++instancesCreated > 1)
	{
		// the instantiation will result in an error
		cerr << getCurrentTime().str() << "Cameras have already been started" << endl;
		cerr << getCurrentTime().str() << "Aborting..." << endl;
		return;
	}

	m_context = DW_NULL_HANDLE;
	for (int i = 0; i < MAX_CAMS; i++)
		m_camera[i] = DW_NULL_HANDLE;
	m_sal = DW_NULL_HANDLE;
	for (int i = 0; i < MAX_CAMS; i++)
		m_streamerCUDA2CPU[i] = DW_NULL_HANDLE;
	m_rigConfig = DW_NULL_HANDLE;
	m_group = group;
	init();
}

void CCamera::exportImage(dwImageHandle_t frameCUDA, int idx)
{
	dwTime_t timeout = 500000;
	dwImageHandle_t imageHandle;
	dwImageCPU *imgCPU;

	// stream the CUDA frame to the CPU domain
	CHECK_DW_ERROR(dwImageStreamer_producerSend(frameCUDA, m_streamerCUDA2CPU[idx]));

	// receive the streamed CPU image as a handle
	CHECK_DW_ERROR(
		dwImageStreamer_consumerReceive(&imageHandle, timeout,
										m_streamerCUDA2CPU[idx]));

	// Converts the Image into the CPU format
	CHECK_DW_ERROR(dwImage_getCPU(&imgCPU, imageHandle));

	// save the image data and properties
	m_data[idx] = imgCPU->data[0];
	m_width[idx] = imgCPU->prop.width;
	m_height[idx] = imgCPU->prop.height;

	// Return the consumed image
	CHECK_DW_ERROR(
		dwImageStreamer_consumerReturn(&imageHandle, m_streamerCUDA2CPU[idx]));

	// Notify the producer that the work is done
	CHECK_DW_ERROR(
		dwImageStreamer_producerReturn(nullptr, timeout,
									   m_streamerCUDA2CPU[idx]));
}

void CCamera::getFrame()
{
	for (int i = 0; i < MAX_CAMS; i++)
	{
		// has to be called twice
		dwCameraFrameHandle_t frameHandle = DW_NULL_HANDLE;
		CHECK_DW_ERROR(dwSensorCamera_readFrame(&frameHandle, 333333, m_camera[i]));

		// Create a image out of the taken Frame
		dwImageHandle_t imageHandle = DW_NULL_HANDLE;
		CHECK_DW_ERROR(
			dwSensorCamera_getImage(&imageHandle,
									DW_CAMERA_OUTPUT_CUDA_RGBA_UINT8, frameHandle));

		// Passes the Image to the Image-Streamer
		exportImage(imageHandle, i);

		// Returns frame to thecamera
		CHECK_DW_ERROR(dwSensorCamera_returnFrame(&frameHandle));
	}
}

uint8_t *CCamera::getFrameData(int idx)
{
	return m_data[idx];
}

uint32_t CCamera::getHeight(int idx)
{
	return m_height[idx];
}

uint32_t CCamera::getWidth(int idx)
{
	return m_width[idx];
}

void CCamera::releaseHandle()
{
	// Release Image Streamer
	cout << endl;
	for (int i = 0; i < MAX_CAMS; i++)
		CHECK_DW_ERROR(dwImageStreamer_release(m_streamerCUDA2CPU[i]));
	cout << getCurrentTime().str() << "Image Streamer released" << endl;

	for (int i = 0; i < MAX_CAMS; i++)
	{
		// Stop the camera
		CHECK_DW_ERROR(dwSensor_stop(m_camera[i]));
		cout << getCurrentTime().str() << "CameraClient: Client stopped - Link " << i
			 << endl;

		// Release camera sensor
		CHECK_DW_ERROR(dwSAL_releaseSensor(m_camera[i]));
		cout << getCurrentTime().str() << "Camera master released - Link " << i << endl;
	}

	// Release SAL module of the SDK
	CHECK_DW_ERROR(dwSAL_release(m_sal));
	cout << getCurrentTime().str() << "Driveworks SDK released" << endl;
}

stringstream CCamera::getCurrentTime()
{
	stringstream ss;

	// Get the current time
	time_t t = time(nullptr);
	tm *now = localtime(&t);

	// Save date and time in a stringstream and return it
	ss << '[' << setw(2) << setfill('0') << now->tm_mday << '-' << setw(2)
	   << setfill('0') << now->tm_mon + 1 << '-' << now->tm_year + 1900
	   << ' ' << setw(2) << setfill('0') << now->tm_hour << ':' << setw(2)
	   << setfill('0') << now->tm_min << ':' << setw(2) << setfill('0')
	   << now->tm_sec << "]: ";
	return ss;
}

void CCamera::convertLink2String(eLink link)
{
	// converts the link into a string
	switch (link)
	{
	case LINK_0:
		m_linkAsString = "link=0";
		break;

	case LINK_1:
		m_linkAsString = "link=1";
		break;

	case LINK_2:
		m_linkAsString = "link=2";
		break;

	case LINK_3:
		m_linkAsString = "link=3";
		break;

	default:
		m_linkAsString = "link=0";
		break;
	}
}

eLink CCamera::convertInt2Link(int link)
{
	switch (link)
	{
	case 0:
		return LINK_0;
	case 1:
		return LINK_1;
	case 2:
		return LINK_2;
	case 3:
		return LINK_3;
	default:
		return LINK_0;
	}
}