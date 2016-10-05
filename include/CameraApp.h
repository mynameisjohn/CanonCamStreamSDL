#pragma once

#include "CommandQueue.h"
#include "CameraModel.h"
#include "CamDisplayWindow.h"

class CameraApp
{
	CommandQueue m_CMDQueue;
	CameraModel m_CamModel;
	CamDisplayWindow m_DisplayWindow;

	std::atomic_bool m_abRunning;
	std::atomic_bool m_abEvfRunning;

	std::thread m_CommandThread;

public:
	CameraApp( EdsCameraRef cam );
	CommandQueue * GetCmdQueue() const;
	CameraModel * GetCamModel() const;
	CamDisplayWindow * GetWindow() const;

	void Quit();

	void Start();

	~CameraApp();

	void SetEvfRunning( bool bEvfRunning );
	bool GetIsEvfRunning() const;

private:
	void threadProc();
};
