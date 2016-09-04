#include "CameraApp.h"

#include "OpenSessionCommand.h"

CameraApp::CameraApp( EdsCameraRef cam ) :
	m_CamModel( cam )
{}

CommandQueue * CameraApp::GetCmdQueue() const
{
	return (CommandQueue *) &m_CMDQueue;
}
CameraModel * CameraApp::GetCamModel() const
{
	return (CameraModel *) &m_CamModel;
}

void CameraApp::Quit()
{
	m_abRunning.store( false );
}

void CameraApp::Start()
{
	m_CMDQueue.push_back( new OpenSessionCommand( GetCamModel() ) );
	m_CMDQueue.push_back( new GetPropertyCommand( GetCamModel(), kEdsPropID_ProductName ) );
	m_CMDQueue.SetCloseCommand( new CloseSessionCommand( GetCamModel() ) );

	m_abRunning.store( true );
	m_CommandThread = std::thread( [this] ()
	{
		threadProc();
	} );
}

CameraApp::~CameraApp()
{
	Quit();
	m_CommandThread.join();
}

void CameraApp::threadProc()
{
	// When using the SDK from another thread in Windows, 
	// you must initialize the COM library by calling CoInitialize 
	::CoInitializeEx( NULL, COINIT_MULTITHREADED );

	bool bRunning_tl = m_abRunning.load();
	while ( bRunning_tl )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		auto pCMD = m_CMDQueue.pop();
		if ( pCMD )
			pCMD->execute();

		bRunning_tl = m_abRunning.load();
	}

	m_CMDQueue.clear( true );
}