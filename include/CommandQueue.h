#pragma once

#include "Command.h"

#include <mutex>
#include <thread>
#include <list>
#include <memory>
#include <atomic>
#include <chrono>

class CommandQueue
{
	std::mutex m_muCommandMutex;
	std::list<CmdPtr> m_liCommands;
	CmdPtr m_pCloseCommand;
public:
	CommandQueue();
	~CommandQueue();
	CmdPtr pop();
	void push_back( Command * pCMD );
	void clear( bool bClose = false );
	void waitTillCompletion();

	void SetCloseCommand( Command * pCMD );
};