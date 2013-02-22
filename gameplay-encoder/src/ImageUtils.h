#ifndef IMAGEUTILS_H__
#define IMAGEUTILS_H__

#include "Base.h"

namespace gameplay
{

class ImageUtils
{
public:
	static int convertTexture2Png(const std::string& imgSource, const std::string& imgDestinationPng);
	static bool isPngFile(const char* path);
	static int isPowerOfTwo(const char* path);
	static int isPowerOfTwo(unsigned int x);
};

}

#endif
