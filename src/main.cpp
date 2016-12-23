#include <libraw/libraw.h>
#include "EDSDK.h"
#include "EDSDKTypes.h"

#include "CameraEventListener.h"
#include "CameraApp.h"

#include "CamDisplayWindow.h"
#include "Drawable.h"
#include "Shader.h"
#include "GLCamera.h"

#include "TelescopeComm.h"

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

template <typename T>
void balanceWhiteSimple( std::vector<cv::Mat_<T> > &src, cv::Mat &dst, const float inputMin, const float inputMax,
						 const float outputMin, const float outputMax, const float p )
{
	/********************* Simple white balance *********************/
	const float s1 = p; // low quantile
	const float s2 = p; // high quantile

	int depth = 2; // depth of histogram tree
	if ( src[0].depth() != CV_8U )
		++depth;
	int bins = 16; // number of bins at each histogram level

	int nElements = int( pow( (float) bins, (float) depth ) );
	// number of elements in histogram tree

	for ( size_t i = 0; i < src.size(); ++i )
	{
		std::vector<int> hist( nElements, 0 );

		typename cv::Mat_<T>::iterator beginIt = src[i].begin();
		typename cv::Mat_<T>::iterator endIt = src[i].end();

		for ( typename cv::Mat_<T>::iterator it = beginIt; it != endIt; ++it )
			// histogram filling
		{
			int pos = 0;
			float minValue = inputMin - 0.5f;
			float maxValue = inputMax + 0.5f;
			T val = *it;

			float interval = float( maxValue - minValue ) / bins;

			for ( int j = 0; j < depth; ++j )
			{
				int currentBin = int( ( val - minValue + 1e-4f ) / interval );
				++hist[pos + currentBin];

				pos = ( pos + currentBin ) * bins;

				minValue = minValue + currentBin * interval;
				maxValue = minValue + interval;

				interval /= bins;
			}
		}

		int total = int( src[i].total() );

		int p1 = 0, p2 = bins - 1;
		int n1 = 0, n2 = total;

		float minValue = inputMin - 0.5f;
		float maxValue = inputMax + 0.5f;

		float interval = ( maxValue - minValue ) / float( bins );

		for ( int j = 0; j < depth; ++j )
			// searching for s1 and s2
		{
			while ( n1 + hist[p1] < s1 * total / 100.0f )
			{
				n1 += hist[p1++];
				minValue += interval;
			}
			p1 *= bins;

			while ( n2 - hist[p2] > ( 100.0f - s2 ) * total / 100.0f )
			{
				n2 -= hist[p2--];
				maxValue -= interval;
			}
			p2 = p2 * bins - 1;

			interval /= bins;
		}

		src[i] = ( outputMax - outputMin ) * ( src[i] - minValue ) / ( maxValue - minValue ) + outputMin;
	}
	/****************************************************************/

	dst.create(/**/ src[0].size(), CV_MAKETYPE( src[0].depth(), int( src.size() ) ) /**/ );
	cv::merge( src, dst );
}

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <stdio.h>

void TestLibRaw()
{
	// Open the CR2 file with LibRaw, unpack, and create image
	LibRaw lrProc;
	assert( LIBRAW_SUCCESS == lrProc.open_file( "Lights1.nef" ) );
	assert( LIBRAW_SUCCESS == lrProc.unpack() );
	assert( LIBRAW_SUCCESS == lrProc.raw2image() );

	// Get image dimensions
	int width = lrProc.imgdata.sizes.iwidth;
	int height = lrProc.imgdata.sizes.iheight;

	// Print the first 4 values of the first 4 rows
	for ( int y = 0; y < 4; y++ )
	{
		for ( int x = 0; x < 4; x++ )
		{
			int idx = y * width + x;
			ushort * uRGBG = lrProc.imgdata.image[idx];
			printf( "[%04d, %04d, %04d, %04d]  ", uRGBG[0], uRGBG[1], uRGBG[2], uRGBG[3] );
		}
		printf( "\n" );
	}

	// Create a buffer of ushorts containing the pixel values of the
	// "BG Bayered" image (even rows are RGRGRG..., odd are GBGBGB...)
	std::vector<ushort> vBayerData;
	for ( int y = 0; y < height; y++ )
	{
		for ( int x = 0; x < width; x++ )
		{
			// Get pixel idx
			int idx = y * width + x;

			// Each pixel is an array of 4 shorts rgbg
			ushort * uRGBG = lrProc.imgdata.image[idx];
			int rowMod = ( y ) % 2; // 0 if even, 1 if odd
			int colMod = ( x + rowMod ) % 2; // 1 if even, 0 if odd
			int clrIdx = 2 * rowMod + colMod;
			ushort val = uRGBG[clrIdx] << 4;
			vBayerData.push_back( val );
		}
	}

	// Get rid of libraw image, construct openCV mat
	lrProc.recycle();
	cv::Mat imgBayer( height, width, CV_16UC1, vBayerData.data() );

	// Debayer image, get output
	cv::Mat imgDeBayer;
	cv::cvtColor( imgBayer, imgDeBayer, CV_BayerBG2BGR );

	assert( imgDeBayer.type() == CV_16UC3 );

	//imgDeBayer.convertTo( imgBayer, CV_32FC3, 1. / USHRT_MAX );
	std::vector<cv::Mat_<ushort>> v;
	cv::split( imgBayer, v );

	//const float fGamma = 2.f;
	//const float fMax = USHRT_MAX + 1.f;
	//std::vector<unsigned char> lut( 256 );
	//for ( unsigned int i = 0; i < 256; i++ )
	//{
	//	lut[i] = cv::saturate_cast<unsigned char>( powf( (float) ( float( i ) / (float) 255.f ), fGamma ) * 255.f );
	//}

	//imgDeBayer.convertTo( imgBayer, CV_32FC3, 1. / USHRT_MAX );
	//imgBayer.convertTo( imgDeBayer, CV_8UC3, 0xff );

	//imgBayer = imgDeBayer.clone();
	//cv::LUT( imgBayer, lut, imgDeBayer );

	//cv::cvtColor( imgDeBayer, imgDeBayer, CV_BGR2YCrCb );

	//std::vector<cv::Mat> v;
	//cv::split( imgDeBayer, v );

	//// The pixel color values were 12 bit, but our data is 16 bit
	//// transform the range [0, 4095] to [0:65535] (multiply by 16)
	////imgDeBayer *= 16;

	//// Remap to float [0, 1], perform gamma correction
	//v[0].convertTo( v[0], CV_32F, 1. / USHRT_MAX );

	//float fGamma = 4.2f;
	//cv::pow( v[0], fGamma, v[0] );
	//
	//v[0].convertTo( v[0], CV_16U, USHRT_MAX );
	//cv::merge( v, imgDeBayer );
	//cv::cvtColor( imgDeBayer, imgDeBayer, CV_YCrCb2BGR );

	// Display image
	cv::namedWindow( "CR2 File", CV_WINDOW_FREERATIO );
	cv::imshow( "CR2 File", imgDeBayer );
	cv::waitKey();
}

int main( int argc, char ** argv )
{
	//TestLibRaw();
	
	EdsCameraRef camera = GetCamera();
	if ( camera == nullptr )
	{
		std::cout << "Error getting camera, exiting program" << std::endl;
		return -1;
	}

	CameraApp camApp( camera );
	std::unique_ptr<TelescopeComm> upTelescopeComm;
	
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
		else if ( strArg == "A" )
			camApp.SetMode( CameraApp::Mode::Averaging );
		else if ( strArg == "E" )
			camApp.SetMode( CameraApp::Mode::Equalizing );
		else
			camApp.SetMode( CameraApp::Mode::Streaming );

		if ( argc > 5 )
		{
			float fFilterRadius, fDilationRadius, fHWHM, fIntensityThreshold;
			std::stringstream( argv[2] ) >> fFilterRadius;
			std::stringstream( argv[3] ) >> fDilationRadius;
			std::stringstream( argv[4] ) >> fHWHM;
			std::stringstream( argv[5] ) >> fIntensityThreshold;
			camApp.GetWindow()->SetStarFinderParams( fFilterRadius, fDilationRadius, fHWHM, fIntensityThreshold );

			std::cout << fFilterRadius << ", " << fDilationRadius << ", " << fHWHM << ", " << fIntensityThreshold << std::endl;

			if ( argc > 6 )
			{
				upTelescopeComm.reset( new TelescopeComm( argv[6] ) );
			}
		}
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
		try
		{
			// Check SDL events
			SDL_Event e { 0 };
			while ( SDL_PollEvent( &e ) )
			{
				// They can quit with winX or escape
				if ( e.type == SDL_QUIT )
					bRun = false;
				else if ( e.type == SDL_KEYUP || e.type == SDL_KEYDOWN )
				{
					if ( upTelescopeComm )
					{
						int nSpeed = 4500;
						int nSlewX( 0 ), nSlewY( 0 );
						if ( e.type == SDL_KEYDOWN )
							upTelescopeComm->GetSlew( &nSlewX, &nSlewY );

						bool bSlew = false;
						if ( e.type == SDL_KEYDOWN )
						{
							switch ( e.key.keysym.sym )
							{
								case SDLK_LEFT:
									nSlewX = -nSpeed;
									bSlew = true;
									break;
								case SDLK_RIGHT:
									nSlewX = nSpeed;
									bSlew = true;
									break;
								case SDLK_UP:
									nSlewY = nSpeed;
									bSlew = true;
									break;
								case SDLK_DOWN:
									nSlewY = -nSpeed;
									bSlew = true;
									break;
								default:
									break;
							}
						}
						else
							bSlew = true;

						if ( bSlew )
							upTelescopeComm->SetSlew( nSlewX, nSlewY );
					}

					if ( e.type == SDL_KEYUP )
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
			}

			if ( camApp.GetMode() == CameraApp::Mode::Off )
				bRun = false;


			camApp.GetWindow()->Draw();
		}
		catch ( std::runtime_error& e )
		{
			std::cout << e.what() << std::endl;
			break;
		}
	}

	// Quit camera app
	camApp.SetMode( CameraApp::Mode::Off );

	if ( false == checkErr( EdsTerminateSDK() ) )
		return -1;
	
	return 0;
}