#ifndef DAEMATERIALENCODER_H_
#define DAEMATERIALENCODER_H_

#include "Base.h"

namespace gameplay
{
    class EncoderArguments;
    class Material;
    class Effect;
    
    class DAEMaterialEncoder
    {
    public:
        /**
         * Constructor.
         */
        DAEMaterialEncoder(void);
        
        /**
         * Destructor.
         */
        virtual ~DAEMaterialEncoder(void);

        /*
         * Creates all material objects
         *
         * @param dom - the DAE-Data Object Model
         */
        void processMaterial(domCOLLADA* dom);

        /*
         * Returns the Material with a given id
         *
         * @param The id of the wanted Material
         * @return The Material Object that is related to the param materialId, if it doen't exist NULL will be returned
         */
        Material* getMaterial(std::string materialId);

        /*
         * Writes the Material File
         */
        void writeMaterialFile();

    private:
        enum DAEMaterial {
            EMISSION,
            AMBIENT,
            DIFFUSE,
            SPECULAR
        };
        
        void processEffect(domEffect *effect, Material *material);
        void processProfileCOMMON(domProfile_COMMON *pc, Material *material);
        bool processColorOrTextureType(domCommon_color_or_texture_type *cot,
                                       DAEMaterial channel,
                                       Effect &effect,
                                       domCommon_float_or_param_type *fop = NULL);
        
        bool getFloat4Param(xsNCName Reference, domFloat4 &f4);
        bool getFloatParam(xsNCName Reference, domFloat &f) const;
        bool processTexture(domCommon_color_or_texture_type_complexType::domTexture *tex, Effect &effect);
		bool setTexturePaths(std::string path, std::string filepath, Effect& effect);

    private:
        domInstance_effectRef currentInstance_effect;
        domEffect *currentEffect;
        domCOLLADA* dom;
        
        std::list<Material*> materials;
    };
    
}

#endif
