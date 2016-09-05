#include "CameraApp.h"

#include "OpenSessionCommand.h"

#include <SDL.h>

CameraApp::CameraApp( EdsCameraRef cam ) :
	m_CamModel( cam ),
	m_DisplayWindow( this, "Test Canon SDK",
					 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN,
					 3, 0, true,
					 "../shaders/shader.vert", "../shaders/shader.frag",
					 512, 512, 0.5f )
{
}

CommandQueue * CameraApp::GetCmdQueue() const
{
	return (CommandQueue *) &m_CMDQueue;
}

CameraModel * CameraApp::GetCamModel() const
{
	return (CameraModel *) &m_CamModel;
}

CamDisplayWindow * CameraApp::GetWindow() const
{
	return (CamDisplayWindow *) &m_DisplayWindow;
}

void CameraApp::Quit()
{
	m_abRunning.store( false );
	if ( m_CommandThread.joinable() )
		m_CommandThread.join();
}

void CameraApp::Start()
{
	m_CMDQueue.push_back( new CompositeCommand( GetCamModel(), {
		new OpenSessionCommand( GetCamModel() ),
		new GetPropertyCommand( GetCamModel(), kEdsPropID_ProductName ),
		new StartEvfCommand( GetCamModel() ),
		new GetPropertyCommand( GetCamModel(), kEdsPropID_Evf_Mode ),
		new GetPropertyCommand( GetCamModel(), kEdsPropID_Evf_OutputDevice),
		new DownloadEvfCommand( GetCamModel(), &m_DisplayWindow ),
	} ) );

	m_CMDQueue.SetCloseCommand( new CompositeCommand( GetCamModel(), {
		new EndEvfCommand( GetCamModel() ),
		new CloseSessionCommand( GetCamModel() ) } ) );

	m_abRunning.store( true );
	m_CommandThread = std::thread( [this] ()
	{
		threadProc();
	} );

	//std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
	//m_CMDQueue.push_back( );
}

CameraApp::~CameraApp()
{
	Quit();
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
		{
			if ( pCMD->execute() == false )
			{
				std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
				m_CMDQueue.push_back( pCMD.release() );
			}
		}

		bRunning_tl = m_abRunning.load();
	}

	m_CMDQueue.clear( true );
}