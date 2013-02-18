#ifndef FILEPATHUTILS_H__
#define FILEPATHUTILS_H__

#include "Base.h"

namespace gameplay
{

class FilepathUtils
{
public:
	static std::string getAbsoluteTextureSourcePath(std::string originalTexturePath, std::string originalModelPath);
	static std::string combineFileNameWithFilePath(const std::string& filePath, const std::string& fileName);
	static std::string uriDecode(const std::string & sSrc);
};

}

#endif
