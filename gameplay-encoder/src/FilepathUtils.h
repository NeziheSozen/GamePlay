#ifndef FILEPATHUTILS_H__
#define FILEPATHUTILS_H__

#include "Base.h"

namespace gameplay
{

class FilepathUtils
{
public:
	static std::string getAbsoluteTextureSourcePath(std::string originalTexturePath, std::string originalModelPath, std::string fileExtension = ".png");
	static std::string combineFileNameWithFilePath(const std::string& filePath, const std::string& fileName);
	static std::string uriDecode(const std::string & sSrc);
    static void escapeFilePath(std::string& filePath);
    
    static void convertToUNIXFilePath(std::string& filePath);
    
    static void replaceOccurencesOfStringWithString(std::string& input, const std::string& oldStr, const std::string& newStr);
};

}

#endif
