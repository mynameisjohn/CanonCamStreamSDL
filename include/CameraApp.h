#pragma once

#include "CommandQueue.h"
#include "CameraModel.h"

class CameraApp
{
	CommandQueue m_CMDQueue;
	CameraModel m_CamModel;

	std::atomic_bool m_abRunning;

	std::thread m_CommandThread;

public:
	CameraApp( EdsCameraRef cam );
	CommandQueue * GetCmdQueue() const;
	CameraModel * GetCamModel() const;

	void Quit();

	void Start();

	~CameraApp();

private:
	void threadProc();
};
