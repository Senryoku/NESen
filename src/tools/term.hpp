#pragma once

#include <iostream>
#include <chrono>

namespace term
{
	using Color = const char*;
	constexpr Color Black		= "\033[0;30m";
	constexpr Color Red			= "\033[0;31m";
	constexpr Color Green		= "\033[0;32m";
	constexpr Color Brown		= "\033[0;33m";
	constexpr Color Blue		= "\033[0;34m";
	constexpr Color Magenta		= "\033[0;35m";
	constexpr Color Cyan		= "\033[0;36m";
	constexpr Color LightGray	= "\033[0;37m";
	constexpr Color DarkGray	= "\033[1;30m";
	constexpr Color LightRed	= "\033[1;31m";
	constexpr Color LightGreen	= "\033[1;32m";
	constexpr Color Yellow		= "\033[1;33m";
	constexpr Color LightBlue	= "\033[1;34m";
	constexpr Color LightPurple	= "\033[1;35m";
	constexpr Color LightCyan	= "\033[1;36m";
	constexpr Color White		= "\033[1;37m";
	constexpr Color Reset		= "\033[0m";

	inline std::string get_time()
	{
		std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char mbstr[100];
		std::strftime(mbstr, sizeof(mbstr), "%H:%M:%S", std::localtime(&time));
		return std::string{mbstr};
	}

	inline void _print(std::ostream& stream)
	{
		stream << Reset << std::endl;
	}
	
	template<typename T, typename... Args>
	inline void _print(std::ostream& stream, T&& t, Args&&... args)
	{
		stream << t;
		_print(stream, args...);
	}
	
	template<typename... Args>
	inline void print(std::ostream& stream, Color color, Args&&... args)
	{
		stream << "[" << get_time() << "] " << color;
		_print(stream, args...);
	}

	template<typename... Args>
	inline void error(Args&&... args)
	{
		print(std::cerr, Red, args...);
	}

	template<typename... Args>
	inline void warn(Args&&... args)
	{
		print(std::cout, Yellow, args...);
	}

	template<typename... Args>
	inline void log(Args&&... args)
	{
		print(std::cout, Reset, args...);
	}

	template<typename... Args>
	inline void success(Args&&... args)
	{
		print(std::cout, Green, args...);
	}
}
