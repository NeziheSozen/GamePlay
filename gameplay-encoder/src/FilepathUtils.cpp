#include "FilepathUtils.h"

namespace gameplay
{
	const char HEX2DEC[256] =
	{
		/*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
		/* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

		/* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

		/* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

		/* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
	};

	std::string FilepathUtils::getAbsoluteTextureSourcePath(std::string originalTexturePath, std::string originalModelPath, std::string fileExtension)
	{
		bool isRelativePath = false;
		std::string resultPath("");
		int index = originalTexturePath.find("file://");
		if (index != std::string::npos)
		{
			originalTexturePath = originalTexturePath.substr(7);
		}
		resultPath = originalTexturePath;
		if(originalTexturePath[0] == '.' && originalTexturePath[1] != '.')
		{
			resultPath = FilepathUtils::combineFileNameWithFilePath(originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.') + 1) );
			isRelativePath = true;
		}
		else if(originalTexturePath[0] == '.' && originalTexturePath[1] == '.')
		{
			resultPath = FilepathUtils::combineFileNameWithFilePath( originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.')) );
			isRelativePath = true;
		}
	
		if (!isRelativePath) 
		{
			int lastSlashIndex = originalTexturePath.find_last_of("/");
			if (lastSlashIndex == std::string::npos) 
			{    
				resultPath = FilepathUtils::combineFileNameWithFilePath(originalModelPath, originalTexturePath);
			}
		}
        
        /* After this point, a valid path should be stored in 'resultPath' */

		resultPath = FilepathUtils::uriDecode(resultPath);
        
        std::string expectedFilePath = resultPath.substr(0, resultPath.find_last_of("."));
        expectedFilePath.append(fileExtension);
        
		// check if resultPath exists
		FILE *fp = fopen(expectedFilePath.c_str(), "rb");
		if(fp == NULL)
		{
			// this happens when we get a path like this: "/texture.png"
			resultPath = FilepathUtils::combineFileNameWithFilePath(originalModelPath, originalTexturePath.substr(0));
		}
		else
		{
			fclose(fp);
		}
    
		return FilepathUtils::uriDecode(resultPath);
	}
    
	std::string FilepathUtils::combineFileNameWithFilePath(const std::string& filePath, const std::string& fileName)
	{
		std::string combinedFileName = "";
    
		if ( filePath.length() > 0 && fileName.length() > 0 ) 
		{
			// check filePath
			if ( filePath[filePath.length()-1] != '/' )
			{
				combinedFileName = filePath + '/';
			}
			else 
			{
				combinedFileName = filePath;
			}
        
			// check fileName
			if ( fileName[0] == '/' )
			{
                
                int overlapping = fileName.find(filePath);
                if ( overlapping != std::string::npos ) {
                    combinedFileName += fileName.substr(filePath.length()+1);
                    
                }else {
                    combinedFileName += fileName.substr(1);
                }
			}
			else
			{
				combinedFileName += fileName;
			}
		}
    
		return combinedFileName;
	}

	std::string FilepathUtils::uriDecode(const std::string & sSrc)
	{
		// Note from RFC1630: "Sequences which start with a percent
		// sign but are not followed by two hexadecimal characters
		// (0-9, A-F) are reserved for future extension"
    
		const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
		const int SRC_LEN = sSrc.length();
		const unsigned char * const SRC_END = pSrc + SRC_LEN;
		// last decodable '%'
		const unsigned char * const SRC_LAST_DEC = SRC_END - 2;
    
		char * const pStart = new char[SRC_LEN];
		char * pEnd = pStart;
    
		while (pSrc < SRC_LAST_DEC)
		{
			if (*pSrc == '%')
			{
				char dec1, dec2;
				if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
					&& -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
				{
					*pEnd++ = (dec1 << 4) + dec2;
					pSrc += 3;
					continue;
				}
			}
        
			*pEnd++ = *pSrc++;
		}
    
		// the last 2- chars
		while (pSrc < SRC_END)
			*pEnd++ = *pSrc++;
    
		std::string sResult(pStart, pEnd);
		delete [] pStart;
		return sResult;
	}
    
    void FilepathUtils::escapeFilePath(std::string& filePath)
    {
    #ifndef WIN32
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, "\\", "/");
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, " ", "\\ ");
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, "(", "\\(");
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, ")", "\\)");
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, "//", "/");
    #endif
    }
    
    void FilepathUtils::convertToUNIXFilePath(std::string &filePath)
    {
#ifndef WIN32
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, "\\", "/");
#endif
    }
    
    void FilepathUtils::replaceOccurencesOfStringWithString(std::string& input, const std::string& oldStr, const std::string& newStr)
    {

        size_t pos = 0;
        while((pos = input.find(oldStr, pos)) != std::string::npos)
        {
            input.replace(pos, oldStr.length(), newStr);
            pos += newStr.length();
        }
    }
}
