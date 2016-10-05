#include "CommandQueue.h"

CommandQueue::CommandQueue()
{

}

CommandQueue::~CommandQueue()
{
	clear();
}

void CommandQueue::clear( bool bClose )
{
	std::lock_guard<std::mutex> lg( m_muCommandMutex );
	m_liCommands.clear();

	if ( bClose && m_pCloseCommand )
	{
		m_pCloseCommand->execute();
		m_pCloseCommand.reset();
	}
}

CmdPtr CommandQueue::pop()
{
	std::lock_guard<std::mutex> lg( m_muCommandMutex );
	if ( m_liCommands.empty() )
		return nullptr;

	auto ret = std::move( m_liCommands.front() );
	m_liCommands.pop_front();
	return std::move( ret );
}

void CommandQueue::push_back( Command * pCMD )
{
	std::lock_guard<std::mutex> lg( m_muCommandMutex );
	m_liCommands.emplace_back( pCMD );
}

void CommandQueue::SetCloseCommand( Command * pCMD )
{
	std::lock_guard<std::mutex> lg( m_muCommandMutex );
	
	if ( pCMD )
		m_pCloseCommand = CmdPtr( pCMD );
	else
		m_pCloseCommand.reset();
}

void CommandQueue::waitTillCompletion()
{
	volatile bool bSpin = true;
	while ( bSpin )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
		std::lock_guard<std::mutex> lg( m_muCommandMutex );
		bSpin = !m_liCommands.empty();
	}
}