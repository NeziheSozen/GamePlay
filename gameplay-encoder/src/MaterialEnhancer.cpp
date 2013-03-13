#include "Base.h"
#include "MaterialEnhancer.h"
#include "Node.h"

using namespace std;

namespace gameplay
{

MaterialEnhancer::MaterialEnhancer(void) : startingNode(NULL)
{
}

MaterialEnhancer::~MaterialEnhancer(void)
{
}

void MaterialEnhancer::setLightInMaterial(GPBFile& gameplayFile)
{
    std::list<Node*> nodes = gameplayFile.getNodeList();
    Scene* scene = gameplayFile.getScene(); //(Scene*)gameplayFile.getFromRefTable("__SCENE__");
    
    std::list<Node*> sceneNodes = scene->getSceneNodes();
    std::list<Node*>::const_iterator sceneIt = sceneNodes.begin();
    
    while (sceneIt != sceneNodes.end())
    {
        Node* n1 = *sceneIt;
        sceneIt++;
        if(sceneIt != sceneNodes.end())
        {
            Node* n2 = *sceneIt;
            n1->setNextSibling(n2);
            n2->setPreviousSibling(n1);
        }
    }
    
    for (std::list<Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        Node* n = *it;
        Model* model = n->getModel();
        if (model)
        {
            Mesh* mesh = model->getMesh();
            MeshSkin* skin = model->getSkin();
            if (skin)
            {
                mesh = skin->getMesh();
            }

            if (mesh)
            {
                for (std::vector<MeshPart*>::iterator i = mesh->parts.begin(); i != mesh->parts.end(); ++i)
                {
                    std::string symbolname = (*i)->getMaterialSymbolName();
                    Material* material = mesh->getMaterial(symbolname);
                    if(material == NULL) {
                        continue;
                    }
                    Light* light = getClosestLight(n);
                    material->setLight(light);
                }
            }
        }
    }
}

Light* MaterialEnhancer::getClosestLightFromSibling(Node* node)
{
    Light* light = node->getLight();
    if (light)
    {
        return light;
    }
    else
    {
        Node* sibling = node->getNextSibling();
        if(sibling)
        {
            light = sibling->getLight();
            if (light) {
                return light;
            }
            else
            {
                return getClosestLightFromSibling(sibling);
            }
        }
        else
        {
            return NULL;
        }
    }
}

Light* MaterialEnhancer::getClosestLight(Node* node)
{
    Light* light = node->getLight();
    if (light)
    {
        return light;
    }
    else
    {
        Node* firstNode = getFirstNode(node);
        light = getClosestLightFromSibling(firstNode);
        if (light)
        {
            return light;
        }
        else
        {
            Node* parent = node->getParent();
            if(parent)
            {
                light = parent->getLight();
                if (light)
                {
                    return light;
                }
                else
                {
                    Node* firstParentNode = getFirstNode(parent);
                    light = getClosestLightFromSibling(firstParentNode);
                    if (light)
                    {
                        return light;
                    }
                    else
                    {
                        return getClosestLight(parent);
                    }
                }
            }
            else
            {
                return NULL;
            }
        }
    }
    return NULL;
}
    
Node* MaterialEnhancer::getFirstNode(Node* node)
{
    Node* firstNode = node;
    while (firstNode->getPreviousSibling()) {
        firstNode = firstNode->getPreviousSibling();
    }
    return firstNode;
}
}
