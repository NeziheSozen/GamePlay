#ifndef MATERIALENHANCER_H_
#define MATERIALENHANCER_H_

#include "GPBFile.h"

namespace gameplay
{

class Node;
class Light;
class MaterialEnhancer
{
public:

    /**
     * Constructor.
     */
    MaterialEnhancer(void);

    /**
     * Destructor.
     */
    virtual ~MaterialEnhancer(void);

    void setLightInMaterial(GPBFile& gameplayFile);

private:
    Node* startingNode;

private:
    Light* getClosestLightFromSibling(Node* node);
    Light* getClosestLight(Node* node);
    Node* getFirstNode(Node* node);
};

}

#endif
