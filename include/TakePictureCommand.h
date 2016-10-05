/******************************************************************************
*                                                                             *
*   PROJECT : EOS Digital Software Development Kit EDSDK                      *
*      NAME : TakePictureCommand.h	                                          *
*                                                                             *
*   Description: This is the Sample code to show the usage of EDSDK.          *
*                                                                             *
*                                                                             *
*******************************************************************************
*                                                                             *
*   Written and developed by Camera Design Dept.53                            *
*   Copyright Canon Inc. 2006 All Rights Reserved                             *
*                                                                             *
*******************************************************************************/

#pragma once

#include "Command.h"
#include "EDSDK.h"

class TakePictureCommand : public Command
{

public:
	TakePictureCommand(CameraModel *model) : Command(model){}


	// Execute command	
	bool execute() override;
};