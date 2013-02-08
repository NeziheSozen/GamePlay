#include "Base.h"
#include "Effect.h"
#include "Light.h"
#include "Scene.h"
#include "Node.h"
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
    minFilter(NEAREST_MIPMAP_LINEAR),
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
//            // Light::DirectionalLight (already defined in shader)
//        }

//        // this will be set in the sceneviewer
//        fprintf(file, "\n\t\t\tu_ambientColor = %f, %f, %f, %f\n",
//                this->ambientColor.x,
//                this->ambientColor.y,
//                this->ambientColor.z,
//                this->ambientColor.w);
//

        // TEST
//        fprintf(file, "\n\t\t\tu_lightColor = %f, %f, %f, 1.0\n",
//                this->specularColor.x,
//                this->specularColor.y,
//                this->specularColor.z);

//        if (!light->isAmbient())
//        {
//            fprintf(file, "\t\t\tu_lightColor = %f, %f, %f\n",
//                    light->getRed(),
//                    light->getGreen(),
//                    light->getBlue());
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
        // TODO: set proper mipmap-values
        // fprintf(file, "\t\t\t\tminFilter = %s\n", filterStr[this->minFilter].c_str());
        // fprintf(file, "\t\t\t\tmagFilter = %s\n", filterStr[this->magFilter].c_str());
        fprintf(file, "\t\t\t\tminFilter = LINEAR\n");
        fprintf(file, "\t\t\t\tmagFilter = LINEAR\n");
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
    size_t index = (index1 != -1 && index1 > index2 ? index1 : index2);
	if (index == std::string::npos)
	{
		index = 0;
	}
    size_t length = path.length();
    this->texFilename = "tex/" + path.substr(index + 1);
    //this->texFilename = gpbOutputPath + "/tex/" + path.substr(path.find_last_of('/') + 1);
    this->hasTexture = true;
}

    
void Effect::setTextureSourcePath(std::string originalTexturePath, std::string originalModelPath)
{
    
    bool isRelativePath = false;
    
    int index = originalTexturePath.find("file://");
    if (index != std::string::npos)
    {
        originalTexturePath = originalTexturePath.substr(7);
    }
    if(originalTexturePath[0] == '.' && originalTexturePath[1] != '.')
    {
        originalTexturePath = this->combineFileNameWithFilePath(originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.') + 1) );
        isRelativePath = true;
    }
    else if(originalTexturePath[0] == '.' && originalTexturePath[1] == '.')
    {
        originalTexturePath = this->combineFileNameWithFilePath( originalModelPath, originalTexturePath.substr(originalTexturePath.find_first_of('.')) );
        isRelativePath = true;
    }
    
    if (!isRelativePath) {
        
        
        int lastSlashIndex = originalTexturePath.find_last_of("/");
        if (lastSlashIndex == std::string::npos) {
            
            originalTexturePath = this->combineFileNameWithFilePath(originalModelPath, originalTexturePath);
        }
    }
    
    this->texSourcePath = uriDecode(originalTexturePath);
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
		std::wstring sourceStr = std::wstring(this->texAbsPath.begin(), this->texAbsPath.end());
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
}
