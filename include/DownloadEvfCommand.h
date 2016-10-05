/******************************************************************************
*                                                                             *
*   PROJECT : EOS Digital Software Development Kit EDSDK                      *
*      NAME : DownloadEvfCommand.h	                                          *
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


//typedef struct _EVF_DATASET 
//{
//	EdsStreamRef	stream; // JPEG stream.
//	EdsUInt32		zoom;
//	EdsRect			zoomRect;
//	EdsPoint		imagePosition;
//	EdsUInt32		histogram[256 * 4]; //(YRGB) YRGBYRGBYRGBYRGB....
//	EdsSize			sizeJpegLarge;
//}EVF_DATASET;


class DownloadEvfCommand : public Command
{
public:
	struct Receiver
	{
		virtual bool HandleEVFImage() = 0;
	} *m_pReceiver{ nullptr };

	DownloadEvfCommand(CameraModel *model, Receiver * pReceiver = nullptr) : 
		Command(model), 
		m_pReceiver(pReceiver)
	{}

    // Execute command	
	bool execute() override;
};