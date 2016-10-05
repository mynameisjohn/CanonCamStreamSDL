/******************************************************************************
*                                                                             *
*   PROJECT : EOS Digital Software Development Kit EDSDK                      *
*      NAME : StartEvfCommand.h												  *
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
//#include "CameraApp.h"
#include "EDSDK.h"

class StartEvfCommand : public Command
{

public:
	StartEvfCommand(CameraModel *model) : Command(model){}

    // Execute command	
	bool execute() override;
};