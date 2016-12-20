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
	bool execute() override;
};