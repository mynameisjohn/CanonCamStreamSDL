/******************************************************************************
*                                                                             *
*   PROJECT : EOS Digital Software Development Kit EDSDK                      *
*      NAME : DownloadCommand.h	                                              *
*                                                                             *
*   Description: This is the Sample code to show the usage of EDSDK.          *
*                                                                             *
*                                                                             *
*******************************************************************************
*                                                                             *
*   Written and developed by Camera Design Dept.53                            *
*   Copyright Canon Inc. 2006-2008 All Rights Reserved                        *
*                                                                             *
*******************************************************************************/

#pragma once

#include "Command.h"
#include "EDSDK.h"

class DownloadCommand : public Command
{
private:
	EdsDirectoryItemRef _directoryItem;

public:
	DownloadCommand(CameraModel *model, EdsDirectoryItemRef dirItem) 
			: _directoryItem(dirItem), Command(model){}


	virtual ~DownloadCommand()
	{
		//Release item
		if(_directoryItem != NULL)
		{
			EdsRelease( _directoryItem);
			_directoryItem = NULL;
		}
	}


	// Execute command 	
	virtual bool execute()
	{
		EdsError				err = EDS_ERR_OK;
		EdsStreamRef			stream = NULL;

		//Acquisition of the downloaded image information
		EdsDirectoryItemInfo	dirItemInfo;
		err = EdsGetDirectoryItemInfo( _directoryItem, &dirItemInfo);

		//Make the file stream at the forwarding destination
		if(err == EDS_ERR_OK)
		{	
			err = EdsCreateFileStream(dirItemInfo.szFileName, kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
		}	

		////Set Progress
		//if(err == EDS_ERR_OK)
		//{
		//	err = EdsSetProgressCallback(stream, ProgressFunc, kEdsProgressOption_Periodically, this);
		//}


		//Download image
		if(err == EDS_ERR_OK)
		{
			err = EdsDownload( _directoryItem, dirItemInfo.size, stream);
		}

		//Forwarding completion
		if(err == EDS_ERR_OK)
		{
			err = EdsDownloadComplete( _directoryItem);
		}

		//Release Item
		if(_directoryItem != NULL)
		{
			err = EdsRelease( _directoryItem);
			_directoryItem = NULL;
		}

		//Release stream
		if(stream != NULL)
		{
			err = EdsRelease(stream);
			stream = NULL;
		}

		return true;
	}
};