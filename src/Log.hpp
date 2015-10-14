#pragma once

#include <iostream>

class Log
{
public:
    Log(std::ostream* dest = &std::cout) :
		_dest(dest)
	{
		
	}

    template<typename T>
    Log& operator<<(const T& v)
	{
		if(_dest)
			(*_dest) << v;
        return *this;
    }
	  
	Log& operator<<(std::ostream& (*v) (std::ostream&))
	{
		if(_dest)
			(*_dest) << v;
        return *this;
    }
private:
    std::ostream* _dest;
};
