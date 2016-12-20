#include <libraw/libraw.h>

#include "Command.h"
#include "CameraApp.h"


bool EndEvfCommand::execute()
{
	EdsError err = EDS_ERR_OK;


	// Get the current output device.
	EdsUInt32 device = _model->getEvfOutputDevice();

	// Do nothing if the remote live view has already ended.
	if ( (device & kEdsEvfOutputDevice_PC) == 0 )
	{
		return true;
	}

	// Get depth of field status.
	EdsUInt32 depthOfFieldPreview = _model->getEvfDepthOfFieldPreview();

	// Release depth of field in case of depth of field status.
	if ( depthOfFieldPreview != 0 )
	{
		depthOfFieldPreview = 0;
		err = EdsSetPropertyData( _model->getCameraObject(), kEdsPropID_Evf_DepthOfFieldPreview, 0, sizeof( depthOfFieldPreview ), &depthOfFieldPreview );

		// Standby because commands are not accepted for awhile when the depth of field has been released.
		if ( err == EDS_ERR_OK )
		{
			Sleep( 500 );
		}
	}

	// Change the output device.
	if ( err == EDS_ERR_OK )
	{
		device &= ~kEdsEvfOutputDevice_PC;
		err = EdsSetPropertyData( _model->getCameraObject(), kEdsPropID_Evf_OutputDevice, 0, sizeof( device ), &device );
	}

	//Notification of error
	if ( err != EDS_ERR_OK )
	{
		// It retries it at device busy
		if ( err == EDS_ERR_DEVICE_BUSY )
		{
			return false;
		}

		// Retry until successful.
		return false;
	}

	_model->GetCamApp()->SetEvfRunning( false );

	return true;
}

bool DownloadEvfCommand::execute()
{
	EdsError err = EDS_ERR_OK;

	EdsEvfImageRef evfImage = NULL;
	EdsStreamRef stream = NULL;
	EdsUInt32 bufferSize = 2;// *16 * 16;

							 // Exit unless during live view.
	if ( (_model->getEvfOutputDevice() & kEdsEvfOutputDevice_PC) == 0 )
	{
		return true;
	}

	if ( m_pReceiver )
	{
		return m_pReceiver->HandleEVFImage();
	}

	return true;
}

bool StartEvfCommand::execute()
{
	EdsError err = EDS_ERR_OK;

	/// Change settings because live view cannot be started
	/// when camera settings are set to gdo not perform live view.h
	EdsUInt32 evfMode = _model->getEvfMode();

	if ( evfMode == 0 )
	{
		evfMode = 1;

		// Set to the camera.
		err = EdsSetPropertyData( _model->getCameraObject(), kEdsPropID_Evf_Mode, 0, sizeof( evfMode ), &evfMode );
	}


	if ( err == EDS_ERR_OK )
	{
		// Get the current output device.
		EdsUInt32 device = _model->getEvfOutputDevice();

		// Set the PC as the current output device.
		device |= kEdsEvfOutputDevice_PC;

		// Set to the camera.
		err = EdsSetPropertyData( _model->getCameraObject(), kEdsPropID_Evf_OutputDevice, 0, sizeof( device ), &device );
	}

	//Notification of error
	if ( err != EDS_ERR_OK )
	{
		// It doesn't retry it at device busy
		if ( err == EDS_ERR_DEVICE_BUSY )
		{
			return false;
		}
	}

	_model->GetCamApp()->SetEvfRunning( true );

	return true;
}

bool TakePictureCommand::execute()
{
	EdsError err = EDS_ERR_OK;
	bool	 locked = false;

	//Taking a picture
	err = EdsSendCommand( _model->getCameraObject(), kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_Completely );
	EdsSendCommand( _model->getCameraObject(), kEdsCameraCommand_PressShutterButton, kEdsCameraCommand_ShutterButton_OFF );


	//Notification of error
	if ( err != EDS_ERR_OK )
	{
		// It retries it at device busy
		if ( err == EDS_ERR_DEVICE_BUSY )
		{
			return true;
		}
	}

	return true;
}

bool DownloadCommand::execute()
{
	// Execute command 	
	
		EdsError				err = EDS_ERR_OK;
		EdsStreamRef			stream = NULL;

		//Acquisition of the downloaded image information
		EdsDirectoryItemInfo	dirItemInfo;
		err = EdsGetDirectoryItemInfo( _directoryItem, &dirItemInfo );

		//Make the file stream at the forwarding destination
		if ( err == EDS_ERR_OK )
		{
			//err = EdsCreateFileStream( dirItemInfo.szFileName, kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream );
			err = EdsCreateMemoryStream( dirItemInfo.size, &stream );
		}

		//Download image
		if ( err == EDS_ERR_OK )
		{
			err = EdsDownload( _directoryItem, dirItemInfo.size, stream );
		}

		//Forwarding completion
		if ( err == EDS_ERR_OK )
		{
			err = EdsDownloadComplete( _directoryItem );
		}

		//Release Item
		if ( _directoryItem != NULL )
		{
			err = EdsRelease( _directoryItem );
			_directoryItem = NULL;
		}

		LibRaw l;
		void * pData( nullptr );
		EdsUInt64 uDataSize( 0 );
		err |= EdsGetLength( stream, &uDataSize );
		err |= EdsGetPointer( stream, &pData );

		int lrRet = l.open_buffer( pData, uDataSize );
		lrRet = l.unpack();
		lrRet = l.raw2image();
		l.recycle();

		//Release stream
		if ( stream != NULL )
		{
			err = EdsRelease( stream );
			stream = NULL;
		}

		return true;
}