#include "Base.h"
#include "Effect.h"
#include "Light.h"
#include "Scene.h"
#include "Node.h"
#include <png.h>

#ifdef WIN32
#include <iostream>
#include <Windows.h>
#else
#include <copyfile.h>
#endif
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

std::string Effect::wrapStr[] = { "REPEAT", "CLAMP" };
std::string Effect::filterStr[] = {
    "NEAREST",
    "LINEAR",
    "NEAREST_MIPMAP_NEAREST",
    "LINEAR_MIPMAP_NEAREST",
    "NEAREST_MIPMAP_LINEAR",
    "LINEAR_MIPMAP_LINEAR"
};

Effect::Effect(void) :
    ambientColor(Vector4(.2f, .2f, .2f, 1.0f)),
    diffuseColor(Vector4(.8f, .8f, .8f, 1.0f)),
    specularColor(Vector4(.0f, .0f, .0f, .0f)),
    specularExponent(.0f),
    wrapS(REPEAT),
    wrapT(REPEAT),
    minFilter(LINEAR),
    magFilter(LINEAR),
    texFilename(""),
    hasTexture(false),
    alpha(1.0f),
    texSourcePath(""),
    texDestinationPath("")
{

}

Effect::~Effect(void)
{
}

unsigned int Effect::getTypeId(void) const
{
    return EFFECT_ID;
}

const char* Effect::getElementName(void) const
{
    return "Effect";
}

void Effect::writeBinary(FILE* file)
{
    Object::writeBinary(file);
    write(_vertexShader, file);
    write(_fragmentShader, file);
}

void Effect::writeText(FILE* file)
{
}

void Effect::writeEffect(FILE* file, Light* light)
{
    copyTexture();
    std::string unlit = "";
    if(light == NULL)
    {
        unlit = "-unlit";
    }
    else
    {
        fprintf(file, "\t\t\tdefines = SPECULAR");
        unsigned char lt = light->getLightType();
        if (lt == Light::PointLight)
        {
            fprintf(file, ";POINT_LIGHT");
        }
        else if (lt == Light::SpotLight)
        {
            fprintf(file, ";SPOT_LIGHT");
        }
//        else
//        {
//            // Light::DirectionalLight (already defined as default in shader)
//        }

        fprintf(file, "\n\t\t\tu_specularExponent = %f\n", this->specularExponent);
        // fprintf(file, "\t\t\tu_modulateAlpha = %f\n\n", this->alpha);
    }

    if(this->hasTexture)
    {
        fprintf(file, "\t\t\tvertexShader = shaders/textured%s.vert\n", unlit.c_str());
        fprintf(file, "\t\t\tfragmentShader = shaders/textured%s.frag\n\n", unlit.c_str());
        fprintf(file, "\t\t\tsampler u_diffuseTexture\n");
        fprintf(file, "\t\t\t{\n");
        // e.g. path = duck-diffuse.png
        fprintf(file, "\t\t\t\tpath = %s\n", this->texFilename.c_str());
        fprintf(file, "\t\t\t\twrapS = %s\n", wrapStr[this->wrapS].c_str());
        fprintf(file, "\t\t\t\twrapT = %s\n", wrapStr[this->wrapT].c_str());
        fprintf(file, "\t\t\t\tminFilter = %s\n", filterStr[this->minFilter].c_str());
        fprintf(file, "\t\t\t\tmagFilter = %s\n", filterStr[this->magFilter].c_str());
        // TODO: support mipmapping
        fprintf(file, "\t\t\t\tmipmap = false\n");
        fprintf(file, "\t\t\t}\n\n");
    }
    else
    {
        fprintf(file, "\t\t\tvertexShader = shaders/colored%s.vert\n", unlit.c_str());
        fprintf(file, "\t\t\tfragmentShader = shaders/colored%s.frag\n\n", unlit.c_str());
            fprintf(file, "\t\t\tu_diffuseColor = %f, %f, %f, %f\n",
            this->diffuseColor.x,
            this->diffuseColor.y,
            this->diffuseColor.z,
            this->diffuseColor.w);
    }
    fprintf(file, "\t\t\trenderState\n");
    fprintf(file, "\t\t\t{\n");
    fprintf(file, "\t\t\t\tcullFace = false\n");
    fprintf(file, "\t\t\t\tdepthTest = true\n");
    fprintf(file, "\t\t\t}\n");
}

void Effect::setAmbient(Vector4 color)
{
    this->ambientColor = color;
}

void Effect::setDiffuse(Vector4 color)
{
    this->diffuseColor = color;
}

void Effect::setSpecular(Vector4 color)
{
    this->specularColor = color;
}

void Effect::setShininess(float shininess)
{
    this->specularExponent = shininess;
}

void Effect::setWrapS(Wrap wrapS)
{
    this->wrapS = wrapS;
}

void Effect::setWrapT(Wrap wrapT)
{
    this->wrapT = wrapT;
}

void Effect::setMinFilter(Filter minFilter)
{
    this->minFilter = minFilter;
}

void Effect::setMagFilter(Filter magFilter)
{
    this->magFilter = magFilter;
}

void Effect::setTextureFilename(std::string path, std::string gpbOutputPath)
{
    size_t index1 = path.find_last_of('\\');
    size_t index2 = path.find_last_of('/');
	size_t length = path.length();
	size_t index;

	if(index1 != -1 && index1 != std::string::npos)
	{
		if(index2 < length && index1 > index2 || index2 >= length)
		{
			index = index1;
		}
		else
		{
			index = index2;
		}
	}
	else
	{
		if(index1 < length && index2 > index1 || index1 >= length)
		{
			index = index2;
		}
		else
		{
			index = index1;
		}
	}

	if (index == std::string::npos || index >= length)
	{
		index = 0;
	}
    this->texFilename = "tex/" + path.substr(index + 1);
    //this->texFilename = gpbOutputPath + "/tex/" + path.substr(path.find_last_of('/') + 1);
    this->hasTexture = true;
}

void Effect::setTextureSourcePath(std::string originalTexturePath, std::string originalModelPath)
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
        resultPath = this->combineFileNameWithFilePath(originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.') + 1) );
        isRelativePath = true;
    }
    else if(originalTexturePath[0] == '.' && originalTexturePath[1] == '.')
    {
        resultPath = this->combineFileNameWithFilePath( originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.')) );
        isRelativePath = true;
    }
    
	
    if (!isRelativePath) 
	{
        int lastSlashIndex = originalTexturePath.find_last_of("/");
        if (lastSlashIndex == std::string::npos) 
		{    
            resultPath = this->combineFileNameWithFilePath(originalModelPath, originalTexturePath);
        }
    }

	resultPath = uriDecode(resultPath);

	// check if resultPath exists
	FILE *fp = fopen(resultPath.c_str(), "rb");
	if(fp == 0)
	{
		// this happens when we get a path like this: "/texture.png"
		resultPath = this->combineFileNameWithFilePath(originalModelPath, originalTexturePath.substr(0));
	}
	else
	{
		fclose(fp);
	}
	
    this->texSourcePath = uriDecode(resultPath);
}
    


std::string Effect::combineFileNameWithFilePath(const std::string& filePath, const std::string& fileName)
{
    std::string combinedFileName = "";
    
    if ( filePath.length() > 0 && fileName.length() > 0 ) {
    
        // check filePath
        if ( filePath[filePath.length()-1] != '/' ) {
            
            combinedFileName = filePath + '/';
        }else {
            combinedFileName = filePath;
        }
        
        
        // check fileName
        if ( fileName[0] == '/' ) {
            combinedFileName += fileName.substr(1);
        }else
        {
            combinedFileName += fileName;
        }
    }
    
    return combinedFileName;
}
    

std::string Effect::getTextureSourcePath()
{
    return this->texSourcePath;
}

void Effect::setAlpha(float alpha)
{
    this->alpha = alpha;
}

void Effect::setTexDestinationPath(std::string texDestinationPath)
{
	size_t index1 = this->texSourcePath.find_last_of('\\');
    size_t index2 = this->texSourcePath.find_last_of('/');
    size_t pos = (index1 != -1 && index1 > index2 ? index1 : index2);
    if(pos != std::string::npos) {
        this->texDestinationPath = texDestinationPath + this->texSourcePath.substr(pos);
    }
    else {
        this->texDestinationPath = texDestinationPath + this->texSourcePath;
    }
    
    // texDestinationPath
    // check if path ends with '/'
    
    this->texDestinationPath = uriDecode(this->texDestinationPath);
}

void Effect::copyTexture()
{
    if (!this->texDestinationPath.empty() && this->texSourcePath.compare(this->texDestinationPath) != 0)
    {
#ifdef WIN32
		std::wstring sourceStr = std::wstring(this->texSourcePath.begin(), this->texSourcePath.end());
		LPCWSTR source = sourceStr.c_str();
		std::wstring destStr = std::wstring(this->texDestinationPath.begin(), this->texDestinationPath.end());
		LPCWSTR dest = destStr.c_str();
		BOOL b = CopyFile(source, dest,0);
#else
        int result = copyfile(this->texSourcePath.c_str(), this->texDestinationPath.c_str(), NULL, COPYFILE_DATA);

        if (result == -1) {
            GP_ERROR(ERR_TEX_COPY, this->texSourcePath.c_str(), this->texDestinationPath.c_str())
        }
#endif
    }
}

std::string Effect::uriDecode(const std::string & sSrc)
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

int Effect::isPowerOfTwo()
{
    png_byte header[8];
    
    FILE *fp = fopen(this->texSourcePath.c_str(), "rb");
    if (fp == 0)
    {
        return 0;
    }
    
    // read the header
    fread(header, 1, 8, fp);
    
    if (png_sig_cmp(header, 0, 8))
    {
        fclose(fp);
        return 0;
    }
    
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        fclose(fp);
        return 0;
    }
    
    // create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        fclose(fp);
        return 0;
    }
    
    // create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fclose(fp);
        return 0;
    }
    
    // the code in this if statement gets called if libpng encounters an error
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return 0;
    }
    
    // init png reading
    png_init_io(png_ptr, fp);
    
    // let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);
    
    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);
    
    // variables to pass to get info
    int bit_depth, color_type;
    png_uint_32 temp_width, temp_height;
    
    // get info about png
    png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type,
                 NULL, NULL, NULL);
    
    return (isPowerOfTwo(temp_width) && isPowerOfTwo(temp_height));
}

int Effect::isPowerOfTwo(unsigned int x){
    return ((x != 0) && !(x & (x - 1)));
}

bool Effect::isPngFile() {
    FILE *fp = fopen(this->texSourcePath.c_str(), "rb");
    if (!fp)
    {
		GP_WARNING(WARN_TEXTURE_NOT_FOUND, this->texSourcePath.c_str());
        return false;
    }
    
    int number = 7;
    Byte* header = (Byte*) malloc(sizeof(Byte)*number);
    
    fread(header, 1, number, fp);
    bool isPng = !png_sig_cmp(header, 0, number);
    fclose(fp);
    return isPng;
}
}
