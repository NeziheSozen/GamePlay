#ifndef IMAGEUTILS_H__
#define IMAGEUTILS_H__

#include "Base.h"

namespace gameplay
{

class ImageUtils
{
public:
	static int convertJpg2Png(const std::string& imgJpg, const std::string& imgPng);
	static bool isPngFile(const char* path);
	static int isPowerOfTwo(const char* path);
	static int isPowerOfTwo(unsigned int x);
};

}

#endif
