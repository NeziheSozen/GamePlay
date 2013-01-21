#include "Base.h"
#include "SceneFile.h"
#include "EncoderArguments.h"

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
    std::string filename = EncoderArguments::getInstance()->getOutputFilePath();
    filename = filename.substr(filename.find_last_of('/') + 1, filename.length());

    std::string materialName = EncoderArguments::getInstance()->getMaterialOutputPath();
    materialName = materialName.substr(materialName.find_last_of('/') + 1, materialName.length());

    std::string fp = EncoderArguments::getInstance()->getFilePath();
    int pos = fp.find_last_of('/');
    fp = (pos == -1) ? fp : fp.substr(0, pos);

    fprintf(file, "scene\n");
    fprintf(file, "{\n");
    fprintf(file, "    path = %s\n\n", filename.c_str());
//    fprintf(file, "    path = %s%s\n\n", fp.c_str(), filename.c_str());

    std::list<Node*> nodes = _gpbFile.getNodeList();

    for (std::list<Node*>::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        Model* model = (*i)->getModel();
        if(model)
        {
            Mesh* mesh = model->getMesh();
            MeshSkin* skin = model->getSkin();
            if (skin)
            {
                mesh = skin->getMesh();
            }

            if (mesh)
            {
                fprintf(file, "    node %s\n", (*i)->getId().c_str());
                fprintf(file, "    {\n");
                
                // for each MeshPart
                int count = 0;
                for (std::vector<MeshPart*>::iterator i = mesh->parts.begin(); i != mesh->parts.end(); ++i)
                {
                    std::string symbolname = (*i)->getMaterialSymbolName();
                    Material* material = mesh->getMaterial(symbolname);
                    if(material == NULL) {
                        continue;
                    }

                    fprintf(file, "        material");
                    if (mesh->parts.size() > 1)
                    {
                        if(!material->getMaterialId().empty())
                        {
                            fprintf(file, "[%d] = ", count);
                        }
                    }
                    else
                    {
                        if(!material->getMaterialId().empty())
                        {
                            fprintf(file, " = ");
                        }
                    }

                    fprintf(file, "%s#%s\n", materialName.c_str(),material->getMaterialId().c_str());
//                    fprintf(file, "%s%s#%s\n", fp.c_str(), materialName.c_str(),material->getMaterialId().c_str());
                    count++;

//                    LOG(1,"symbol: %s material: %s\n", symbolname.c_str(), material.getMaterialId().c_str());
                }
                fprintf(file, "    }\n\n");
            }
        }
//        else
//        {
//            LOG(1,"no Material: %s\n", (*i)->getId().c_str());
//        }
    }
    
    fprintf(file, "}\n");
}

}
