#include "CameraEventListener.h"

/*static*/ EdsError EDSCALLBACK  CameraEventListener::handleObjectEvent (
	EdsUInt32			inEvent,
	EdsBaseRef			inRef,
	EdsVoid *			inContext
	)
{
	CameraApp * pCamApp = (CameraApp *) inContext;

	switch ( inEvent )
	{
		case kEdsObjectEvent_DirItemRequestTransfer:
			pCamApp->GetCmdQueue()->push_back( new DownloadCommand( pCamApp->GetCamModel(), inRef ) );
			break;

		default:
			//Object without the necessity is released
			if ( inRef != NULL )
			{
				EdsRelease( inRef );
			}
			break;
	}

	return EDS_ERR_OK;
}

/*static*/ EdsError EDSCALLBACK  CameraEventListener::handlePropertyEvent (
	EdsUInt32			inEvent,
	EdsUInt32			inPropertyID,
	EdsUInt32			inParam,
	EdsVoid *			inContext
	)
{
	CameraApp * pCamApp = (CameraApp *) inContext;

	switch ( inEvent )
	{
		case kEdsPropertyEvent_PropertyChanged:
			pCamApp->GetCmdQueue()->push_back( new GetPropertyCommand( pCamApp->GetCamModel(), inPropertyID ) );
			break;

		case kEdsPropertyEvent_PropertyDescChanged:
			pCamApp->GetCmdQueue()->push_back( new GetPropertyDescCommand( pCamApp->GetCamModel(), inPropertyID ) );
			break;
	}

	return EDS_ERR_OK;
}

/*static*/ EdsError EDSCALLBACK  CameraEventListener::handleStateEvent (
	EdsUInt32			inEvent,
	EdsUInt32			inParam,
	EdsVoid *			inContext
	)
{

	CameraApp * pCamApp = (CameraApp *) inContext;

	switch ( inEvent )
	{
		case kEdsStateEvent_Shutdown:
			pCamApp->SetMode( CameraApp::Mode::Off );
			break;
	}

	return EDS_ERR_OK;
}