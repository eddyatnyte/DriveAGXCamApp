/*
 * CCameraSample.h
 *
 *  Created on: Oct 10, 2024
 *      Author: Edaet Boloban
 */

#ifndef CCAMERA_H_
#define CCAMERA_H_

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <ctime>

#include <dw/core/base/Types.h>
#include <dw/core/base/Version.h>
#include <dw/core/context/Context.h>
#include <dw/image/Image.h>
#include <dw/interop/streamer/ImageStreamer.h>
#include <dw/rig/Rig.h>
#include <dw/sensors/common/Sensors.h>
#include <dw/sensors/camera/Camera.h>

#include <dwvisualization/image/Image.h>
#include <dwvisualization/interop/ImageStreamer.h>

#include <framework/Checks.hpp>

enum eLink
{
	LINK_0,
	LINK_1,
	LINK_2,
	LINK_3
};

enum eGroup
{
	GROUP_A,
	GROUP_B,
	GROUP_C,
	GROUP_D
};

#define MAX_CAMS 4

/**
 * This class uses the DriveWorks API in order to connect to the cameras and   * read out the frames.
 *
 * For the use of this class, a rig file is necessary with the according camera * name, that is stored in the SIPL Query.
 *
 * Make sure that you are only able create one instance of the class
 * since the SAL (Sensor Abstraction Layer) can only be started once,
 * as it is in the method initCamera().
 */
class CCamera
{
private:
	// Definition of the necessary handles
	dwContextHandle_t m_context;
	dwSensorHandle_t m_camera[MAX_CAMS];
	dwSALHandle_t m_sal;
	dwRigHandle_t m_rigConfig;

	// Image-Streamer from CUDA to CPU
	dwImageStreamerHandle_t m_streamerCUDA2CPU[MAX_CAMS];

	eGroup m_group;
	dwVersion m_sdkVersion;
	dwSensorParams m_senParams[MAX_CAMS];

	// stores the image and its properties
	uint8_t *m_data[MAX_CAMS];
	uint32_t m_height[MAX_CAMS];
	uint32_t m_width[MAX_CAMS];

	// describes where the cameras are connected at
	const char *m_linkAsString;
	const char *m_groupAsString;

	/**
	 * Initializes the DW context.
	 */
	void initContext();

	/**
	 * Initializes the parameters of the camera using the gmsl protocol.
	 * Starts the SAL and the Camera.
	 */
	void initCamera();

	/**
	 * Initializes an Image Streamer.
	 * The Image Streamer streams CUDA-Images to CPU-Images.
	 */
	void initImageStreamer();

	/**
	 * Calls all the init()-methods
	 */
	void init();

	/**
	 * Converts the Cuda-Image into a CPU-Image and saves
	 * the image data into m_data
	 *
	 * @param frameCUDA Cuda-Image
	 */
	void exportImage(dwImageHandle_t frameCUDA, int idx);

	/**
	 * Returns the current date and time in the following format:
	 * "[dd-mm-yyyy hh:mm:ss]"
	 *
	 * @return current date and time
	 */
	std::stringstream getCurrentTime();

	/**
	 * Converts the link into a string
	 */
	void convertLink2String(eLink link);

	/**
	 * Converts the group into a string
	 */
	void convertGroup2String(eGroup group);

	/**
	 * Converts the integer value into a Link (0 = LINK_0, ...)
	 *
	 * @return link as an integer
	 */
	eLink convertInt2Link(int link);

public:
	/**
	 * Initializes all attributes with DW_NULL_HANDLE
	 *
	 * Calls the init()-method
	 *
	 * @param group where the cameras are connected
	 */
	CCamera(eGroup group);

	/**
	 * Reads a single Frame from the connected Camera.
	 * Calls exportImage() in order to save the image data
	 *
	 * Has to be called continously, when creating a real-time application
	 */
	void getFrame();

	/**
	 * Returns the raw data of the recent captured frame
	 */
	uint8_t *getFrameData(int idx);

	/**
	 * Returns the height of the recent captured Frame
	 */
	uint32_t getHeight(int idx);

	/**
	 * Returns the width of the recent captured frame
	 */
	uint32_t getWidth(int idx);

	/**
	 * Releases all initialized Handles
	 */
	void releaseHandle();

	// makes sure, that only one instance of the class is created
	static int instancesCreated;
};

#endif /* CCAMERA_H_ */
