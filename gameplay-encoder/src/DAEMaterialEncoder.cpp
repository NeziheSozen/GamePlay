#include "Base.h"
#include "DAEMaterialEncoder.h"
#include "EncoderArguments.h"
#include "Material.h"
#include "Effect.h"
#include <dom/domInstance_effect.h>
#include <dom/domCommon_color_or_texture_type.h>
#include <iostream>
#include <png.h>
#include "FilepathUtils.h"
#include "ImageUtils.h"

using namespace std;

namespace gameplay
{
    
    DAEMaterialEncoder::DAEMaterialEncoder()
    {
    }
    
    DAEMaterialEncoder::~DAEMaterialEncoder()
    {
    }

    Material* DAEMaterialEncoder::getMaterial(std::string materialId)
    {
        // TODO: optimize search because complexity O(n) is too high
        std::list<Material*>::iterator it;
        for (it = materials.begin(); it != materials.end(); ++it)
        {
            if((*it)->getMaterialId().compare(materialId) == 0)
            {
                return (*it);
            }
        }
        return NULL;
    }

    void DAEMaterialEncoder::processMaterial(domCOLLADA* dom)
    {
        this->dom = dom;
        // all about the material:
        domLibrary_materials_Array &materialLibs = dom->getLibrary_materials_array();
        size_t materialLibsCount = materialLibs.getCount();
        for (size_t i = 0; i < materialLibsCount; i++)
        {
            const domLibrary_materialsRef& libMaterial = materialLibs.get(i);
            const domMaterial_Array& materialArray = libMaterial->getMaterial_array();
            size_t materialCount = materialArray.getCount();
            
            for (size_t j = 0; j < materialCount; j++)
            {
                domMaterial *mat = daeSafeCast< domMaterial >(materialArray.get(j));
                if (mat)
                {
                    currentInstance_effect = mat->getInstance_effect();
                    domEffect *effect = daeSafeCast< domEffect >( currentInstance_effect->getUrl().getElement() );
                    if (effect)
                    {
                        Material *material = new Material();
                        material->setMaterialId(mat->getId());
                        materials.push_front(material);
                        processEffect(effect, material);
                    }
                }
            }
        }
    }

    void DAEMaterialEncoder::writeMaterialFile()
    {
        // get filepath:
        std::string filepath = EncoderArguments::getInstance()->getMaterialOutputPath();
        FILE* _file = fopen(filepath.c_str(), "w");
        if (!_file)
        {
            return;
        }
        
        std::list<Material*>::iterator it;
        for (it = materials.begin(); it != materials.end(); ++it)
        {
            (*it)->writeText(_file);
        }
    }
    
    void DAEMaterialEncoder::processEffect(domEffect *effect, Material *material)
    {
        // check if there is light
//        domLibrary_lights_Array &lightsLib = this->dom->getLibrary_lights_array();
//        for (size_t i = 0; i < lightsLib.getCount(); i++)
//        {
//            const domLibrary_lightsRef& libLights = lightsLib.get(i);
//            const domLight_Array& lightArray = libLights->getLight_array();
////            for (size_t j = 0; j < lightArray.getCount(); j++)
////            {
////                const domLightRef& light = lightArray.get(j);
////                const domTechniqueRef& tecCommon = light->getTechnique_common();
////                if(tecCommon)
////                {
////                    //tecCommon->get
////                    domLight
////                    light->getTechnique_common()->get
////                }
////            }
//        }
        
        // efects
        for ( size_t i = 0; i < effect->getFx_profile_abstract_array().getCount(); i++ )
        {
            domProfile_COMMON *pc = daeSafeCast< domProfile_COMMON >( effect->getFx_profile_abstract_array()[i] );
            if (pc)
            {
                currentEffect = effect;
                processProfileCOMMON(pc, material);
            }
        }
    }
    
    void DAEMaterialEncoder::processProfileCOMMON(domProfile_COMMON *pc, Material *material)
    {
        domProfile_COMMON::domTechnique *teq = pc->getTechnique();
        domProfile_COMMON::domTechnique::domBlinn *blinn = teq->getBlinn();
        domProfile_COMMON::domTechnique::domConstant *constant = teq->getConstant();
        domProfile_COMMON::domTechnique::domLambert *lambert = teq->getLambert();
        domProfile_COMMON::domTechnique::domPhong *phong = teq->getPhong();
        
		if(blinn)
        {
            material->getEffect().setUseSpecular(true);
            if(!processColorOrTextureType(blinn->getEmission(), EMISSION, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "emission");
			}

            if(!processColorOrTextureType(blinn->getAmbient(), AMBIENT, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "ambient");
			}

            if(!processColorOrTextureType(blinn->getDiffuse(), DIFFUSE, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "diffuse");
			}

            processColorOrTextureType(blinn->getSpecular(), SPECULAR, material->getEffect(), blinn->getShininess());
            
            if (blinn->getTransparency()) {
                domFloat f;
                if(blinn->getTransparency()->getFloat()){
                    material->getEffect().setAlpha(blinn->getTransparency()->getFloat()->getValue());
                }else if(getFloatParam(blinn->getTransparency()->getParam()->getRef(), f))
                {
                    material->getEffect().setAlpha(f);
                }
            }
            
            /*
             mapping doesnt exist in gameplay for:
             - getReflective()
             - getReflectivity()
             - getTransparent()
             - getIndex_of_refraction()
             */
        }
        else if(constant)
        {
           if(!processColorOrTextureType(constant->getEmission(), EMISSION, material->getEffect()))
		   {
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "emission");
		   }
           if (constant->getTransparency()) {
                domFloat f;
                if(constant->getTransparency()->getFloat()){
                    material->getEffect().setAlpha(constant->getTransparency()->getFloat()->getValue());
                }else if(getFloatParam(constant->getTransparency()->getParam()->getRef(), f))
                {
                    material->getEffect().setAlpha(f);
                }
            }
            
            /*
             mapping doesnt exist in gameplay for:
             - getReflective()
             - getReflectivity()
             - getTransparent()
             - getIndex_of_refraction()
             */
        }
        else if(lambert)
        {
            material->getEffect().setUseSpecular(false);
            if(!processColorOrTextureType(lambert->getEmission(), EMISSION, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "emission");
			}
            if(!processColorOrTextureType(lambert->getAmbient(), AMBIENT, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "ambient");
			}
            if(!processColorOrTextureType(lambert->getDiffuse(), DIFFUSE, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "diffuse");
			}
            
            if (lambert->getTransparency()) {
                domFloat f;
                if(lambert->getTransparency()->getFloat()){
                    material->getEffect().setAlpha(lambert->getTransparency()->getFloat()->getValue());
                }else if(getFloatParam(lambert->getTransparency()->getParam()->getRef(), f))
                {
                    material->getEffect().setAlpha(f);
                }
            }
            
            /*
             mapping doesnt exist in gameplay for:
             - getReflective()
             - getReflectivity()
             - getTransparent()
             - getIndex_of_refraction()
             */
        }
        else if(phong)
        {
            material->getEffect().setUseSpecular(true);
            if(!processColorOrTextureType(phong->getEmission(), EMISSION, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "emission");
			}
            if(!processColorOrTextureType(phong->getAmbient(), AMBIENT, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "ambient");
			}
            if(!processColorOrTextureType(phong->getDiffuse(), DIFFUSE, material->getEffect()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "diffuse");
			}
            if(!processColorOrTextureType(phong->getSpecular(), SPECULAR, material->getEffect(), phong->getShininess()))
			{
				GP_INFO(INFO_PROP_NOT_SET_IN_MATERIAL, "specular");
			}
            
            if (phong->getTransparency()) {
                domFloat f;
                if(phong->getTransparency()->getFloat()){
                    material->getEffect().setAlpha(phong->getTransparency()->getFloat()->getValue());
                }else if(getFloatParam(phong->getTransparency()->getParam()->getRef(), f))
                {
                    material->getEffect().setAlpha(f);
                }
            }
            
            /*
             mapping doesnt exist in gameplay for:
             - getReflective()
             - getReflectivity()
             - getTransparent()
             - getIndex_of_refraction()
             */
        }
    }
    
    
    bool DAEMaterialEncoder::processColorOrTextureType(domCommon_color_or_texture_type *cot,
                                                       DAEMaterial channel,
                                                       Effect &effect,
                                                       domCommon_float_or_param_type *fop)
    {
        if (!cot) {
            return false;
        }
        
        bool retVal = false;
        
        if (channel == EMISSION)
        {
            // TODO: EMISSION doesn't exist in Gameplay3d so far
        }
        else if ( channel == AMBIENT )
        {
            if (cot->getColor())
            {
                domFloat4 &f4 = cot->getColor()->getValue();
                effect.setAmbient(Vector4(f4[0], f4[1], f4[2], f4[3]));
                retVal = true;
            }
            else if (cot->getParam())
            {
                domFloat4 f4;
                if (getFloat4Param(cot->getParam()->getRef(), f4))
                {
                    effect.setAmbient(Vector4(f4[0], f4[1], f4[2], f4[3]));
                    retVal = true;
                }
            }
            else
            {
//                LOG(1, "Currently no support for <texture> in Ambient channel");
                GP_ERROR(ERR_NO_TEXTURE_SUPPORT_IN_MATERIAL, "Ambient");
            }
        }
        else if ( channel == DIFFUSE )
        {
            if (cot->getColor())
            {
                domFloat4 &f4 = cot->getColor()->getValue();
                effect.setDiffuse(Vector4(f4[0], f4[1], f4[2], f4[3]));
                retVal = true;
            }
            else if (cot->getParam())
            {
                domFloat4 f4;
                if (getFloat4Param(cot->getParam()->getRef(), f4))
                {
                    effect.setDiffuse(Vector4(f4[0], f4[1], f4[2], f4[3]));
                    retVal = true;
                }
            }
            else if (cot->getTexture())
            {
                retVal = processTexture( cot->getTexture(), effect);
                domExtra *extra = cot->getTexture()->getExtra();
                if (extra && extra->getType() && strcmp(extra->getType(), "color") == 0 )
                {
                    //the extra data for osg. Diffuse color can happen with a texture.
                    for ( unsigned int i = 0; i < extra->getTechnique_array().getCount(); i++ )
                    {
                        domTechnique *teq = extra->getTechnique_array()[i];
                        if ( strcmp( teq->getProfile(), "SCEI" ) == 0 )
                        {
                            Vector4 col;
                            domAny *dcol = (domAny*)(daeElement*)teq->getContents()[0];
                            std::istringstream diffuse_colour((const char *)dcol->getValue());
                            diffuse_colour >> col.x >> col.y >> col.z >> col.w;
                            effect.setDiffuse(col);
                            retVal = true;
                            break;
                        }
                    }
                }
            }
        }
        else if ( channel == SPECULAR )
        {
            if (cot->getColor())
            {
                domFloat4 &f4 = cot->getColor()->getValue();
                effect.setSpecular(Vector4(f4[0], f4[1], f4[2], f4[3]));
                retVal = true;
            }
            else if (cot->getParam())
            {
                domFloat4 f4;
                if (getFloat4Param(cot->getParam()->getRef(), f4))
                {
                    effect.setSpecular(Vector4(f4[0], f4[1], f4[2], f4[3]));
                    retVal = true;
                }
            }
            else
            {
//                LOG(1, "Currently no support for <texture> in Specular channel ");
                GP_ERROR(ERR_NO_TEXTURE_SUPPORT_IN_MATERIAL, "Specular");
                
            }
            if (fop && fop->getFloat())
            {
                float shininess = fop->getFloat()->getValue();
                if(shininess < 1.0)
                {
                    shininess *= 128.0;
                }
                else if(shininess > 128.0)
                {
                    shininess = 128.0;
                }
                effect.setShininess(shininess);
                retVal = true;
            }
        }
        
        return retVal;
    }
    
    bool DAEMaterialEncoder::getFloat4Param(xsNCName Reference, domFloat4 &f4)
    {
        std::string MyReference = Reference;
        
        MyReference.insert(0, "./");
        daeSIDResolver Resolver(currentEffect, MyReference.c_str());
        daeElement *el = Resolver.getElement();
        if (NULL == el)
            return false;
        
        if (NULL != currentInstance_effect)
        {
            // look here first for setparams
            // I am sure there must be a better way of doing this
            // Maybe the Collada DAE guys can give us a parameter management mechanism !
            const domInstance_effect::domSetparam_Array& SetParamArray = currentInstance_effect->getSetparam_array();
            size_t NumberOfSetParams = SetParamArray.getCount();
            for (size_t i = 0; i < NumberOfSetParams; i++)
            {
                // Just do a simple comaprison of the ref strings for the time being
                if (0 == strcmp(SetParamArray[i]->getRef(), Reference))
                {
                    if (NULL != SetParamArray[i]->getFx_basic_type_common() && (NULL != SetParamArray[i]->getFx_basic_type_common()->getFloat4()))
                    {
                        f4 = SetParamArray[i]->getFx_basic_type_common()->getFloat4()->getValue();
                        return true;
                    }
                }
            }
        }
        
        domCommon_newparam_type *cnp = daeSafeCast< domCommon_newparam_type >( el );
        domFx_newparam_common *npc = daeSafeCast< domFx_newparam_common >( el );
        if ((cnp != NULL) && (NULL != cnp->getFloat4()))
        {
            f4 = cnp->getFloat4()->getValue();
            return true;
        }
        else if ((npc != NULL) && (NULL != npc->getFx_basic_type_common()) && (NULL != npc->getFx_basic_type_common()->getFloat4()))
        {
            f4 = npc->getFx_basic_type_common()->getFloat4()->getValue();
            return true;
        }
        else
            return false;
    }
    
    bool DAEMaterialEncoder::getFloatParam(xsNCName Reference, domFloat &f) const
    {
        std::string MyReference = Reference;

        MyReference.insert(0, "./");
        daeSIDResolver Resolver(currentEffect, MyReference.c_str());
        daeElement *el = Resolver.getElement();
        if (!el)
        {
            return false;
        }
        
        if (currentInstance_effect)
        {
            // look here first for setparams
            // I am sure there must be a better way of doing this
            // Maybe the Collada DAE guys can give us a parameter management mechanism !
            const domInstance_effect::domSetparam_Array& SetParamArray = currentInstance_effect->getSetparam_array();
            size_t NumberOfSetParams = SetParamArray.getCount();
            for (size_t i = 0; i < NumberOfSetParams; i++)
            {
                // Just do a simple comaprison of the ref strings for the time being
                if (0 == strcmp(SetParamArray[i]->getRef(), Reference))
                {
                    if (NULL != SetParamArray[i]->getFx_basic_type_common() && (NULL != SetParamArray[i]->getFx_basic_type_common()->getFloat()))
                    {
                        f = SetParamArray[i]->getFx_basic_type_common()->getFloat()->getValue();
                        return true;
                    }
                }
            }
        }
        
        domCommon_newparam_type *cnp = daeSafeCast< domCommon_newparam_type >( el );
        domFx_newparam_common *npc = daeSafeCast< domFx_newparam_common >( el );
        if ((cnp != NULL) && (NULL != cnp->getFloat()))
        {
            f = cnp->getFloat()->getValue();
            return true;
        }
        else if ((npc != NULL) && (NULL != npc->getFx_basic_type_common()) && (NULL != npc->getFx_basic_type_common()->getFloat()))
        {
            f = npc->getFx_basic_type_common()->getFloat()->getValue();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool DAEMaterialEncoder::processTexture(domCommon_color_or_texture_type_complexType::domTexture *tex, Effect &effect)
    {
        //find the newparam for the sampler based on the texture attribute
        domFx_sampler2D_common *sampler = NULL;
        domFx_surface_common *surface = NULL;
        domImage *dImg = NULL;
        
        std::string target = std::string("./") + std::string(tex->getTexture());
        
        daeSIDResolver res1( currentEffect, target.c_str() );
        daeElement *el = res1.getElement();
        
        bool isTexFilenameSet = false;
        
        if (el)
        {
            domCommon_newparam_type *cnp = daeSafeCast< domCommon_newparam_type >( el );
            domFx_newparam_common *npc = daeSafeCast< domFx_newparam_common >( el );
            
            if (cnp)
            {
                sampler = cnp->getSampler2D();
            }
            else if (npc)
            {
                sampler = npc->getFx_basic_type_common()->getSampler2D();
            }
            
            if (!sampler)
            {
                return false;
            }
            
            //find the newparam for the surface based on the sampler2D->source value
            target = std::string("./") + std::string( sampler->getSource()->getValue() );
            daeSIDResolver res2( currentEffect, target.c_str() );
            el = res2.getElement();
            if (!el)
            {
                return false;
            }
            cnp = daeSafeCast< domCommon_newparam_type >( el );
            npc = daeSafeCast< domFx_newparam_common >( el );
            
            if (cnp)
            {
                surface = cnp->getSurface();
            }
            else if (npc)
            {
                surface = npc->getFx_basic_type_common()->getSurface();
            }
            
            if (!surface)
            {
                return false;
            }
            
            //look for the domImage based on the surface initialization stuff
            daeIDRef &ref = surface->getFx_surface_init_common()->getInit_from_array()[0]->getValue();
            
            dImg = daeSafeCast< domImage >(ref.getElement());
        }
        else
        {
            // check if the attribute texture is already the id of the image in the <library_images>
            
            // Example:
            // <texture texture="file1-image" texcoord="CHANNEL0">
            // <library_images>
            //      <image id="file1-image" name="file1"><init_from>./Stone.jpg</init_from></image>
            // </library_images>
            
            std::string textureId = std::string(tex->getTexture());
            
            domLibrary_images_Array &imagesLib = this->dom->getLibrary_images_array();
            for (size_t i = 0; i < imagesLib.getCount(); i++)
            {
                const domLibrary_imagesRef& libImages = imagesLib.get(i);
                const domImage_Array& imageArray = libImages->getImage_array();
                for (size_t j=0; j < imageArray.getCount(); j++)
                {
                    domImage *img = daeSafeCast< domImage >(imageArray.get(j));
                    if(std::string(img->getId()).compare(textureId) == 0)
                    {
                        if(img->getInit_from() && img->getInit_from()->hasValue())
                        {
							if(!FilepathUtils::setTexturePaths(img->getInit_from()->getValue().str(), 
												img->getInit_from()->getValue().originalStr(), 
												effect))
							{
								return false;
							}
                            isTexFilenameSet = true;
                        }
                    }
                }
            }
        }
        
        if (!dImg && !isTexFilenameSet)
        {
            return false;
        }
        
        //Got a sampler and a surface and an imaged. Time to create the texture stuff for osg
        //osg::Image *img = NULL;
        if (dImg && dImg->getInit_from() && !isTexFilenameSet)
        {
            // daeURI uri = dImg->getInit_from()->getValue();
            dImg->getInit_from()->getValue().validate();
            if ( std::string( dImg->getInit_from()->getValue().getProtocol() ) == std::string( "file" ) )
            {
                std::string path =  dImg->getInit_from()->getValue().pathDir()+
                dImg->getInit_from()->getValue().pathFile();
                
                std::string filePath = dImg->getInit_from()->getValue().originalStr();
                
                // remove space encodings
                path = cdom::uriToNativePath(path);
                
                if(path.empty())
                {
                    return false;
                }

				if(!FilepathUtils::setTexturePaths(path, EncoderArguments::getInstance()->getFilePath(), effect))
				{
					return false;
				}
            }
            else
            {
                return false;
            }
        }

        //set texture parameters
        if (sampler)
        {
            if(!effect.isPowerOfTwo())
            {
                effect.setWrapS(Effect::CLAMP);
                effect.setWrapT(Effect::CLAMP);
                effect.setMinFilter(Effect::LINEAR);
                effect.setMagFilter(Effect::LINEAR);
                GP_WARNING(WARN_TEXTURES_NONPOWER_OF_2, effect.getTextureSourcePath().c_str());

				if(sampler->getWrap_s() && (sampler->getWrap_s()->getValue() == FX_SAMPLER_WRAP_COMMON_WRAP || sampler->getWrap_s()->getValue() == FX_SAMPLER_WRAP_COMMON_MIRROR))
				{
					GP_ERROR(ERR_NO_CLAMP_FOR_NOP_USED, "");
				}
				if(sampler->getWrap_t() && (sampler->getWrap_t()->getValue() == FX_SAMPLER_WRAP_COMMON_WRAP || sampler->getWrap_t()->getValue() == FX_SAMPLER_WRAP_COMMON_MIRROR))
				{
					GP_ERROR(ERR_NO_CLAMP_FOR_NOP_USED, "");
				}
            }
            else
            {
                /**************   wrapS   **************/
                if (sampler->getWrap_s())
                {
                    gameplay::Effect::Wrap wrap;
                    switch( sampler->getWrap_s()->getValue() )
                    {
                        case FX_SAMPLER_WRAP_COMMON_WRAP:
                        case FX_SAMPLER_WRAP_COMMON_MIRROR:
                            wrap = Effect::REPEAT;
                            break;
                        case FX_SAMPLER_WRAP_COMMON_CLAMP:
                        case FX_SAMPLER_WRAP_COMMON_NONE:
                        case FX_SAMPLER_WRAP_COMMON_BORDER:
                        default:
                            wrap = Effect::CLAMP;
                            break;
                    }
                    effect.setWrapS(wrap);
                }
                else
                {
                    effect.setWrapS(Effect::REPEAT);
                }
                /**************   wrapT   **************/
                if (sampler->getWrap_t())
                {
                    gameplay::Effect::Wrap wrap;
                    switch( sampler->getWrap_t()->getValue() )
                    {
                        case FX_SAMPLER_WRAP_COMMON_WRAP:
                        case FX_SAMPLER_WRAP_COMMON_MIRROR:
                            wrap = Effect::REPEAT;
                            break;
                        case FX_SAMPLER_WRAP_COMMON_CLAMP:
                        case FX_SAMPLER_WRAP_COMMON_NONE:
                        case FX_SAMPLER_WRAP_COMMON_BORDER:
                        default:
                            wrap = Effect::CLAMP;
                            break;
                    }
                    effect.setWrapT(wrap);
                }
                else
                {
                    effect.setWrapT(Effect::REPEAT);
                }
            }
            /**************   Minfilter   **************/
            if (sampler->getMinfilter())
            {
                gameplay::Effect::Filter filter;
                switch( sampler->getMinfilter()->getValue() )
                {
                    case FX_SAMPLER_FILTER_COMMON_NEAREST:
                        filter = Effect::NEAREST;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_LINEAR:
                        filter = Effect::LINEAR;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_NEAREST_MIPMAP_NEAREST:
                        filter = Effect::NEAREST_MIPMAP_NEAREST;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_LINEAR_MIPMAP_NEAREST:
                        filter = Effect::LINEAR_MIPMAP_NEAREST;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_NONE:
                    case FX_SAMPLER_FILTER_COMMON_NEAREST_MIPMAP_LINEAR:
                        filter = Effect::NEAREST_MIPMAP_LINEAR;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_LINEAR_MIPMAP_LINEAR:
                        filter = Effect::LINEAR_MIPMAP_LINEAR;
                        break;
                    default:
                        filter = Effect::LINEAR;
                        break;
                }
                effect.setMinFilter(filter);
            }
            else
            {
                effect.setMinFilter(Effect::LINEAR);
            }
            /**************   Magfilter   **************/
            if (sampler->getMagfilter())
            {
                gameplay::Effect::Filter filter;
                switch( sampler->getMagfilter()->getValue() )
                {
                    case FX_SAMPLER_FILTER_COMMON_NEAREST:
                        filter = Effect::NEAREST;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_NONE:
                    case FX_SAMPLER_FILTER_COMMON_LINEAR:
                        filter = Effect::LINEAR;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_NEAREST_MIPMAP_NEAREST:
                        filter = Effect::NEAREST_MIPMAP_NEAREST;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_LINEAR_MIPMAP_NEAREST:
                        filter = Effect::LINEAR_MIPMAP_NEAREST;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_NEAREST_MIPMAP_LINEAR:
                        filter = Effect::NEAREST_MIPMAP_LINEAR;
                        break;
                    case FX_SAMPLER_FILTER_COMMON_LINEAR_MIPMAP_LINEAR:
                        filter = Effect::LINEAR_MIPMAP_LINEAR;
                        break;
                    default:
                        filter = Effect::LINEAR;
                        break;
                }
                effect.setMagFilter(filter);
            }
            else
            {
                effect.setMagFilter(Effect::LINEAR);
            }
            if (sampler->getBorder_color())
            {
//                const domFloat4 &col = sampler->getBorder_color()->getValue();
//                t2D->setBorderColor( osg::Vec4( col[0], col[1], col[2], col[3] ) );
            }
        }
        else 
        {
            effect.setWrapS(Effect::REPEAT);
            effect.setWrapT(Effect::REPEAT);
            effect.setMinFilter(Effect::LINEAR);
            effect.setMagFilter(Effect::LINEAR);
        }
        return true;
    }
}