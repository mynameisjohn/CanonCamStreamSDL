#pragma once

#include "CommandQueue.h"
#include "CameraModel.h"
#include "CamDisplayWindow.h"

class CameraApp
{
public:
	CameraApp( EdsCameraRef cam );
	~CameraApp();

	enum class Mode
	{
		Off,
		Streaming,
		Averaging,
		Equalizing
	};

	CommandQueue * GetCmdQueue() const;
	CameraModel * GetCamModel() const;
	CamDisplayWindow * GetWindow() const;

	void SetMode( const Mode m );
	Mode GetMode() const;

	void SetEvfRunning( bool bEvfRunning );
	bool GetIsEvfRunning() const;

private:
	void threadProc();
	void quit();
	void start();

	CommandQueue m_CMDQueue;
	CameraModel m_CamModel;
	CamDisplayWindow m_DisplayWindow;

	std::atomic_bool m_abRunning;
	std::atomic_bool m_abEvfRunning;
	std::atomic<Mode> m_aMode;

	std::thread m_CommandThread;
};
