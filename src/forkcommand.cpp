#include "error.hpp"

#include <string>
#include <vector>

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

const int COMMAND_TIMEOUT = 40;
const int MAX_FORK_OUTPUT = 2000000;
const int COMMAND_BUFFER_SIZE = 10240;

std::string ForkCommand(const std::string& command)
{
    std::string result="";

    int pipeDescriptors[2];
    if ( pipe(pipeDescriptors) == -1 )
    {
	return GetErrorDescription(errno);
    }

    // Set the read end of the pipe to be nonblocking so that read
    // won't hang later on
    int flags = fcntl(pipeDescriptors[0], F_GETFL);
    if ( flags != -1 )
    {
	fcntl(pipeDescriptors[0], F_SETFL, flags | O_NONBLOCK);
    }
 
    pid_t childPid;
    if ( (childPid=fork()) == 0 )
    {
	// Child side of the fork

	// Redirect stdout to write-end of pipe
        if ( close(pipeDescriptors[0]) == -1 ||
	     close(STDIN_FILENO) == -1 ||
	     dup2(pipeDescriptors[1], STDOUT_FILENO) == -1 ||
	     dup2(pipeDescriptors[1], STDERR_FILENO) == -1 ||
	     close(pipeDescriptors[1]) == -1 )
	{
	    exit(1);
	}

	// Change processgroup so that we can kill the entire child process
	// group without killing ourselves
	if ( setpgid(0,0) == -1 )
	{
	    exit(1);
	}

	execlp("/bin/sh","/bin/sh","-c",command.c_str(),(char*)0);
    }
    else if ( childPid > 0 ) 
    {
	// Parent side of the fork

	close(pipeDescriptors[1]);
	time_t started = time(0);

	int totalOutputSize = 0;
	bool childDidNotExit = true;
	while( started+COMMAND_TIMEOUT > time(0) &&
	       totalOutputSize < MAX_FORK_OUTPUT)
	{
	    fd_set pipeSet;
	    FD_ZERO(&pipeSet);
	    FD_SET(pipeDescriptors[0],&pipeSet);

	    // wait COMMAND_TIMEOUT seconds
	    struct timeval timeout;
	    timeout.tv_sec = std::max<long>(started + COMMAND_TIMEOUT - time(0), 0);
	    timeout.tv_usec = 0;

	    // Wait for data
	    int selectRes = select(pipeDescriptors[0]+1,&pipeSet,0,0,&timeout);
	    if ( selectRes > 0 && FD_ISSET(pipeDescriptors[0],&pipeSet) )
	    {
		int bytes = 0;
		std::vector<char> buffer(COMMAND_BUFFER_SIZE, 0);
		while ((bytes=read(pipeDescriptors[0], &buffer[0],
				   static_cast<size_t>(buffer.size()-1))) > 0
		       && totalOutputSize < MAX_FORK_OUTPUT ) {
		    totalOutputSize += bytes;
		    result += &buffer[0];
		}
	    }
	    // If the process is done we quit
	    int status;
	    if ( waitpid(childPid,&status,WNOHANG) > 0 )
	    {
	        childDidNotExit = false;
		break;
	    }
	}
	if ( childDidNotExit &&
	     (started+COMMAND_TIMEOUT <= time(0) ||
	      totalOutputSize >= MAX_FORK_OUTPUT) )
	{
	    // Kill child processgroup, nanosleep gives time for child to die
	    int status;
	    struct timespec sleepInfo;
	    sleepInfo.tv_sec=0;
	    sleepInfo.tv_nsec=10;
	    long counter = 0;
	    do
	    {
		// Attempt to send regular SIGTERM a few times,
		// if that fails enough times we start sending SIGKILL
		if ( counter > 100000 )
		{
		    kill(-childPid, SIGKILL);
		}
		else
		{
		    kill(-childPid, SIGTERM);
		    ++counter;
		}
		nanosleep(&sleepInfo, 0);
	    } while ( waitpid(-childPid, &status, WNOHANG) == 0 );

	    // Wait for all processes in the process group
	    while ( waitpid(-childPid, &status, 0) > 0 )
	    {
		;
	    }
	}
    }
    else
    {
        // Fork failed
        result = GetErrorDescription(errno);
    }
    close(pipeDescriptors[0]);
    return result;
}
