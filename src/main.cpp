#include "EDSDK.h"
#include "EDSDKTypes.h"

#include "CameraEventListener.h"
#include "CameraApp.h"

#include "CamDisplayWindow.h"
#include "Drawable.h"
#include "Shader.h"
#include "GLCamera.h"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_events.h>

#include <iostream>
#include <vector>



bool checkErr( EdsError err )
{
	return err == EDS_ERR_OK;
}

EdsCameraRef GetCamera()
{
	bool isSDKLoaded = checkErr( EdsInitializeSDK() );
	if ( isSDKLoaded == false )
		return nullptr;

	EdsCameraListRef cameraList = nullptr;
	EdsCameraRef camera = nullptr;
	EdsUInt32 count = 0;

	if ( checkErr( EdsGetCameraList( &cameraList ) ) == false )
	{
		EdsTerminateSDK();
		return nullptr;
	}

	if ( false == checkErr( EdsGetChildCount( cameraList, &count ) ) )
	{
		EdsTerminateSDK();
		return nullptr;
	}

	if ( false == checkErr( EdsGetChildAtIndex( cameraList, 0, &camera ) ) )
	{
		EdsTerminateSDK();
		return nullptr;
	}

	EdsRelease( cameraList );

	EdsDeviceInfo deviceInfo;
	if ( false == checkErr( EdsGetDeviceInfo( camera, &deviceInfo ) ) )
	{
		EdsTerminateSDK();
		return nullptr;
	}

	std::cout << "Got camera " << deviceInfo.szDeviceDescription << std::endl;

	return camera;
}

int main( int argc, char ** argv )
{
	EdsCameraRef camera = GetCamera();
	if ( camera == nullptr )
	{
		std::cout << "Error getting camera, exiting program" << std::endl;
		return -1;
	}

	CameraApp camApp( camera );
	
	//Set Property Event Handler
	if ( false == checkErr( EdsSetPropertyEventHandler( camera, kEdsPropertyEvent_All, CameraEventListener::handlePropertyEvent, (EdsVoid *) &camApp ) ) )
		 return -1;

	//Set Object Event Handler
	if ( false == checkErr( EdsSetObjectEventHandler( camera, kEdsObjectEvent_All, CameraEventListener::handleObjectEvent, (EdsVoid *) &camApp ) ) )
		return -1;

	//Set State Event Handler
	if ( false == checkErr( EdsSetCameraStateEventHandler( camera, kEdsStateEvent_All, CameraEventListener::handleStateEvent, (EdsVoid *) &camApp ) ) )
		return -1;

	// Start camera app
	if ( argc > 1 )
	{
		const std::string strArg = argv[1];
		if ( strArg == "S" )
			camApp.SetMode( CameraApp::Mode::Streaming );
		if ( strArg == "A" )
			camApp.SetMode( CameraApp::Mode::Averaging );
		if ( strArg == "E" )
			camApp.SetMode( CameraApp::Mode::Equalizing );
		else
			camApp.SetMode( CameraApp::Mode::Streaming );
	}
	else
		camApp.SetMode( CameraApp::Mode::Streaming );

	// Map of keys to app modes
	const std::map<int, CameraApp::Mode> mapKeysToMode{
		{ SDLK_0, CameraApp::Mode::Off },
		{ SDLK_1, CameraApp::Mode::Streaming },
		{ SDLK_2, CameraApp::Mode::Averaging },
		{ SDLK_3, CameraApp::Mode::Equalizing },
		{ SDLK_KP_0, CameraApp::Mode::Off },
		{ SDLK_KP_1, CameraApp::Mode::Streaming },
		{ SDLK_KP_2, CameraApp::Mode::Averaging },
		{ SDLK_KP_3, CameraApp::Mode::Equalizing },
	};

	bool bRun = true;
	while ( bRun )
	{
		// Check SDL events
		SDL_Event e{ 0 };
		while ( SDL_PollEvent( &e ) )
		{
			// They can quit with winX or escape
			if ( e.type == SDL_QUIT )
				bRun = false;
			else if ( e.type == SDL_KEYUP )
			{
				if ( e.key.keysym.sym == SDLK_ESCAPE )
					bRun = false;
				// Press space to take a picture
				else if ( e.key.keysym.sym == SDLK_SPACE )
				{
					// Enqueue {End EVF, Take picture, Start EVF} as a composite
					camApp.GetCmdQueue()->push_back( new CompositeCommand( camApp.GetCamModel(), {
						new EndEvfCommand( camApp.GetCamModel() ),
						new TakePictureCommand( camApp.GetCamModel() ),
						new StartEvfCommand( camApp.GetCamModel() ) } ) );
					// Wait for all commands to complete
					camApp.GetCmdQueue()->waitTillCompletion();
					// Enequeue a download evf commmand to resume the stream
					camApp.GetCmdQueue()->push_back( new DownloadEvfCommand( camApp.GetCamModel(), camApp.GetWindow() ) );
				}
				else
				{
					auto itKey = mapKeysToMode.find( e.key.keysym.sym );
					if ( itKey != mapKeysToMode.end() )
					{
						camApp.SetMode( itKey->second );
					}
				}
			}
		}

		if ( camApp.GetMode() == CameraApp::Mode::Off )
			bRun = false;
		
		camApp.GetWindow()->Draw();
	}

	// Quit camera app
	camApp.SetMode( CameraApp::Mode::Off );

	if ( false == checkErr( EdsTerminateSDK() ) )
		return -1;
	
	return 0;
}