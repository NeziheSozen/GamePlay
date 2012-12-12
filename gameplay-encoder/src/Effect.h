#ifndef EFFECT_H_
#define EFFECT_H_

#include "Object.h"

namespace gameplay
{
class Light;
class Scene;
class Node;
class Effect : public Object
{
public:
    /**
     * Defines the set of supported texture wrapping modes.
     */
    enum Wrap
    {
        REPEAT = 0,
        CLAMP
    };
    /**
     * Defines the set of supported texture filters.
     */
    enum Filter
    {
        NEAREST = 0,
        LINEAR,
        NEAREST_MIPMAP_NEAREST,
        LINEAR_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_LINEAR
    };
public:
    /**
     * Constructor.
     */
    Effect(void);

    /**
     * Destructor.
     */
    virtual ~Effect(void);

    virtual unsigned int getTypeId(void) const;

    virtual const char* getElementName(void) const;

    virtual void writeBinary(FILE* file);
    virtual void writeText(FILE* file);

    void writeEffect(FILE* file, Light* light = NULL);

    void setAmbient(Vector4 color);
    void setDiffuse(Vector4 color);
    void setSpecular(Vector4 color);
    void setShininess(float shininess); // u_specularExponent
    void setWrapS(Wrap wrapS);
    void setWrapT(Wrap wrapT);
    void setMinFilter(Filter minFilter);
    void setMagFilter(Filter magFilter);
    void setTextureFilename(std::string path);
    void setAlpha(float alpha);

    void setTextureFilePath(std::string path, std::string gpbOutputPath);
    std::string getTextureFilePath();
    void setTexDestinationPath(std::string texDestinationPath);

public:
    static const std::string wrapStr[2];
    static const std::string filterStr[6];

private:
    void copyTexture();

private:
    Vector4 ambientColor;
    Vector4 diffuseColor;
    Vector4 specularColor;
    float specularExponent;
    Wrap wrapS;
    Wrap wrapT;
    Filter minFilter;
    Filter magFilter;
    std::string texFilename;
    bool hasTexture;
    float alpha;
    std::string _vertexShader;
    std::string _fragmentShader;
    std::string texAbsPath;
    std::string texDestinationPath;
};

}

#endif
