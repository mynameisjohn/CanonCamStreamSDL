#pragma once

#include "CameraModel.h"

class Command
{
protected:
	CameraModel * _model;
public:
	Command( CameraModel * pModel ) :_model( pModel ) {}

	CameraModel* getCameraModel() { return _model; }

	// Execute command	
	virtual bool execute() = 0;
};

#include "DownloadCommand.h"
#include "OpenSessionCommand.h"
#include "CloseSessionCommand.h"
#include "GetPropertyCommand.h"
#include "GetPropertyDescCommand.h"
#include "TakePictureCommand.h"