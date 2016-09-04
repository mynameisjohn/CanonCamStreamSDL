#include "EDSDK.h"
#include "EDSDKTypes.h"

#include "CameraEventListener.h"
#include "CameraApp.h"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_events.h>

#include <iostream>

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

	SDL_Window * pWindow( nullptr );
	SDL_GLContext glContext( nullptr );
	pWindow = SDL_CreateWindow( "TestCanonSDK",
								SDL_WINDOWPOS_UNDEFINED,
								SDL_WINDOWPOS_UNDEFINED,
								500,
								500,
								SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	glContext = SDL_GL_CreateContext( pWindow );
	if ( glContext == nullptr )
	{
		std::cout << "Error creating opengl context" << std::endl;
		return false;
	}

	//Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if ( glewError != GLEW_OK )
	{
		printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
		return -1;
	}

	SDL_GL_SetSwapInterval( 1 );

	glClearColor( 1, 1, 0, 1 );

	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_LESS );
	glEnable( GL_MULTISAMPLE_ARB );

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
					camApp.GetCmdQueue()->push_back( new TakePictureCommand( camApp.GetCamModel() ) );
			}
		}
	}

	// Quit camera app
	camApp.Quit();

	SDL_DestroyWindow( pWindow );
	SDL_GL_DeleteContext( glContext );

	if ( false == checkErr( EdsTerminateSDK() ) )
		return -1;
	
	return 0;
}