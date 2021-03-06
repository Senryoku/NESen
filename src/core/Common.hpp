#pragma once

#include <sstream>
#include <iomanip>
#include <cassert>

#include <tools/log.hpp>

using byte_t = char;
using ubyte_t = uint8_t;
using word_t = uint8_t;
using addr_t = uint16_t;
using checksum_t = uint16_t;

inline int from_2c_to_signed(word_t src)
{
	return (src & 0x80) ? -((~src + 1) & 0xFF) : src;
}
	
struct color_t
{ 
	word_t r, g, b, a;
	
	color_t(word_t _r = 255, word_t _g = 255, word_t _b = 255, word_t _a = 255) :
		r(_r),
		g(_g),
		b(_b),
		a(_a)
	{
	}
	
	color_t& operator=(const color_t& v) =default;
	bool operator==(const color_t& v)
	{
		return r == v.r && g == v.g && b == v.b && a == v.a;
	}
	
	color_t& operator=(word_t val)
	{
		r = g = b = a = val;
		return *this;
	}
	
	bool operator==(word_t val)
	{
		return r == g && g == b && b == a && a == val;
	}
	
	color_t operator*(float val) const
	{
		color_t c;
		c.r = std::max(0.0f, std::min(255.0f, r * val));
		c.g = std::max(0.0f, std::min(255.0f, g * val));
		c.b = std::max(0.0f, std::min(255.0f, b * val));
		c.a = std::max(0.0f, std::min(255.0f, a * val));
		return c;
	}
};
	
template<typename T>
struct HexaGen
{
	T v;
	HexaGen(T _t) : v(_t) {}
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const HexaGen<T>& t)
{
	return os << "0x" << std::hex << std::setw(sizeof(T) * 2) << std::setfill('0') << (int) t.v;
}

using Hexa = HexaGen<addr_t>;
using Hexa8 = HexaGen<word_t>;

namespace config
{
	extern std::string _executable_folder;

	// 'Parsing' executable location
	void set_folder(const char* exec_path);
	std::string to_abs(const std::string& path);
}
