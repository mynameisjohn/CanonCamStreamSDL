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
		return -1;

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
	camApp.Start();

	float fPicSize = 0.5f;
	//CamDisplayWindow window( &camApp, "Test Canon SDK", 
	//	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN,
	//	3, 0, true,
	//	"../shaders/shader.vert", "../shaders/shader.frag",
	//	512, 512, 0.5f);

	bool bRun = true;
	while ( bRun )
	{
		SDL_Event e{ 0 };
		while ( SDL_PollEvent( &e ) )
		{
			if ( e.type == SDL_QUIT )
				bRun = false;
			else if ( e.type == SDL_KEYUP )
			{
				if ( e.key.keysym.sym == SDLK_ESCAPE )
					bRun = false;
				else if ( e.key.keysym.sym == SDLK_SPACE )
					// To take a picture, end evf mode, take picture, and start it back up
					camApp.GetCmdQueue()->push_back( new CompositeCommand( camApp.GetCamModel(), {
						new EndEvfCommand( camApp.GetCamModel() ),
						new TakePictureCommand( camApp.GetCamModel() ),
						new StartEvfCommand( camApp.GetCamModel() ) } ) );

			}
		}

		camApp.GetWindow()->Draw();
	}

	// Quit camera app
	camApp.Quit();

	if ( false == checkErr( EdsTerminateSDK() ) )
		return -1;
	
	return 0;
}