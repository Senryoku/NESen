#include "log.hpp"

namespace Log
{

const std::array<const char*, 5> _log_types = {
	"Info",
	"Success",
	"Warning",
	"Error",
	"Output"
};

const std::array<Color, 5> _log_types_colors = {
	LightBlue,
	Green,
	Yellow,
	Red,
	Reset
};

LogType 								_min_level = Success;
std::ostringstream						_log_line;
std::deque<LogLine>						_logs;
std::function<void(const LogLine& ll)>	_log_callback = _default_log_callback;
std::function<void(const LogLine& ll)>	_update_callback = _default_update_callback;

void _addLogLine(const LogLine& ll)
{
}

void _log(LogType lt)
{
	if(_logs.size() > BufferSize)
		_logs.pop_back();
	
	if(!_logs.empty() && _log_line.str() == _logs.front().message) {
		++_logs.front().repeat;
		
		if(_update_callback && lt >= _min_level)
			_update_callback(_logs.front());
	} else {
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		_logs.push_front(LogLine{
			now,
			lt,
			_log_line.str()
		});
	
		if(_log_callback && lt >= _min_level)
			_log_callback(_logs.front());
	}
	_log_line.str("");
}

};
