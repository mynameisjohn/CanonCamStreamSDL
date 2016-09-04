#pragma once

#include "CameraModel.h"
#include <memory>
#include <list>
#include <algorithm>

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

using CmdPtr = std::unique_ptr<Command>;
class CompositeCommand : public Command
{
	std::list<CmdPtr> m_liCommands;

public:
	CompositeCommand( CameraModel * pModel, std::initializer_list<Command *> liCommands ) : Command( pModel )
	{
		for ( Command * pCMD : liCommands )
			m_liCommands.emplace( m_liCommands.end(), pCMD );
	}
	bool execute() override
	{
		return std::all_of(m_liCommands.begin(), m_liCommands.end(), 
							[] ( const CmdPtr& pCMD )
		{
			return pCMD->execute();
		} );
	}
};

#include "DownloadCommand.h"
#include "OpenSessionCommand.h"
#include "CloseSessionCommand.h"
#include "GetPropertyCommand.h"
#include "GetPropertyDescCommand.h"
#include "TakePictureCommand.h"
#include "StartEvfCommand.h"
#include "EndEvfCommand.h"
#include "DownloadEvfCommand.h"