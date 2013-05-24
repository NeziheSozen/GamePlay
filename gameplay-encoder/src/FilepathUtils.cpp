#include "FilepathUtils.h"
#include "ImageUtils.h"
#include "EncoderArguments.h"

#include <fstream>

static void readStream(png_structp png, png_bytep data, png_size_t length)
{
    std::ifstream* stream = reinterpret_cast<std::ifstream *>(png_get_io_ptr(png));
    stream->read((char *) data, (int)length);
}

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
		index = originalTexturePath.find("file:"); // in case of: "file://User/username/texture.png"
		if (index != std::string::npos)
		{
			originalTexturePath = originalTexturePath.substr(5);
			// in case of: "file:/C:/User/username/texture.png" then we would have at this point following path: "/C:/User/username/texture.png"
			if(originalTexturePath.length() > 3 && originalTexturePath[2] == ':' && originalTexturePath[3] == '/')
			{
				originalTexturePath = originalTexturePath.substr(1);
			}
		}

		resultPath = originalTexturePath;
		if(originalTexturePath[0] == '.' && originalTexturePath[1] != '.') // e.g. "./texture.png"
		{
			resultPath = FilepathUtils::combineFileNameWithFilePath(originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.') + 1) );
			isRelativePath = true;
		}
		else if(originalTexturePath[0] == '.' && originalTexturePath[1] == '.')  // e.g. "../asdf/texture.png"
		{
			resultPath = FilepathUtils::combineFileNameWithFilePath( originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.')) );
			isRelativePath = true;
		}
		else if(originalTexturePath[0] == '/' && originalTexturePath.find_last_of('/') == 0) // e.g. "/texture.png"
		{
			resultPath = FilepathUtils::combineFileNameWithFilePath( originalModelPath, originalTexturePath );
			isRelativePath = true;
		}
		else if(originalTexturePath[0] != '.' && originalTexturePath[0] != '/') // e.g. "galaxy.fbm/body middle.tga"
		{
			int indexOfDoubleDot = originalTexturePath.find_first_of(':');
			if( indexOfDoubleDot == std::string::npos )
			{
				resultPath = FilepathUtils::combineFileNameWithFilePath( originalModelPath, originalTexturePath );
				isRelativePath = true;
			}
		}

		// handle absolute paths
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

		// check if resultPath exists
		std::string expectedFilePath = resultPath.substr(0, resultPath.find_last_of(".") + 1);
        expectedFilePath.append(fileExtension);
		FILE *fp = fopen(expectedFilePath.c_str(), "rb");
		if(fp == NULL)
		{
			GP_ERROR(ERR_FILE_NOT_FOUND, originalTexturePath.c_str());
			return "";
		}
		else
		{
			fclose(fp);
		}
    
		return resultPath;
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
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, "'", "\\'");
    #endif
    }
    
    void FilepathUtils::convertToUNIXFilePath(std::string &filePath)
    {
        FilepathUtils::replaceOccurencesOfStringWithString(filePath, "\\", "/");
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

	bool FilepathUtils::setTexturePaths(std::string texturePath, std::string modelFilepath, Effect& effect)
	{
		// paths that contain '\' will be converted to paths with only '/'
		FilepathUtils::convertToUNIXFilePath(texturePath);
		FilepathUtils::convertToUNIXFilePath(modelFilepath);

		// get the extension
		int index = texturePath.find_last_of('.') + 1;
		std::string ext = texturePath.substr(index);

		// convert texture path to a path with .png extension ignoring the original extension
		const std::string& texturePathAsPng(texturePath.substr(0, texturePath.find_last_of('.')) + ".png");
		bool isConverted2Png = false;

		// get the model path without the model-filename
		std::string modelPath(modelFilepath);
		int pos = modelPath.find_last_of('/');
		modelPath = (pos == -1) ? modelPath : modelPath.substr(0, pos);

		// absoluteTexturePathPng: the converted and final texture path as png will be stored here
		std::string absoluteTexturePathPng("");
		std::string currentTexturePath(FilepathUtils::getAbsoluteTextureSourcePath(texturePath, modelPath, ext));

		// handle non-png textures
		if(ext.compare("PNG") != 0 && ext.compare("png") != 0)
		{
			// store the absolute png path
			absoluteTexturePathPng = currentTexturePath.substr(0, currentTexturePath.find_last_of('.')) + ".png";
			if(absoluteTexturePathPng.compare("") != 0 && currentTexturePath.compare("") != 0)
			{
				// convert the texture to a png texture
				if(ImageUtils::convertTexture2Png(currentTexturePath, absoluteTexturePathPng) != 0)
				{
					GP_ERROR(ERR_CONVERT_TO_PNG, texturePath.c_str(), texturePathAsPng.c_str());
					return false;
				}
				isConverted2Png = true;
				GP_INFO(INFO_TEXTURE_CONVERTED2PNG, texturePath.c_str(), texturePathAsPng.c_str())
			}
		}
		else
		{
			// store the absolute png path
			absoluteTexturePathPng = currentTexturePath;
		}

        bool validPngFile;
        bool hasTransparency = hasTransparentPixels(absoluteTexturePathPng, validPngFile);

        // sanity check - is the png file really a png file?
		if(!validPngFile)
		{
			GP_ERROR(ERR_CORRUPTED_PNG, texturePath.c_str());
			return false;
		}

        // set the new absolute texture path (png) in the effect-object
		effect.setTextureFilename(absoluteTexturePathPng, modelPath, hasTransparency);
		effect.setTextureSourcePath(absoluteTexturePathPng);

		if (EncoderArguments::getInstance()->textureOutputEnabled())
		{
			effect.setTexDestinationPath(EncoderArguments::getInstance()->getTextureOutputPath());
		}
		return true;
	}

    bool FilepathUtils::hasTransparentPixels(const std::string& pathToPng, bool& valid) {
        valid = false; // let's be pesimistic

		std::ifstream pngFile(pathToPng.c_str(), std::ios::in | std::ios::binary);
        if (!pngFile.good()) { // can't read file
            return false;
        }

        // Initialize png read struct (last three parameters use stderr+longjump if NULL).
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png == NULL) // cannot create struct
        {
            return false;
        }

        // Initialize info struct.
        png_infop info = png_create_info_struct(png);
        if (info == NULL) // cannot create info structure
        {
            png_destroy_read_struct(&png, NULL, NULL);
            return false;
        }

        // Set up error handling (required without using custom error handlers above).
        if (setjmp(png_jmpbuf(png))) // cannot set error handler
        {
            png_destroy_read_struct(&png, &info, NULL);
            return false;
        }
        
        // Initialize file io.
        png_set_read_fn(png, (png_voidp)&pngFile, readStream);

        png_read_info(png,info);

        png_byte colorType = png_get_color_type(png, info);
        png_uint_32 bitdepth   = png_get_bit_depth(png, info);
        
        if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png);
            if (bitdepth != 8) {
                png_set_expand_gray_1_2_4_to_8(png);
                bitdepth = 8;
            }
        }

        if (bitdepth > 8) {
            png_set_strip_16(png); // transform 16bit per channel -> 8bit
        }

        if (bitdepth < 8) {
            png_set_packing(png); // transform 1,2,4 bit per channel -> 8bit
        }

        if (colorType != PNG_COLOR_TYPE_RGBA) {
            png_set_expand(png); // combine palette, trns -> RGB / RGBA
            png_set_filler(png, 0xff, PNG_FILLER_AFTER); // add alpha channel if missing
        }

        png_read_update_info(png, info);
        size_t stride = png_get_rowbytes(png, info);

        // Allocate image data.
        png_uint_32 height = png_get_image_height(png, info);
        png_uint_32 width = png_get_image_width(png, info);
        unsigned char* bufptr = new unsigned char[height * stride];

        png_bytep *row_pointers = new png_bytep[height * sizeof(png_bytep)];

        for (int i = 0; i < height; i++) {
            row_pointers[i] = bufptr + i * stride;
        }
        png_read_image(png, row_pointers);

        // Clean up.
        png_read_end(png,0);
        delete[] row_pointers;
        png_destroy_read_struct(&png, &info, NULL);

        valid = true;

        // for checking ignore a 1 pixel border
        for (int i = 1; i < height-1; ++i) {
            for (int j = 1; j < width-1; ++j) {
                unsigned char alpha = bufptr[(i*width + j)*4 + 3];
                if (alpha != 255) {
                    delete[] bufptr;
                    return true;
                }
            }
        }

        delete[] bufptr;
        return false;
    }
}
