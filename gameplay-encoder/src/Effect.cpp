#include "Base.h"
#include "Effect.h"
#include "Light.h"
#include "Scene.h"
#include "Node.h"
#include "ImageUtils.h"
#include "FilepathUtils.h"

#ifdef WIN32
#include <iostream>
#include <Windows.h>
#else
#include <copyfile.h>
#include <errno.h>
#endif
namespace gameplay
{

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
    texDestinationPath(""),
    useSpecular(false),
    transparentTexture(false)
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

void Effect::writeEffect(FILE* file, Light* light, bool hasSkin, int numberOfJoints)
{
    std::vector<std::string> defines;
    
    
    copyTexture();
    std::string unlit = "";
    if(light == NULL)
    {
        unlit = "-unlit";
    }
    else
    {
        if (this->useSpecular) {
            defines.push_back("SPECULAR");
        }
        
        unsigned char lt = light->getLightType();
        if (lt == Light::PointLight)
        {
            defines.push_back("POINT_LIGHT");
        }
        else if (lt == Light::SpotLight)
        {
            defines.push_back("SPOT_LIGHT");
        }
        else
        {
            defines.push_back("DIRECTIONAL_LIGHT");
        }
        
        if (this->useSpecular) {
            fprintf(file, "\t\t\tu_specularExponent = %f\n", this->specularExponent);
        }
        // fprintf(file, "\t\t\tu_modulateAlpha = %f\n\n", this->alpha);
    }
    
    // TODO: skinning:
    // defines = SKINNING;SKINNING_JOINT_COUNT 3
    // u_matrixPalette = MATRIX_PALETTE
    if(hasSkin)
    {
        fprintf(file, "\t\t\tu_matrixPalette = MATRIX_PALETTE\n");
        defines.push_back("SKINNING");
        
        if(numberOfJoints != -1)
        {
            char buffer [50];
            sprintf (buffer, "SKINNING_JOINT_COUNT %d", numberOfJoints);
            defines.push_back(buffer);
        }
    }
    
    // write defines
    if(defines.size() > 0)
    {
        // write first item because there shouldnt be a ';' at the end
        fprintf(file, "\t\t\tdefines = ");
        std::string currentItem(defines.back());
        fprintf(file, currentItem.c_str());
        defines.pop_back();
        while (!defines.empty())
        {
            fprintf(file, ";");
            currentItem = defines.back();
            fprintf(file, currentItem.c_str());
            defines.pop_back();
        }
        fprintf(file, "\n");
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

    if(this->useSpecular) {
        fprintf(file, "\t\t\tu_specularColor = %f, %f, %f, %f\n",
                this->specularColor.x,
                this->specularColor.y,
                this->specularColor.z,
                this->specularColor.w);
    }

    fprintf(file, "\t\t\trenderState\n");
    fprintf(file, "\t\t\t{\n");
    fprintf(file, "\t\t\t\tcullFace = false\n");
    fprintf(file, "\t\t\t\tdepthTest = true\n");

    if (this->isTransparent()) {
        fprintf(file, "\t\t\t\tblend = true\n");
		fprintf(file, "\t\t\t\tsrcBlend = SRC_ALPHA\n");
        fprintf(file, "\t\t\t\tdstBlend = ONE_MINUS_SRC_ALPHA\n");
    }
    
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

    if (this->specularColor.x == 0 && this->specularColor.y == 0 && this->specularColor.z == 0) {
        // Special case for black specular color (as color is currently not supported)
        this->useSpecular = false;
    }
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

void Effect::setTextureFilename(std::string path, std::string gpbOutputPath, bool hasAlpha)
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
		index = -1; // index is set to -1 because the substring is taken from 0 then (index+1)
	}
    this->texFilename = "tex/" + path.substr(index + 1);
    this->transparentTexture = hasAlpha;
    //this->texFilename = gpbOutputPath + "/tex/" + path.substr(path.find_last_of('/') + 1);
    this->hasTexture = true;
}

void Effect::setTextureSourcePath(std::string path)
{
    this->texSourcePath = path;
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
    
    this->texDestinationPath = FilepathUtils::uriDecode(this->texDestinationPath);
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
        FilepathUtils::convertToUNIXFilePath(this->texSourcePath);
        FilepathUtils::convertToUNIXFilePath(this->texDestinationPath);
        
        int result = copyfile(this->texSourcePath.c_str(), this->texDestinationPath.c_str(), NULL, COPYFILE_DATA);

        if (result == -1) {
            GP_ERROR(ERR_TEX_COPY, this->texSourcePath.c_str(), this->texDestinationPath.c_str(), strerror(errno));
        }
#endif
    }
}

bool Effect::isPngFile() {
#ifndef WIN32
    std::string escapedFilePath(this->texSourcePath);
    FilepathUtils::convertToUNIXFilePath(escapedFilePath);
	return ImageUtils::isPngFile(escapedFilePath.c_str());
#endif
    
	return ImageUtils::isPngFile(this->texSourcePath.c_str());
}

int Effect::isPowerOfTwo(){
	return ImageUtils::isPowerOfTwo(this->texSourcePath.c_str());
}

void Effect::setUseSpecular(bool useSpecular) {
    this->useSpecular = useSpecular;
}

bool Effect::hasUseSpecular() {
    return this->useSpecular;
}

bool Effect::isTransparent()
{
    if (this->hasTexture) {
        return this->transparentTexture;
    } else {
        return this->diffuseColor.w < 1.0f;
    }

    return false;
}
}
