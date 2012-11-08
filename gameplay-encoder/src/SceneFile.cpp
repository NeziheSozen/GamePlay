#include "Base.h"
#include "SceneFile.h"

namespace gameplay
{

SceneFile::SceneFile(GPBFile& gpbFile)
{
    _gpbFile = gpbFile;
}

SceneFile::~SceneFile(void)
{
}

void SceneFile::writeFile(FILE* file){
    fprintf(file, "scene\n");
    fprintf(file, "{\n");

    std::list<Node*> nodes = _gpbFile.getNodeList();
    
    for (std::list<Node*>::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        Model* model = (*i)->getModel();
        if(model)
        {
            Mesh* mesh = model->getMesh();
            if (mesh)
            {
                fprintf(file, "    node %s\n", (*i)->getId().c_str());
                fprintf(file, "    {\n");
                
                
                // for each MeshPart
                int count = 0;
                for (std::vector<MeshPart*>::iterator i = mesh->parts.begin(); i != mesh->parts.end(); ++i)
                {
                    std::string symbolname = (*i)->getMaterialSymbolName();
                    Material material;
                    mesh->getMaterial(material, symbolname);
                    fprintf(file, "        material");
                    if (mesh->parts.size() > 1)
                    {
                        fprintf(file, "[%d] = ", count);
                    }
                    else
                    {
                        fprintf(file, " = ");
                    }
                    
                    fprintf(file, "res/scene.material#%s\n", material.getMaterialId().c_str());
                    count++;
                    
                    
//                    LOG(1,"symbol: %s material: %s\n", symbolname.c_str(), material.getMaterialId().c_str());
                }
                fprintf(file, "    }\n");
            }
        }
        else
        {
            LOG(1,"no Material: %s\n", (*i)->getId().c_str());
        }
    }
    
    fprintf(file, "}\n");
}

}
