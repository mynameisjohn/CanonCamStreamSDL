#include "CameraApp.h"

#include "OpenSessionCommand.h"

#include <SDL.h>

CameraApp::CameraApp( EdsCameraRef cam ) :
	m_CamModel( this, cam ),
	m_DisplayWindow( this, "Test Canon SDK",
					 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1056, 704, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN,
					 3, 0, true,
					 "shader.vert", "shader.frag",
					 1.f )
{
	m_abRunning.store( false );
	m_abEvfRunning.store( false );
	m_aMode.store( CameraApp::Mode::Off );
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

void CameraApp::quit()
{
	m_abRunning.store( false );
	m_abEvfRunning.store( false );
	m_aMode.store( CameraApp::Mode::Off );

	if ( m_CommandThread.joinable() )
		m_CommandThread.join();
}

void CameraApp::start()
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
	quit();
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

void CameraApp::SetEvfRunning( bool bEvfRunning )
{
	m_abEvfRunning.store( bEvfRunning );
}

bool CameraApp::GetIsEvfRunning() const
{
	return m_abEvfRunning.load();
}

std::ostream& operator<<( std::ostream& o, const CameraApp::Mode m )
{
	switch ( m )
	{
		case CameraApp::Mode::Off:
			o << "Off";
			break;
		case CameraApp::Mode::Streaming:
			o << "Streaming";
			break;
		case CameraApp::Mode::Averaging:
			o << "Averaging";
			break;
		case CameraApp::Mode::Equalizing:
			o << "Equalizing";
			break;
	}
	return o;
}

void CameraApp::SetMode( const CameraApp::Mode m )
{
	CameraApp::Mode curMode = m_aMode.load();
	if ( curMode != m )
	{
		std::cout << "App transitioning from mode " << curMode << " to mode " << m << std::endl;
		switch ( m )
		{
			case Mode::Off:
				quit();
				break;
			case Mode::Streaming:
			case Mode::Averaging:
			case Mode::Equalizing:
				if ( m_aMode.load() == Mode::Off )
					start();
				break;
		}
		m_aMode.store( m );
	}
}

CameraApp::Mode CameraApp::GetMode() const
{
	return m_aMode.load();
}