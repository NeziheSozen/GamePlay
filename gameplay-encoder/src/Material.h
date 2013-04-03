#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "Object.h"
#include "MaterialParameter.h"
#include "Effect.h"

namespace gameplay
{
class Light;
class Scene;
class Node;
class Material : public Object
{
public:

    /**
     * Constructor.
     */
    Material(void);

    /**
     * Destructor.
     */
    virtual ~Material(void);

    virtual unsigned int getTypeId(void) const;
    virtual const char* getElementName(void) const;
    virtual void writeBinary(FILE* file);
    virtual void writeText(FILE* file);
    Effect& getEffect() const;
    void setMaterialId(std::string materialId);
    std::string getMaterialId();
    void setLight(Light* light);
    Light* getLight();
    void setSkin(bool hasSkin);
    void setNumberOfJoints(int num);

private:
    std::list<MaterialParameter> _parameters;
    Effect* _effect;
    std::string materialId;
    Light* _light;
    bool hasSkin;
    int numberOfJoints;
};

}

#endif
