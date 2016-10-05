#pragma once

#include "Command.h"
#include "EDSDK.h"
#include <chrono>
#include <thread>

class SleepCommand : public Command
{
	uint32_t m_uSleepDur;
public:
	SleepCommand( CameraModel *model, uint32_t uSleepDur ) : Command( model ), m_uSleepDur( uSleepDur ) {}


	// Execute command	
	virtual bool execute()
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( m_uSleepDur ) );
		return true;
	}
};