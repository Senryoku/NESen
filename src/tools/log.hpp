#pragma once

#include <array>
#include <deque>
#include <iostream>
#include <sstream>
#include <chrono>
#include <functional>

namespace Log
{
using Color = const char*;
const Color Black		= "\033[0;30m";
const Color Red			= "\033[0;31m";
const Color Green		= "\033[0;32m";
const Color Brown		= "\033[0;33m";
const Color Blue		= "\033[0;34m";
const Color Magenta		= "\033[0;35m";
const Color Cyan		= "\033[0;36m";
const Color LightGray	= "\033[0;37m";
const Color DarkGray	= "\033[1;30m";
const Color LightRed	= "\033[1;31m";
const Color LightGreen	= "\033[1;32m";
const Color Yellow		= "\033[1;33m";
const Color LightBlue	= "\033[1;34m";
const Color LightPurple	= "\033[1;35m";
const Color LightCyan	= "\033[1;36m";
const Color White		= "\033[1;37m";
const Color Reset		= "\033[0m";
	
constexpr size_t BufferSize = 100;

enum LogType
{
	Info		= 0,
	Success		= 1,
	Warning		= 2,
	Error		= 3,
	Print		= 4
};

extern LogType _min_level;
extern const std::array<const char*, 5>	_log_types;
extern const std::array<Color, 5> _log_types_colors;

struct LogLine
{
	std::time_t	 time;
	LogType		 type;
	std::string	 message;
	unsigned int repeat = 1;
	
	inline operator std::string() const
	{
		return str();
	}
	
	inline std::string str() const
	{
		std::string full;
		char mbstr[100];
		std::strftime(mbstr, sizeof(mbstr), "%H:%M:%S", std::localtime(&time));
		full = std::string(Cyan) + "[" + std::string(mbstr) + "] " + Reset + _log_types_colors[type] + _log_types[type] + Reset;
		if(repeat > 1)
			full += " <*" + std::to_string(repeat) + ">";
		full += ": " + message;
		return full;
	}
};

extern std::ostringstream						_log_line;
extern std::deque<LogLine>						_logs;
extern std::function<void(const LogLine& ll)>	_log_callback;
extern std::function<void(const LogLine& ll)>	_update_callback;

inline void _default_log_callback(const LogLine& ll) {
	std::cout << std::endl << "\x1B[31m" << ll.str();
}

inline void _default_update_callback(const LogLine& ll) {
	std::cout << '\r' << ll.str();
}

void _log(LogType lt);

template<typename T, typename ...Args>
inline void _log(LogType lt, const T& msg, Args... args)
{
	_log_line << msg;
	_log(lt, args...);
}

template<typename ...Args>
inline void print(Args... args)
{
	_log(LogType::Print, args...);
}

template<typename ...Args>
inline void info(Args... args)
{
	_log(LogType::Info, args...);
}

template<typename ...Args>
inline void warn(Args... args)
{
	_log(LogType::Warning, args...);
}

template<typename ...Args>
inline void error(Args... args)
{
	_log(LogType::Error, args...);
}

template<typename ...Args>
inline void success(Args... args)
{
	_log(LogType::Success, args...);
}

}
