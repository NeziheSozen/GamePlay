#ifdef USE_FBX

#include "Base.h"
#include <algorithm>
#include <string>
#include <sstream>

#include "FBXSceneEncoder.h"
#include "EncoderArguments.h"
#include "SceneFile.h"
#include "MaterialEnhancer.h"
#include "ImageUtils.h"
#include "FilepathUtils.h"

using namespace gameplay;

static IdStore idStore;

/**
 * Returns the aspect ratio from the given camera.
 * 
 * @param fbxCamera The FBX camera to get the aspect ratio from.
 * 
 * @return The aspect ratio from the camera.
 */
static float getAspectRatio(FbxCamera* fbxCamera);

/**
 * Returns the field of view Y from the given camera.
 * 
 * @param fbxCamera The camera to get the fiew of view from.
 * 
 * @return The field of view Y.
 */
static float getFieldOfView(FbxCamera* fbxCamera);

/**
 * Loads the texture coordinates from given mesh's polygon part into the vertex.
 * 
 * @param fbxMesh The mesh to get the polygon from.
 * @param uvs The UV list to load tex coords from.
 * @param uvSetIndex The UV set index of the uvs.
 * @param polyIndex The index of the polygon in the mesh.
 * @param posInPoly The position of the vertex in the polygon.
 * @param meshVertexIndex The index of the vertex in the mesh.
 * @param vertex The vertex to copy the texture coordinates to.
 */
static void loadTextureCoords(FbxMesh* fbxMesh, const FbxGeometryElementUV* uvs, int uvSetIndex, int polyIndex, int posInPoly, int meshVertexIndex, Vertex* vertex);

/**
 * Loads the normal from the mesh and adds it to the given vertex.
 * 
 * @param fbxMesh The mesh to get the polygon from.
 * @param vertexIndex The vertex index in the mesh.
 * @param controlPointIndex The control point index.
 * @param vertex The vertex to copy to.
 */
static void loadNormal(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex);

/**
 * Loads the tangent from the mesh and adds it to the given vertex.
 * 
 * @param fbxMesh The mesh to load from.
 * @param vertexIndex The index of the vertex within fbxMesh.
 * @param controlPointIndex The control point index.
 * @param vertex The vertex to copy to.
 */
static void loadTangent(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex);

/**
 * Loads the binormal from the mesh and adds it to the given vertex.
 * 
 * @param fbxMesh The mesh to load from.
 * @param vertexIndex The index of the vertex within fbxMesh.
 * @param controlPointIndex The control point index.
 * @param vertex The vertex to copy to.
 */
static void loadBinormal(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex);

/**
 * Loads the vertex diffuse color from the mesh and adds it to the given vertex.
 * 
 * @param fbxMesh The mesh to load from.
 * @param vertexIndex The index of the vertex within fbxMesh.
 * @param controlPointIndex The control point index.
 * @param vertex The vertex to copy to.
 */
static void loadVertexColor(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex);

/**
 * Loads the blend weight and blend indices data into the vertex.
 * 
 * @param vertexWeights List of vertex weights. The x member contains the blendIndices. The y member contains the blendWeights.
 * @param vertex The vertex to copy the blend data to.
 */
static void loadBlendData(const std::vector<Vector2>& vertexWeights, Vertex* vertex);

/**
 * Loads the blend weights and blend indices from the given mesh.
 * 
 * Each element of weights is a list of Vector2s where "x" is the blend index and "y" is the blend weight.
 * 
 * @param fbxMesh The mesh to load from.
 * @param weights List of blend weights and blend indices for each vertex.
 * 
 * @return True if this mesh has a mesh skin, false otherwise.
 */
static bool loadBlendWeights(FbxMesh* fbxMesh, std::vector<std::vector<Vector2> >& weights);

/**
 * Copies from an FBX matrix to a float[16] array.
 */
static void copyMatrix(const FbxMatrix& fbxMatrix, float* matrix);

/**
 * Copies from an FBX matrix to a gameplay matrix.
 */
static void copyMatrix(const FbxMatrix& fbxMatrix, Matrix& matrix);

/**
 * Finds the min and max start time and stop time of the given animation curve.
 * 
 * startTime is updated if the animation curve contains a start time that is less than startTime.
 * stopTime is updated if the animation curve contains a stop time that is greater than stopTime.
 * frameRate is updated if the animation curve contains a frame rate that is greater than frameRate.
 * 
 * @param animCurve The animation curve to read from.
 * @param startTime The min start time. (in/out)
 * @param stopTime The max stop time. (in/out)
 * @param frameRate The frame rate. (in/out)
 */
static void findMinMaxTime(FbxAnimCurve* animCurve, float* startTime, float* stopTime, float* frameRate);

/**
 * Appends key frame data to the given node for the specified animation target attribute.
 * 
 * @param fbxNode The node to get the matrix transform from.
 * @param channel The aniamtion channel to write values into.
 * @param time The time of the keyframe.
 * @param scale The evaluated scale for the keyframe.
 * @param rotation The evalulated rotation for the keyframe.
 * @param translation The evalulated translation for the keyframe.

 */
static void appendKeyFrame(FbxNode* fbxNode, AnimationChannel* channel, float time, const Vector3& scale, const Quaternion& rotation, const Vector3& translation);

/**
 * Decomposes the given node's matrix transform at the given time and copies to scale, rotation and translation.
 * 
 * @param fbxNode The node to get the matrix transform from.
 * @param time The time to get the matrix transform from.
 * @param scale The scale to copy to.
 * @param rotation The rotation to copy to.
 * @param translation The translation to copy to.
 */
static void decompose(FbxNode* fbxNode, float time, Vector3* scale, Quaternion* rotation, Vector3* translation);

/**
 * Creates an animation channel that targets the given node and target attribute using the given key times and key values.
 * 
 * @param fbxNode The node to target.
 * @param targetAttrib The attribute type to target.
 * @param keyTimes The key times for the animation channel.
 * @param keyValues The key values for the animation channel.
 * 
 * @return The newly created animation channel.
 */
static AnimationChannel* createAnimationChannel(FbxNode* fbxNode, unsigned int targetAttrib, const std::vector<float>& keyTimes, const std::vector<float>& keyValues);

void addScaleChannel(Animation* animation, FbxNode* fbxNode, float startTime, float stopTime);

void addTranslateChannel(Animation* animation, FbxNode* fbxNode, float startTime, float stopTime);

/**
 * Determines if it is possible to automatically group animations for mesh skins.
 * 
 * @param fbxScene The FBX scene to search.
 * 
 * @return True if there is at least one mesh skin that has animations that can be grouped.
 */
bool isGroupAnimationPossible(FbxScene* fbxScene);
bool isGroupAnimationPossible(FbxNode* fbxNode);
bool isGroupAnimationPossible(FbxMesh* fbxMesh);

FbxAnimCurve* getCurve(FbxPropertyT<FbxDouble3>& prop, FbxAnimLayer* animLayer, const char* pChannel)
{
#if FBXSDK_VERSION_MAJOR == 2013 && FBXSDK_VERSION_MINOR == 1
    return prop.GetCurve<FbxAnimCurve>(animLayer, pChannel);
#else
    return prop.GetCurve(animLayer, pChannel);
#endif
}

////////////////////////////////////
// Member Functions
////////////////////////////////////

FBXSceneEncoder::FBXSceneEncoder()
    : _groupAnimation(NULL), _autoGroupAnimations(false)
{
}

FBXSceneEncoder::~FBXSceneEncoder()
{
}

void FBXSceneEncoder::write(const std::string& filepath, EncoderArguments& arguments)
{
    FbxManager* sdkManager = FbxManager::Create();
    FbxIOSettings *ios = FbxIOSettings::Create(sdkManager, IOSROOT);
    sdkManager->SetIOSettings(ios);
    FbxImporter* importer = FbxImporter::Create(sdkManager,"");
    
    if (!importer->Initialize(filepath.c_str(), -1, sdkManager->GetIOSettings()))
    {
//        LOG(1, "Call to FbxImporter::Initialize() failed.\n");
        GP_ERROR(ERR_FBX_IMPORTER_NOT_INITIALIZED, filepath.c_str());
//        LOG(1, "Error returned: %s\n\n", importer->GetLastErrorString());
        GP_ERROR(ERR_FBX_ERROR_MESSAGE, importer->GetLastErrorString());
        exit(-1);
    }
    
    FbxScene* fbxScene = FbxScene::Create(sdkManager,"__FBX_SCENE__");

    print("Loading FBX file.");
    importer->Import(fbxScene);
    importer->Destroy();

    print("Loading Scene.");
    loadScene(fbxScene);
    
    // Determine if animations should be grouped.
    _autoGroupAnimations = (EncoderArguments::getInstance()->groupAnimations() == 1 ? true : false);
    // add grouped animations
    Scene* scene = _gamePlayFile.getScene();
    if(scene && _autoGroupAnimations)
    {
        std::list<Node*> allNodes = scene->getSceneNodes();
        for (std::list<Node*>::iterator it = allNodes.begin(); it != allNodes.end(); ++it)
        {
            Node* node = ((Node*)(*it));
            if(node->getParent() == NULL)
            {
                _autoGroupAnimations = true;
                std::stringstream ss;
                ss << node->getId() << "_animation";
                EncoderArguments::getInstance()->addGroupAnimationNode(node->getId());
                EncoderArguments::getInstance()->addGroupAnimationAnimationNode(ss.str());
            }
        }
    }
    
    if (EncoderArguments::getInstance()->getGroupAnimationAnimationId().empty() && isGroupAnimationPossible(fbxScene))
    {
        if (EncoderArguments::getInstance()->groupAnimations() == -1 && promptUserGroupAnimations())
        {
            _autoGroupAnimations = true;
        }
    }
    
    print("Loading animations.");
    loadAnimations(fbxScene, arguments);
    sdkManager->Destroy();

    print("Optimizing GamePlay Binary.");
    _gamePlayFile.adjust();
    if (_autoGroupAnimations)
    {
        _gamePlayFile.groupMeshSkinAnimations();
    }
    
    // write file only if material output enabled
    // it is possible that the user wants to get the scene-file only
    if(arguments.materialOutputEnabled())
    {
        // connect light to material
        MaterialEnhancer* me = new MaterialEnhancer();
        me->setLightInMaterial(_gamePlayFile);

        // get filepath:
        std::string filepath = EncoderArguments::getInstance()->getMaterialOutputPath();
        FILE* _file = fopen(filepath.c_str(), "w");
        if (!_file)
        {
            return;
        }

        std::list<Material*>::iterator it;
        for (it = _materials.begin(); it != _materials.end(); ++it)
        {
            (*it)->writeText(_file);
        }
    }

    if (arguments.sceneOutputEnabled())
    {
        SceneFile* sceneFile = new SceneFile(_gamePlayFile);
        std::string filepathScene = arguments.getSceneOutputPath();
        FILE* _file = fopen(filepathScene.c_str(), "w");
        sceneFile->writeFile(_file);
    }

    std::string outputFilePath = arguments.getOutputFilePath();

    if (arguments.textOutputEnabled())
    {
        int pos = outputFilePath.find_last_of('.');
        if (pos > 2)
        {
            std::string path = outputFilePath.substr(0, pos);
            path.append(".xml");
            GP_INFO(INFO_SAVE_DEBUG_FILE, path.c_str());
            if (!_gamePlayFile.saveText(path))
            {
                GP_ERROR(ERR_WRITING_TEXT_FILE, path.c_str());
            }
        }
    }
    else
    {
        GP_INFO(INFO_SAVE_BINARY_FILE, outputFilePath.c_str());
        if (!_gamePlayFile.saveBinary(outputFilePath))
        {
            GP_ERROR(ERR_WRITING_BINARY_FILE, outputFilePath.c_str());
        }
    }
}

void FBXSceneEncoder::loadScene(FbxScene* fbxScene)
{
    Scene* scene = new Scene();

    std::string sceneName;
    if (!fbxScene->GetName()) {
        sceneName.assign(fbxScene->GetName());
    }

    if (sceneName.length() == 0)
    {
        sceneName.assign("__SCENE__");
    }
    std::stringstream ss;
    ss << fbxScene->GetUniqueID();

    const char* name = idStore.getId(sceneName, ss.str()).c_str();
    scene->setId(name);

    // Load all of the nodes and their contents.
    FbxNode* rootNode = fbxScene->GetRootNode();
    if (rootNode)
    {
        print("Triangulate.");
        triangulateRecursive(rootNode);

        print("Load nodes.");
        // Don't include the FBX root node in the GPB.
        const int childCount = rootNode->GetChildCount();
        for (int i = 0; i < childCount; ++i)
        {
            Node* node = loadNode(rootNode->GetChild(i));
            if (node)
            {
                scene->add(node);
            }
        }
    }

    // Load the MeshSkin information from the scene's poses.
    loadBindShapes(fbxScene);

    // Find the ambient light of the scene
    FbxColor ambientColor = fbxScene->GetGlobalSettings().GetAmbientColor();
    scene->setAmbientColor((float)ambientColor.mRed, (float)ambientColor.mGreen, (float)ambientColor.mBlue);
    
    // Assign the first camera node (if there is one) in the scene as the active camera
    // This ensures that if there's a camera in the scene that it is assigned as the 
    // active camera.
    // TODO: add logic to find the "active" camera node in the fbxScene
    scene->setActiveCameraNode(scene->getFirstCameraNode());
    
    _gamePlayFile.addScene(scene);
}

void FBXSceneEncoder::loadAnimationChannels(FbxAnimLayer* animLayer, FbxNode* fbxNode, Animation* animation)
{
    std::stringstream ss;
    ss << fbxNode->GetUniqueID();
    const char* name = idStore.getId(fbxNode->GetName(), ss.str()).c_str();
    //Node* node = _gamePlayFile.getNode(name);

    // Determine which properties are animated on this node
    // Find the transform at each key frame
    // TODO: Ignore properties that are not animated (scale, rotation, translation)
    // This should result in only one animation channel per animated node.

    float startTime = FLT_MAX, stopTime = -1.0f, frameRate = -FLT_MAX;
    bool tx = false, ty = false, tz = false, rx = false, ry = false, rz = false, sx = false, sy = false, sz = false;
    FbxAnimCurve* animCurve = NULL;
    animCurve = getCurve(fbxNode->LclTranslation, animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    if (animCurve)
    {
        tx = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclTranslation, animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    if (animCurve)
    {
        ty = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclTranslation, animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
    if (animCurve)
    {
        tz = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclRotation, animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    if (animCurve)
    {
        rx = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclRotation, animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    if (animCurve)
    {
        ry = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclRotation, animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
    if (animCurve)
    {
        rz = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclScaling, animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    if (animCurve)
    {
        sx = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclScaling, animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    if (animCurve)
    {
        sy = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }
    animCurve = getCurve(fbxNode->LclScaling, animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
    if (animCurve)
    {
        sz = true;
        findMinMaxTime(animCurve, &startTime, &stopTime, &frameRate);
    }

    if (!(sx || sy || sz || rx || ry || rz || tx || ty || tz))
        return; // no animation channels

    assert(startTime != FLT_MAX);
    assert(stopTime >= 0.0f);

    // Determine which animation channels to create
    std::vector<unsigned int> channelAttribs;
    if (sx && sy && sz)
    {
        if (rx || ry || rz)
        {
            if (tx && ty && tz)
            {
                channelAttribs.push_back(Transform::ANIMATE_SCALE_ROTATE_TRANSLATE);
            }
            else
            {
                channelAttribs.push_back(Transform::ANIMATE_SCALE_ROTATE);
                if (tx)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_X);
                if (ty)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Y);
                if (tz)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Z);
            }
        }
        else
        {
            if (tx && ty && tz)
            {
                channelAttribs.push_back(Transform::ANIMATE_SCALE_TRANSLATE);
            }
            else
            {
                channelAttribs.push_back(Transform::ANIMATE_SCALE);
                if (tx)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_X);
                if (ty)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Y);
                if (tz)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Z);
            }
        }
    }
    else
    {
        if (rx || ry || rz)
        {
            if (tx && ty && tz)
            {
                channelAttribs.push_back(Transform::ANIMATE_ROTATE_TRANSLATE);
            }
            else
            {
                channelAttribs.push_back(Transform::ANIMATE_ROTATE);
                if (tx)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_X);
                if (ty)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Y);
                if (tz)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Z);
            }
        }
        else
        {
            if (tx && ty && tz)
            {
                channelAttribs.push_back(Transform::ANIMATE_TRANSLATE);
            }
            else
            {
                if (tx)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_X);
                if (ty)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Y);
                if (tz)
                    channelAttribs.push_back(Transform::ANIMATE_TRANSLATE_Z);
            }
        }

        if (sx)
            channelAttribs.push_back(Transform::ANIMATE_SCALE_X);
        if (sy)
            channelAttribs.push_back(Transform::ANIMATE_SCALE_Y);
        if (sz)
            channelAttribs.push_back(Transform::ANIMATE_SCALE_Z);
    }
    unsigned int channelCount = channelAttribs.size();
    assert(channelCount > 0);

    // Allocate channel list
    int channelStart = animation->getAnimationChannelCount();
    for (unsigned int i = 0; i < channelCount; ++i)
    {
        AnimationChannel* channel = new AnimationChannel();
        channel->setTargetId(name);
        channel->setInterpolation(AnimationChannel::LINEAR);
        channel->setTargetAttribute(channelAttribs[i]);
        animation->add(channel);
    }

    // Evaulate animation curve in increments of frameRate and populate channel data.
    FbxAMatrix fbxMatrix;
    Matrix matrix;
    double increment = 1000.0 / (double)frameRate;
    int frameCount = (int)ceil((double)(stopTime - startTime) / increment) + 1; // +1 because stop time is inclusive
    for (int frame = 0; frame < frameCount; ++frame)
    {
        double time = startTime + (frame * (double)increment);

        // Note: We used to clamp time to stop time, but FBX sdk does not always produce
        // and accurate stopTime (sometimes it is rounded down for some reason), so I'm
        // disabling this clamping for now as it seems more accurate under normal circumstances.
        //time = std::min(time, (double)stopTime);

        // Evalulate the animation at this time
        FbxTime kTime;
        kTime.SetMilliSeconds((FbxLongLong)time);
        fbxMatrix = fbxNode->EvaluateLocalTransform(kTime);
        copyMatrix(fbxMatrix, matrix);

        // Decompose the evalulated transformation matrix into separate
        // scale, rotation and translation.
        Vector3 scale;
        Quaternion rotation;
        Vector3 translation;
        matrix.decompose(&scale, &rotation, &translation);
        rotation.normalize();

        // Append keyframe data to all channels
        for (unsigned int i = channelStart, channelEnd = channelStart + channelCount; i < channelEnd; ++i)
        {
            appendKeyFrame(fbxNode, animation->getAnimationChannel(i), time, scale, rotation, translation);
        }
    }

    if (_groupAnimation != animation)
    {
        // TODO explain
        _gamePlayFile.addAnimation(animation);
    }
}

void FBXSceneEncoder::loadAnimationLayer(FbxAnimLayer* fbxAnimLayer, FbxNode* fbxNode, const EncoderArguments& arguments)
{
    bool animationGroupId = false;
    std::stringstream ss;
    ss << fbxNode->GetUniqueID();
    const char* name = idStore.getId(fbxNode->GetName(), ss.str()).c_str();
    // Check if this node's animations are supposed to be grouped
    if (name && arguments.containsGroupNodeId(name))
    {
        animationGroupId = true;
        _groupAnimation = new Animation();
        _groupAnimation->setId(arguments.getAnimationId(name));
    }
    Animation* animation = _groupAnimation;
    if (!animation)
    {
        animation = new Animation();
        animation->setId(name);
    }
    loadAnimationChannels(fbxAnimLayer, fbxNode, animation);

    const int childCount = fbxNode->GetChildCount();
    for (int modelCount = 0; modelCount < childCount; ++modelCount)
    {
        loadAnimationLayer(fbxAnimLayer, fbxNode->GetChild(modelCount), arguments);
    }
    if (animationGroupId)
    {
        _gamePlayFile.addAnimation(_groupAnimation);
        _groupAnimation = NULL;
    }
}

void FBXSceneEncoder::loadAnimations(FbxScene* fbxScene, const EncoderArguments& arguments)
{
    FbxAnimEvaluator* evaluator = fbxScene->GetEvaluator();
    if (!evaluator)
        return;
    FbxAnimStack* animStack = evaluator->GetContext();
    if (!animStack)
        return;

    for (int i = 0; i < fbxScene->GetSrcObjectCount(FBX_TYPE(FbxAnimStack)); ++i)
    {
        FbxAnimStack* animStack = FbxCast<FbxAnimStack>(fbxScene->GetSrcObject(FBX_TYPE(FbxAnimStack), i));
        int nbAnimLayers = animStack->GetMemberCount(FBX_TYPE(FbxAnimLayer));
        for (int l = 0; l < nbAnimLayers; ++l)
        {
            FbxAnimLayer* animLayer = animStack->GetMember(FBX_TYPE(FbxAnimLayer), l);
            loadAnimationLayer(animLayer, fbxScene->GetRootNode(), arguments);
        }
    }
}

Node* FBXSceneEncoder::loadNode(FbxNode* fbxNode)
{
    Node* node = NULL;

    // Check if this node has already been loaded
    std::stringstream ss;
    ss << fbxNode->GetUniqueID();
    const std::string& id(idStore.getId(fbxNode->GetName(), ss.str()).c_str());

    node = _gamePlayFile.getNode(id.c_str());
    if (node)
    {
        return node;
    }

    node = new Node();
    node->setId(id);
    _gamePlayFile.addNode(node);

    transformNode(fbxNode, node);
    
    loadCamera(fbxNode, node);
    loadLight(fbxNode, node);
    loadModel(fbxNode, node);

    if (fbxNode->GetSkeleton())
    {
        // Indicate that this is a joint node for the purpose of debugging.
        // The XML debug output will print that this node is a joint.
        node->setIsJoint(true);
    }

    // Load child nodes
    const int childCount = fbxNode->GetChildCount();
    for (int i = 0; i < childCount; ++i)
    {
        Node* child = loadNode(fbxNode->GetChild(i));
        if (child)
        {
            node->addChild(child);
        }
    }
    return node;
}

Mesh* FBXSceneEncoder::getMesh(FbxUInt64 meshId)
{
    // Check if this mesh was already loaded.
    std::map<FbxUInt64, Mesh*>::iterator it = _meshes.find(meshId);
    if (it != _meshes.end())
    {
        return it->second;
    }
    return NULL;
}

void FBXSceneEncoder::saveMesh(FbxUInt64 meshId, Mesh* mesh)
{
    assert(mesh);
    if (!getMesh(meshId))
    {
        _meshes[meshId] = mesh;
    }
}

void FBXSceneEncoder::print(const char* str)
{
//    LOG(1, "%s\n", str);
    GP_INFO(INFO_LOG, str);
    
}

void FBXSceneEncoder::transformNode(FbxNode* fbxNode, Node* node)
{
    FbxAMatrix matrix;
    float m[16];
    
    if (fbxNode->GetCamera() || fbxNode->GetLight())
    {
        FbxAMatrix rotateAdjust;
        
        if(fbxNode->GetLight())
        {
            /*
             * according to the fbx-documentation the light's forward vector
             * points along a node's negative Y axis.
             * so we have to rotate it by 90° around the X-axis to correct it.
             */
            if(fbxNode->RotationActive.Get())
            {
                const FbxVector4& postRotation = fbxNode->PostRotation.Get();
                fbxNode->SetPostRotation(FbxNode::eSourcePivot, FbxVector4(postRotation.mData[0] + 90.0,
                                                                           postRotation.mData[1],
                                                                           postRotation.mData[2])
                                         );
            }
            else
            {
                // if the rotation is deactivated we have to rotate it anyway to get the correct transformation in the end
                rotateAdjust.SetR(FbxVector4(-90.0, 0.0, 0.0));
            }
            
            matrix = fbxNode->EvaluateLocalTransform() * rotateAdjust;
        }
        else if(fbxNode->GetCamera())
        {
            // TODO: use the EvaluateLocalTransform() function for the transformations for the camera
            /*
             * the current implementation ignores pre- and postrotation among others (usually happens with fbx-export from blender)
             *
             * Some info for a future implementation:
             * according to the fbx-documentation the camera's forward vector
             * points along a node's positive X axis.
             * so we have to correct it if we use the EvaluateLocalTransform-function
             * just rotating it by 90° around the Y axis (similar to above) doesn't work
             */
              matrix.SetTRS(fbxNode->LclTranslation.Get(), fbxNode->LclRotation.Get(), fbxNode->LclScaling.Get());
        }
        
        copyMatrix(matrix, m);
        node->setTransformMatrix(m);
    }
    else
    {
        matrix = fbxNode->EvaluateLocalTransform();
        copyMatrix(matrix, m);
        node->setTransformMatrix(m);
    }
}

void FBXSceneEncoder::loadBindShapes(FbxScene* fbxScene)
{
    float m[16];
    const int poseCount = fbxScene->GetPoseCount();
    for (int i = 0; i < poseCount; ++i)
    {
        FbxPose* pose = fbxScene->GetPose(i);
        assert(pose);
        if (pose->IsBindPose() && pose->GetCount() > 0)
        {
            FbxNode* fbxNode = pose->GetNode(0);
            if (fbxNode->GetMesh() != NULL)
            {
                std::stringstream ss;
                ss << fbxNode->GetUniqueID();
                const std::string& id(idStore.getId(fbxNode->GetName(), ss.str()).c_str());
                Node* node = _gamePlayFile.getNode(id.c_str());
                // assert(node && node->getModel());

                Model* model = node->getModel();
                if (model && model->getSkin())
                {
                    MeshSkin* skin = model->getSkin();
                    copyMatrix(pose->GetMatrix(0), m);
                    skin->setBindShape(m);
                }
            }
        }
    }
}

void FBXSceneEncoder::loadCamera(FbxNode* fbxNode, Node* node)
{
    FbxCamera* fbxCamera = fbxNode->GetCamera();
    if (!fbxCamera)
    {
        return;
    }
    Camera* camera = new Camera();
    std::stringstream ss;
    ss << fbxNode->GetUniqueID();
    const char* name = idStore.getId(fbxNode->GetName(), ss.str()).c_str();

    if (name)
    {
        std::string id(name);
        id.append("_Camera");
        camera->setId(id);
    }
    camera->setAspectRatio(getAspectRatio(fbxCamera));
    camera->setNearPlane((float)fbxCamera->NearPlane.Get());
    camera->setFarPlane((float)fbxCamera->FarPlane.Get());

    if (fbxCamera->ProjectionType.Get() == FbxCamera::eOrthogonal)
    {
        camera->setOrthographic();
        camera->setViewportWidth((float)fbxCamera->GetApertureWidth());
        camera->setViewportWidth((float)fbxCamera->GetApertureHeight());
        // xmag in FBX can be calculated from: OrthoZoom * 30.0 / 2.0
        camera->setViewportWidth((float)fbxCamera->OrthoZoom.Get() * 15.0f);
    }
    else if (fbxCamera->ProjectionType.Get() == FbxCamera::ePerspective)
    {
        camera->setPerspective();
        camera->setFieldOfView(getFieldOfView(fbxCamera));
    }
    else
    {
//        LOG(2, "Warning: Unknown camera type in node.\n");
        GP_WARNING(WARN_UNKNOWN_CAMERA_TYPE, name ? name : "Unknowm Node");
        return;
    }
    _gamePlayFile.addCamera(camera);
    node->setCamera(camera);
}

void FBXSceneEncoder::loadLight(FbxNode* fbxNode, Node* node)
{
    FbxLight* fbxLight = fbxNode->GetLight();
    if (!fbxLight)
    {
        return;
    }
    Light* light = new Light();
    std::stringstream ss;
    ss << fbxNode->GetUniqueID();
    const char* name = idStore.getId(fbxNode->GetName(), ss.str()).c_str();
    if (name)
    {
        std::string id(name);
        id.append("_Light");
        light->setId(id);
    }

    FbxDouble3 color = fbxLight->Color.Get();
    light->setColor((float)color[0], (float)color[1], (float)color[2]);
    
    switch (fbxLight->LightType.Get())
    {
    case FbxLight::ePoint:
    {
        FbxLight::EDecayType decayType = fbxLight->DecayType.Get();
        switch (decayType)
        {
        case FbxLight::eNone:
            // No decay. Can assume we have an ambient light, because ambient lights in the scene are 
            // converted to point lights with no decay when exporting to FBX.
            light->setAmbientLight();
            break;
        case FbxLight::eLinear:
            light->setPointLight();
            light->setLinearAttenuation((float)fbxLight->DecayStart.Get());
            break;
        case FbxLight::eQuadratic:
            light->setPointLight();
            light->setQuadraticAttenuation((float)fbxLight->DecayStart.Get());
            break;
        case FbxLight::eCubic:
        default:
            // Not supported..
            break;
        }
        break;
    }
    case FbxLight::eDirectional:
    {
        light->setDirectionalLight();
        break;
    }
    case FbxLight::eSpot:
    {
        light->setSpotLight();

        FbxLight::EDecayType decayType = fbxLight->DecayType.Get();
        switch (decayType)
        {
        case FbxLight::eNone:
            // No decay.
            break;
        case FbxLight::eLinear:
            light->setLinearAttenuation((float)fbxLight->DecayStart.Get());
            break;  
        case FbxLight::eQuadratic:
            light->setQuadraticAttenuation((float)fbxLight->DecayStart.Get());
            break;
        case FbxLight::eCubic:
            // Not supported..
            break;
        }

        light->setFalloffAngle((float)fbxLight->OuterAngle.Get()); // fall off angle
        break;
    }
    default:
    {
//        LOG(2, "Warning: Unknown light type in node.\n");
        GP_WARNING(WARN_UNKNOWN_LIGHT_TYPE, name ? name : "Unknowm Node");
        return;
    }
    }

    _gamePlayFile.addLight(light);
    node->setLight(light);
}

void FBXSceneEncoder::loadModel(FbxNode* fbxNode, Node* node)
{
    FbxMesh* fbxMesh = fbxNode->GetMesh();
    if (!fbxMesh)
    {
        return;
    }
    if (fbxMesh->IsTriangleMesh() && fbxMesh->GetPolygonCount())
    {
        Mesh* mesh = loadMesh(fbxMesh);
        Model* model = new Model();
        model->setMesh(mesh);
        node->setModel(model);
        loadSkin(fbxMesh, model);
        if (model->getSkin())
        {
            // TODO: explain
            node->resetTransformMatrix();
        }
    }
}

void FBXSceneEncoder::loadSkin(FbxMesh* fbxMesh, Model* model)
{
    const int deformerCount = fbxMesh->GetDeformerCount();
    for (int i = 0; i < deformerCount; ++i)
    {
        FbxDeformer* deformer = fbxMesh->GetDeformer(i);
        if (deformer->GetDeformerType() == FbxDeformer::eSkin)
        {
            FbxSkin* fbxSkin = static_cast<FbxSkin*>(deformer);

            MeshSkin* skin = new MeshSkin();

            std::vector<std::string> jointNames;
            std::vector<Node*> joints;
            std::vector<Matrix> bindPoses;

            const int clusterCount = fbxSkin->GetClusterCount();
            for (int j = 0; j < clusterCount; ++j)
            {
                FbxCluster* cluster = fbxSkin->GetCluster(j);
                assert(cluster);
                FbxNode* linkedNode = cluster->GetLink();
                if (linkedNode && linkedNode->GetSkeleton())
                {
                    std::stringstream ss;
                    ss << linkedNode->GetUniqueID();
                    const char* jointName = idStore.getId(linkedNode->GetName(), ss.str()).c_str();
                    assert(jointName);
                    jointNames.push_back(jointName);
                    Node* joint = loadNode(linkedNode);
                    assert(joint);
                    joints.push_back(joint);

                    FbxAMatrix matrix;
                    cluster->GetTransformLinkMatrix(matrix);
                    Matrix m;
                    copyMatrix(matrix.Inverse(), m);
                    bindPoses.push_back(m);
                }
            }
            skin->setJointNames(jointNames);
            skin->setJoints(joints);
            skin->setBindPoses(bindPoses);
            model->setSkin(skin);
            if(model->getMesh())
            {
                const int matCount = fbxMesh->GetNode()->GetMaterialCount();
                int meshPartSize = (matCount > 0) ? matCount : 1;
                FbxNode* fbxNodeSkin = fbxMesh->GetNode();
                for (int i = 0; i < meshPartSize; i++)
                {
                    FbxSurfaceMaterial* material = fbxNodeSkin->GetMaterial(i);
                    if (material) {
                        char uniqueId[70];
                        sprintf(uniqueId, "%llu", material->GetUniqueID());
                        std::string materialId = std::string(uniqueId);
                        Material* mat = getMaterial(materialId);
                        if(mat != NULL) {
                            // set number of joints
                            mat->setNumberOfJoints(joints.size());
                        }
                    }
                }
            }
            break;
        }
    }
}

FbxDouble FBXSceneEncoder::getAlpha(FbxDouble transparencyFactor, FbxPropertyT<FbxDouble3> transparencyColor) {
    if (transparencyFactor == 1.0) { // from Maya .fbx
        if (transparencyColor.Get()[0] >= DBL_MIN) {
            return 1.0-transparencyColor.Get()[0];
        } else {
            return 1.0;
        }
    } else { // from 3dsmax .fbx
        if (transparencyFactor >= DBL_MIN) {
            return 1.0 - transparencyFactor;
        } else {
            return 1.0;
        }
    }
}

void FBXSceneEncoder::loadDefaultMaterial(Mesh* mesh, MeshPart* meshPart)
{
    std::string materialId = std::string("__encoder_default_material__");

    Material* mat = getMaterial(materialId);
    meshPart->setMaterialSymbolName(materialId);
    if(mat == NULL) {
        mat = new Material();
        mat->setMaterialId(materialId);

        _materials.push_back(mat);
    }
    mesh->addInstanceMaterial(materialId, mat);

    FbxPropertyT<FbxDouble3> lKFbxDouble3;
    FbxPropertyT<FbxDouble> lKFbxDouble1;
    FbxColor theColor;

    
    mat->getEffect().setUseSpecular(false);
    // Ambient Color
    mat->getEffect().setAmbient(Vector4(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha));

    // Diffuse Color
    mat->getEffect().setDiffuse(Vector4(.5f, .5f, .5f, 1.0f));

    // Opacity
    mat->getEffect().setAlpha(1.0);
}

void FBXSceneEncoder::loadMaterial(Mesh* mesh, MeshPart* meshPart, FbxSurfaceMaterial* fbxMaterial)
{
    char uniqueId[70];
    sprintf(uniqueId, "%llu", fbxMaterial->GetUniqueID());
    std::string materialId = std::string(uniqueId);

    Material* mat = getMaterial(materialId);
    meshPart->setMaterialSymbolName(materialId);
    if(mat == NULL) {
        mat = new Material();
        mat->setMaterialId(materialId);

        _materials.push_back(mat);
    }
    mesh->addInstanceMaterial(materialId, mat); 

    FbxPropertyT<FbxDouble3> lKFbxDouble3;
    FbxPropertyT<FbxDouble> lKFbxDouble1;
    FbxColor theColor;

    FbxProperty lPropertyNormalMap = fbxMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);
    if(lPropertyNormalMap.IsValid())
    {
        if(lPropertyNormalMap.GetSrcObjectCount<FbxTexture>() > 0)
        {
            GP_WARNING(WARN_NORMALMAP_NOT_SUPPORTED, "");
        }
    }

    FbxProperty lPropertyBumpMap = fbxMaterial->FindProperty(FbxSurfaceMaterial::sBump);
    if(lPropertyBumpMap.IsValid())
    {
        if(lPropertyBumpMap.GetSrcObjectCount<FbxTexture>() > 0)
        {
            GP_WARNING(WARN_BUMPMAP_NOT_SUPPORTED, "");
        }
    }

    
    if (fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        mat->getEffect().setUseSpecular(true);
        // We found a Phong material.  Display its properties.
        // Display the Ambient Color
        lKFbxDouble3 =((FbxSurfacePhong *) fbxMaterial)->Ambient;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
        // LOG(1, "            Ambient: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
        mat->getEffect().setAmbient(Vector4(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha));

        // Display the Diffuse Color
        FbxProperty lProperty = ((FbxSurfacePhong *) fbxMaterial)->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if(lProperty.IsValid())
        {
            
            this->assignTexturesToMaterialFromFBXPropertyForMeshPart(mat, lProperty, meshPart);
        }

        FbxDouble alpha = this->getAlpha(((FbxSurfacePhong *)fbxMaterial)->TransparencyFactor, ((FbxSurfacePhong *)fbxMaterial)->TransparentColor);
        lKFbxDouble3 =((FbxSurfacePhong *) fbxMaterial)->Diffuse;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2], alpha);
        //LOG(1, "            Diffuse: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
        mat->getEffect().setDiffuse(Vector4(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha));

        // Display the Specular Color (unique to Phong materials)
        lKFbxDouble3 =((FbxSurfacePhong *) fbxMaterial)->Specular;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
//        LOG(1, "            Specular: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
        mat->getEffect().setSpecular(Vector4(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha));

        // TODO: Display the Emissive Color
        lKFbxDouble3 =((FbxSurfacePhong *) fbxMaterial)->Emissive;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
//        LOG(1, "            Emissive: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

        //Opacity is Transparency factor now
        lKFbxDouble1 =((FbxSurfacePhong *) fbxMaterial)->TransparencyFactor;
//        LOG(1, "            Opacity: %f\n", 1.0-lKFbxDouble1.Get());
        mat->getEffect().setAlpha(lKFbxDouble1.Get());

        // Display the Shininess
        lKFbxDouble1 =((FbxSurfacePhong *) fbxMaterial)->Shininess;
//        LOG(1, "            Shininess: %f\n", lKFbxDouble1.Get());
        float shininess = lKFbxDouble1.Get();
        if(shininess < 1.0)
        {
            shininess *= 128.0;
        }
        else if(shininess > 128.0)
        {
            shininess = 128.0;
        }
        mat->getEffect().setShininess(shininess);

        // TODO: Display the Reflectivity
        lKFbxDouble1 =((FbxSurfacePhong *) fbxMaterial)->ReflectionFactor;
//        LOG(1, "            Reflectivity: %f\n", lKFbxDouble1.Get());
    }
    else if(fbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId) )
    {
        mat->getEffect().setUseSpecular(false);
        // We found a Lambert material. Display its properties.
        // Display the Ambient Color
        lKFbxDouble3=((FbxSurfaceLambert *)fbxMaterial)->Ambient;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
//        LOG(1, "            Ambient: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
        mat->getEffect().setAmbient(Vector4(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha));

        // Display the Diffuse Color
        FbxProperty lProperty = ((FbxSurfaceLambert *) fbxMaterial)->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if(lProperty.IsValid())
        {
            this->assignTexturesToMaterialFromFBXPropertyForMeshPart(mat, lProperty, meshPart);
        }

        FbxDouble alpha = this->getAlpha(((FbxSurfaceLambert *)fbxMaterial)->TransparencyFactor, ((FbxSurfaceLambert *)fbxMaterial)->TransparentColor);

        // Display the Diffuse Color
        lKFbxDouble3 =((FbxSurfaceLambert *)fbxMaterial)->Diffuse;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2], alpha);
//        LOG(1, "            Diffuse: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
        mat->getEffect().setDiffuse(Vector4(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha));

        // TODO: Display the Emissive
        lKFbxDouble3 =((FbxSurfaceLambert *)fbxMaterial)->Emissive;
        theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
//        LOG(1, "            Emissive: %f, %f, %f, %f\n", theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

        // Display the Opacity
        lKFbxDouble1 =((FbxSurfaceLambert *)fbxMaterial)->TransparencyFactor;
        LOG(1, "            Opacity: %f\n", 1.0-lKFbxDouble1.Get());
        mat->getEffect().setAlpha(lKFbxDouble1.Get());
    }
    else {
        LOG(1, "*********** No Material\n");
        GP_WARNING(WARN_GENERIC_FBX_LOG, "No Material");
    }
}

void FBXSceneEncoder::assignTexturesToMaterialFromFBXPropertyForMeshPart(const Material* mat, const FbxProperty& property, const MeshPart* meshPart)
{
    // check if textures are available
    
    bool foundTextureForMesh = false;
    
    int lTextureCount = property.GetSrcObjectCount<FbxFileTexture>();
    FbxFileTexture* fileTexture = NULL;

    if ( 1 == lTextureCount )
    {
        
        fileTexture = property.GetSrcObject<FbxFileTexture>(0);
        
        this->addTextureToMaterial(fileTexture, mat);
        
        foundTextureForMesh = true;
        
    }
    else if ( lTextureCount > 1 )
    {
        
        fileTexture = property.GetSrcObject<FbxFileTexture>(0);
        
        this->addTextureToMaterial(fileTexture, mat);
        
        GP_WARNING(WARN_MULTIPLE_MATERIALS_ASSIGNED_FOR_MESH, meshPart->getId().c_str());
        
        foundTextureForMesh = true;
    }
    
    if (!foundTextureForMesh)
    {
        // if not, check if layered textures are available
        int layeredTextureCount = property.GetSrcObjectCount<FbxLayeredTexture>();
        if ( 1 == layeredTextureCount )
        {
            
            // proceed with single layered texture
            FbxLayeredTexture* layeredTexture = property.GetSrcObject<FbxLayeredTexture>(0);
            
            fileTexture = layeredTexture->GetSrcObject<FbxFileTexture>(0);
            this->addTextureToMaterial(fileTexture, mat);
            
            GP_INFO(INFO_USING_FIRST_LAYERED_TEXTURE, meshPart->getId().c_str());
            
        }else if ( layeredTextureCount > 1)
        {
            
            FbxLayeredTexture* layeredTexture = property.GetSrcObject<FbxLayeredTexture>(0);
            
            fileTexture = layeredTexture->GetSrcObject<FbxFileTexture>(0);
            this->addTextureToMaterial(fileTexture, mat);
            
            GP_WARNING(WARN_LAYERED_TEXTURES_NOT_SUPPORTED_USING_FIRST_TEXTURE_ONLY, meshPart->getId().c_str());
        }
    }
}

void FBXSceneEncoder::addTextureToMaterial(FbxFileTexture* fbxFileTexture, const Material* mat)
{
    FilepathUtils::setTexturePaths(fbxFileTexture->GetRelativeFileName(), EncoderArguments::getInstance()->getFilePath(), mat->getEffect());
        
    if(!mat->getEffect().isPowerOfTwo())
    {
        mat->getEffect().setWrapS(Effect::CLAMP);
        mat->getEffect().setWrapT(Effect::CLAMP);
        mat->getEffect().setMinFilter(Effect::LINEAR);
        mat->getEffect().setMagFilter(Effect::LINEAR);
        GP_WARNING(WARN_TEXTURES_NONPOWER_OF_2, mat->getEffect().getTextureSourcePath().c_str());
        if(fbxFileTexture->GetWrapModeU() != fbxsdk_2013_3::FbxTexture::eClamp || fbxFileTexture->GetWrapModeV() != fbxsdk_2013_3::FbxTexture::eClamp)
        {
            GP_ERROR(ERR_NO_CLAMP_FOR_NOP_USED, "");
        }
    }
    else
    {
        switch(fbxFileTexture->GetWrapModeU())
        {
            case fbxsdk_2013_3::FbxTexture::eRepeat:
                mat->getEffect().setWrapS(Effect::REPEAT);
                break;
            case fbxsdk_2013_3::FbxTexture::eClamp:
            default:
                mat->getEffect().setWrapS(Effect::CLAMP);
                break;
        }
        switch(fbxFileTexture->GetWrapModeV())
        {
            case fbxsdk_2013_3::FbxTexture::eRepeat:
                mat->getEffect().setWrapT(Effect::REPEAT);
                break;
            case fbxsdk_2013_3::FbxTexture::eClamp:
            default:
                mat->getEffect().setWrapT(Effect::CLAMP);
                break;
        }
    }
}


Material* FBXSceneEncoder::getMaterial(std::string materialId)
{
    // TODO: optimize search because complexity O(n) is too high
    std::list<Material*>::iterator it;
    for (it = _materials.begin(); it != _materials.end(); ++it)
    {
        if((*it)->getMaterialId().compare(materialId) == 0)
        {
            return (*it);
        }
    }
    return NULL;
}

Mesh* FBXSceneEncoder::loadMesh(FbxMesh* fbxMesh)
{
    // Check if this mesh has already been loaded.
    Mesh* mesh = getMesh(fbxMesh->GetUniqueID());
    if (mesh)
    {
        return mesh;
    }
    mesh = new Mesh();
    // GamePlay requires that a mesh have a unique ID but FbxMesh doesn't have a string ID.
    std::stringstream ss;
    ss << fbxMesh->GetNode()->GetUniqueID();
    const char* name = idStore.getId(fbxMesh->GetNode()->GetName(), ss.str()).c_str();
    if (name)
    {
        std::string id(name);
        id.append("_Mesh");
        mesh->setId(id);
    }

    // The number of mesh parts is equal to the number of materials that affect this mesh.
    // There is always at least one mesh part.
    std::vector<MeshPart*> meshParts;
    const int materialCount = fbxMesh->GetNode()->GetMaterialCount();
    int meshPartSize = (materialCount > 0) ? materialCount : 1;
    FbxNode* fbxNode = fbxMesh->GetNode();
    for (int i = 0; i < meshPartSize; i++)
    {
        MeshPart* meshPart = new MeshPart();
        meshParts.push_back(meshPart);
        FbxSurfaceMaterial* material = fbxNode->GetMaterial(i);
        if (material) {
            loadMaterial(mesh, meshPart, material);
        } else {
            GP_WARNING(WARN_USING_DEFAULT_MATERIAL, "");
            loadDefaultMaterial(mesh, meshPart);
        }
    }

    // Find the blend weights and blend indices if this mesh is skinned.
    std::vector<std::vector<Vector2> > weights;
    bool hasSkin = loadBlendWeights(fbxMesh, weights);
    
    // set skinning in material
    if(hasSkin)
    {
        const int matCount = fbxMesh->GetNode()->GetMaterialCount();
        meshPartSize = (matCount > 0) ? matCount : 1;
        FbxNode* fbxNodeSkin = fbxMesh->GetNode();
        for (int i = 0; i < meshPartSize; i++)
        {
            FbxSurfaceMaterial* material = fbxNodeSkin->GetMaterial(i);
            if (material) {
                char uniqueId[70];
                sprintf(uniqueId, "%llu", material->GetUniqueID());
                std::string materialId = std::string(uniqueId);
                Material* mat = getMaterial(materialId);
                if(mat != NULL) {
                    mat->setSkin(true);
                }
            }
        }
    }
    
    // Get list of uv sets for mesh
    FbxStringList uvSetNameList;
    fbxMesh->GetUVSetNames(uvSetNameList);
    const int uvSetCount = uvSetNameList.GetCount();

    int vertexIndex = 0;
    FbxVector4* controlPoints = fbxMesh->GetControlPoints();
    const int polygonCount = fbxMesh->GetPolygonCount();
    for (int polyIndex = 0; polyIndex < polygonCount; ++polyIndex)
    {
        const int polygonSize = fbxMesh->GetPolygonSize(polyIndex);
        for (int posInPoly = 0; posInPoly < polygonSize; ++posInPoly)
        {
            int controlPointIndex = fbxMesh->GetPolygonVertex(polyIndex, posInPoly);
            Vertex vertex;

            FbxVector4& position = controlPoints[controlPointIndex];
            vertex.position.x = (float)position[0];
            vertex.position.y = (float)position[1];
            vertex.position.z = (float)position[2];

            // Load tex coords for all uv sets
            for (int uvSetIndex = 0; uvSetIndex < uvSetCount; ++uvSetIndex)
            {
                const FbxGeometryElementUV* uvElement = fbxMesh->GetElementUV(uvSetNameList.GetStringAt(uvSetIndex));
                if (uvElement)
                    loadTextureCoords(fbxMesh, uvElement, uvSetIndex, polyIndex, posInPoly, vertexIndex, &vertex);
            }

            // Load other data
            loadNormal(fbxMesh, vertexIndex, controlPointIndex, &vertex);
            loadTangent(fbxMesh, vertexIndex, controlPointIndex, &vertex);
            loadBinormal(fbxMesh, vertexIndex, controlPointIndex, &vertex);
            loadVertexColor(fbxMesh, vertexIndex, controlPointIndex, &vertex);

            if (hasSkin)
            {
                loadBlendData(weights[controlPointIndex], &vertex);
            }

            // Determine which mesh part this vertex index should be added to based on the material that affects it.
            int meshPartIndex = 0;
            const int elementMatrialCount = fbxMesh->GetElementMaterialCount();
            for (int k = 0; k < elementMatrialCount; ++k)
            {
                FbxGeometryElementMaterial* elementMaterial = fbxMesh->GetElementMaterial(k);
                meshPartIndex = elementMaterial->GetIndexArray().GetAt(polyIndex);
            }

            // Add the vertex to the mesh if it hasn't already been added and find the vertex index.
            unsigned int index = 0;
            if (mesh->contains(vertex))
            {
                index = mesh->getVertexIndex(vertex);
            }
            else
            {
                index = mesh->addVertex(vertex);
            }

            if (meshPartIndex < 0) meshPartIndex = 0;

            MeshPart* p = meshParts.at(meshPartIndex);
            p->addIndex(index);
            vertexIndex++;
        }
    }

    const size_t meshpartsSize = meshParts.size();
    for (size_t i = 0; i < meshpartsSize; ++i)
    {
        mesh->addMeshPart(meshParts[i]);
    }

    // The order that the vertex elements are add to the list matters.
    // It should be the same order as how the Vertex data is written.

    // Position
    mesh->addVetexAttribute(POSITION, Vertex::POSITION_COUNT);

    int verticesCount = mesh->vertices.size();
    if (verticesCount > 0) {
        const Vertex& vertex = mesh->vertices[0];
        // Normals
        if (vertex.hasNormal)
        {
            mesh->addVetexAttribute(NORMAL, Vertex::NORMAL_COUNT);
        }
        // Tangents
        if (vertex.hasTangent)
        {
            mesh->addVetexAttribute(TANGENT, Vertex::TANGENT_COUNT);
        }
        // Binormals
        if (vertex.hasBinormal)
        {
            mesh->addVetexAttribute(BINORMAL, Vertex::BINORMAL_COUNT);
        }
        // Texture Coordinates
        for (unsigned int i = 0; i < MAX_UV_SETS; ++i)
        {
            if (vertex.hasTexCoord[i])
            {
                mesh->addVetexAttribute(TEXCOORD0 + i, Vertex::TEXCOORD_COUNT);
            }
        }
        // Diffuse Color
        if (vertex.hasDiffuse)
        {
            mesh->addVetexAttribute(COLOR, Vertex::DIFFUSE_COUNT);
        }
        // Skinning BlendWeights BlendIndices
        if (vertex.hasWeights)
        {
            mesh->addVetexAttribute(BLENDWEIGHTS, Vertex::BLEND_WEIGHTS_COUNT);
            mesh->addVetexAttribute(BLENDINDICES, Vertex::BLEND_INDICES_COUNT);
        }
    }

    _gamePlayFile.addMesh(mesh);
    saveMesh(fbxMesh->GetUniqueID(), mesh);
    return mesh;
}

void FBXSceneEncoder::triangulateRecursive(FbxNode* fbxNode)
{
    // Triangulate all NURBS, patch and mesh under this node recursively.
    FbxNodeAttribute* nodeAttribute = fbxNode->GetNodeAttribute();

    if (nodeAttribute)
    {
        if (nodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh ||
            nodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbs ||
            nodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbsSurface ||
            nodeAttribute->GetAttributeType() == FbxNodeAttribute::ePatch)
        {
            FbxGeometryConverter converter(fbxNode->GetFbxManager());
            converter.TriangulateInPlace(fbxNode);
        }
    }

    const int childCount = fbxNode->GetChildCount();
    for (int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        triangulateRecursive(fbxNode->GetChild(childIndex));
    }
}

////////////////////////////////////
// Functions
////////////////////////////////////

float getAspectRatio(FbxCamera* fbxCamera)
{
    return (float)fbxCamera->FilmAspectRatio.Get();
    /*
    FbxCamera::ECameraAspectRatioMode camAspectRatioMode = fbxCamera->GetAspectRatioMode();
    double aspectX = fbxCamera->AspectWidth.Get();
    double aspectY = fbxCamera->AspectHeight.Get();
    double aspectRatio = 1.333333;
    switch ( camAspectRatioMode)
    {
    case FbxCamera::eWINDOW_SIZE:
        aspectRatio = aspectX / aspectY;
        break;
    case FbxCamera::eFIXED_RATIO:
        aspectRatio = aspectX;
        break;
    case FbxCamera::eFIXED_RESOLUTION:
        aspectRatio = aspectX / aspectY * fbxCamera->GetPixelRatio();
        break;
    case FbxCamera::eFIXED_WIDTH:
        aspectRatio = fbxCamera->GetPixelRatio() / aspectY;
        break;
    case FbxCamera::eFIXED_HEIGHT:
        aspectRatio = fbxCamera->GetPixelRatio() * aspectX;
        break;
    default:
        break;
    }
    return (float)aspectRatio;
    */
}

inline double vfov(double hfov, double aspect)
{
    static const double MATH_PI_180 = 0.01745329251994329576923690768489;
    static const double MATH_180_PI = 57.295779513082320876798154814105;
    return (2.0 * atan((aspect) * tan( (hfov * MATH_PI_180) * 0.5)) * MATH_180_PI);
}

float getFieldOfView(FbxCamera* fbxCamera)
{
    double fieldOfViewX = 0.0;
    double fieldOfViewY = 0.0;
    double filmHeight = fbxCamera->GetApertureHeight();
    double filmWidth = fbxCamera->GetApertureWidth() * fbxCamera->GetSqueezeRatio();
    double apertureRatio = filmHeight / filmWidth;
    if ( fbxCamera->GetApertureMode() == FbxCamera::eVertical)
    {
        fieldOfViewY = fbxCamera->FieldOfView.Get();
    }
    else if (fbxCamera->GetApertureMode() == FbxCamera::eHorizontal)
    {
        fieldOfViewX = fbxCamera->FieldOfView.Get();
        fieldOfViewY = vfov( fieldOfViewX, apertureRatio);
    }
    else if (fbxCamera->GetApertureMode() == FbxCamera::eFocalLength)
    {
        fieldOfViewX = fbxCamera->ComputeFieldOfView(fbxCamera->FocalLength.Get());
        fieldOfViewY = vfov( fieldOfViewX, apertureRatio);
    }
    else if (fbxCamera->GetApertureMode() == FbxCamera::eHorizAndVert)
    {
        fieldOfViewY = fbxCamera->FieldOfViewY.Get();
    }
    else
    {
        fieldOfViewY = 45.0;
    }
    return (float)fieldOfViewY;
}

void loadTextureCoords(FbxMesh* fbxMesh, const FbxGeometryElementUV* uvs, int uvSetIndex, int polyIndex, int posInPoly, int meshVertexIndex, Vertex* vertex)
{
    assert(fbxMesh && polyIndex >=0 && posInPoly >= 0);

    const bool useIndex = uvs->GetReferenceMode() != FbxGeometryElement::eDirect;
    const int indexCount = useIndex ? uvs->GetIndexArray().GetCount() : 0;
    int uvIndex = -1;

    switch (uvs->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint:
        {
            // Get the index of the current vertex in control points array
            int polyVertIndex = fbxMesh->GetPolygonVertex(polyIndex, posInPoly);

            // The UV index depends on the reference mode
            uvIndex = useIndex ? uvs->GetIndexArray().GetAt(polyVertIndex) : polyVertIndex;
        }
        break;

    case FbxGeometryElement::eByPolygonVertex:
        if (meshVertexIndex < indexCount)
        {
            uvIndex = useIndex ? uvs->GetIndexArray().GetAt(meshVertexIndex) : meshVertexIndex;
        }
        break;

    default:
        // Only support eByPolygonVertex and eByControlPoint mappings
        break;
    }

    vertex->hasTexCoord[uvSetIndex] = true;

    // Store UV information in vertex
    if (uvIndex != -1)
    {
        FbxVector2 uvValue = uvs->GetDirectArray().GetAt(uvIndex);
        vertex->texCoord[uvSetIndex].x = (float)uvValue[0];
        vertex->texCoord[uvSetIndex].y = (float)uvValue[1];
    }
}

void loadNormal(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex)
{
    if (fbxMesh->GetElementNormalCount() > 0)
    {
        // Get only the first
        FbxGeometryElementNormal* normal = fbxMesh->GetElementNormal(0);
        FbxGeometryElement::EMappingMode mappingMode = normal->GetMappingMode();
        if (mappingMode == FbxGeometryElement::eByControlPoint)
        {
            switch (normal->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxVector4 vec4 = normal->GetDirectArray().GetAt(controlPointIndex);
                    vertex->hasNormal = true;
                    vertex->normal.x = (float)vec4[0];
                    vertex->normal.y = (float)vec4[1];
                    vertex->normal.z = (float)vec4[2];
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = normal->GetIndexArray().GetAt(controlPointIndex);
                    FbxVector4 vec4 = normal->GetDirectArray().GetAt(id);
                    vertex->hasNormal = true;
                    vertex->normal.x = (float)vec4[0];
                    vertex->normal.y = (float)vec4[1];
                    vertex->normal.z = (float)vec4[2];
                }
                break;
            default:
                break;
            }
        }
        else if (mappingMode == FbxGeometryElement::eByPolygonVertex)
        {
            switch (normal->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxVector4 vec4 = normal->GetDirectArray().GetAt(vertexIndex);
                    vertex->hasNormal = true;
                    vertex->normal.x = (float)vec4[0];
                    vertex->normal.y = (float)vec4[1];
                    vertex->normal.z = (float)vec4[2];
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = normal->GetIndexArray().GetAt(vertexIndex);
                    FbxVector4 vec4 = normal->GetDirectArray().GetAt(id);
                    vertex->hasNormal = true;
                    vertex->normal.x = (float)vec4[0];
                    vertex->normal.y = (float)vec4[1];
                    vertex->normal.z = (float)vec4[2];
                }
                break;
            default:
                break;
            }
        }
    }
}

void loadTangent(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex)
{
    if (fbxMesh->GetElementTangentCount() > 0)
    {
        // Get only the first tangent
        FbxGeometryElementTangent* tangent = fbxMesh->GetElementTangent(0);
        FbxGeometryElement::EMappingMode mappingMode = tangent->GetMappingMode();
        if (mappingMode == FbxGeometryElement::eByControlPoint)
        {
            switch (tangent->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxVector4 vec4 = tangent->GetDirectArray().GetAt(controlPointIndex);
                    vertex->hasTangent = true;
                    vertex->tangent.x = (float)vec4[0];
                    vertex->tangent.y = (float)vec4[1];
                    vertex->tangent.z = (float)vec4[2];
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = tangent->GetIndexArray().GetAt(controlPointIndex);
                    FbxVector4 vec4 = tangent->GetDirectArray().GetAt(id);
                    vertex->hasTangent = true;
                    vertex->tangent.x = (float)vec4[0];
                    vertex->tangent.y = (float)vec4[1];
                    vertex->tangent.z = (float)vec4[2];
                }
                break;
            default:
                break;
            }
        }
        else if (mappingMode == FbxGeometryElement::eByPolygonVertex)
        {
            switch (tangent->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxVector4 vec4 = tangent->GetDirectArray().GetAt(vertexIndex);
                    vertex->hasTangent = true;
                    vertex->tangent.x = (float)vec4[0];
                    vertex->tangent.y = (float)vec4[1];
                    vertex->tangent.z = (float)vec4[2];
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = tangent->GetIndexArray().GetAt(vertexIndex);
                    FbxVector4 vec4 = tangent->GetDirectArray().GetAt(id);
                    vertex->hasTangent = true;
                    vertex->tangent.x = (float)vec4[0];
                    vertex->tangent.y = (float)vec4[1];
                    vertex->tangent.z = (float)vec4[2];
                }
                break;
            default:
                break;
            }
        }
    }
}

void loadBinormal(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex)
{
    if (fbxMesh->GetElementBinormalCount() > 0)
    {
        // Get only the first binormal.
        FbxGeometryElementBinormal* binormal = fbxMesh->GetElementBinormal(0);
        FbxGeometryElement::EMappingMode mappingMode = binormal->GetMappingMode();

        if (mappingMode == FbxGeometryElement::eByControlPoint)
        {
            switch (binormal->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxVector4 vec4 = binormal->GetDirectArray().GetAt(controlPointIndex);
                    vertex->hasBinormal = true;
                    vertex->binormal.x = (float)vec4[0];
                    vertex->binormal.y = (float)vec4[1];
                    vertex->binormal.z = (float)vec4[2];
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = binormal->GetIndexArray().GetAt(controlPointIndex);
                    FbxVector4 vec4 = binormal->GetDirectArray().GetAt(id);
                    vertex->hasBinormal = true;
                    vertex->binormal.x = (float)vec4[0];
                    vertex->binormal.y = (float)vec4[1];
                    vertex->binormal.z = (float)vec4[2];
                }
                break;
            default:
                break;
            }
        }
        else if (mappingMode == FbxGeometryElement::eByPolygonVertex)
        {
            switch (binormal->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxVector4 vec4 = binormal->GetDirectArray().GetAt(vertexIndex);
                    vertex->hasBinormal = true;
                    vertex->binormal.x = (float)vec4[0];
                    vertex->binormal.y = (float)vec4[1];
                    vertex->binormal.z = (float)vec4[2];
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = binormal->GetIndexArray().GetAt(vertexIndex);
                    FbxVector4 vec4 = binormal->GetDirectArray().GetAt(id);
                    vertex->hasBinormal = true;
                    vertex->binormal.x = (float)vec4[0];
                    vertex->binormal.y = (float)vec4[1];
                    vertex->binormal.z = (float)vec4[2];
                }
                break;
            default:
                break;
            }
        }
    }
}

void loadVertexColor(FbxMesh* fbxMesh, int vertexIndex, int controlPointIndex, Vertex* vertex)
{
    if (fbxMesh->GetElementVertexColorCount() > 0)
    {
        // Get only the first vertex color.
        FbxGeometryElementVertexColor* vertexColor = fbxMesh->GetElementVertexColor(0);
        FbxGeometryElement::EMappingMode mappingMode = vertexColor->GetMappingMode();
        if (mappingMode == FbxGeometryElement::eByControlPoint)
        {
            switch (vertexColor->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxColor color = vertexColor->GetDirectArray().GetAt(controlPointIndex);

                    vertex->hasDiffuse = true;
                    vertex->diffuse.x = (float)color.mRed;
                    vertex->diffuse.y = (float)color.mGreen;
                    vertex->diffuse.z = (float)color.mBlue;
                    vertex->diffuse.w = (float)color.mAlpha;
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = vertexColor->GetIndexArray().GetAt(controlPointIndex);
                    FbxColor color = vertexColor->GetDirectArray().GetAt(id);

                    vertex->hasDiffuse = true;
                    vertex->diffuse.x = (float)color.mRed;
                    vertex->diffuse.y = (float)color.mGreen;
                    vertex->diffuse.z = (float)color.mBlue;
                    vertex->diffuse.w = (float)color.mAlpha;
                }
                break;
            default:
                break;
            }
        }
        else if (mappingMode == FbxGeometryElement::eByPolygonVertex)
        {
            switch (vertexColor->GetReferenceMode())
            {
            case FbxGeometryElement::eDirect:
                {
                    FbxColor color = vertexColor->GetDirectArray().GetAt(vertexIndex);

                    vertex->hasDiffuse = true;
                    vertex->diffuse.x = (float)color.mRed;
                    vertex->diffuse.y = (float)color.mGreen;
                    vertex->diffuse.z = (float)color.mBlue;
                    vertex->diffuse.w = (float)color.mAlpha;
                }
                break;
            case FbxGeometryElement::eIndexToDirect:
                {
                    int id = vertexColor->GetIndexArray().GetAt(vertexIndex);
                    FbxColor color = vertexColor->GetDirectArray().GetAt(id);

                    vertex->hasDiffuse = true;
                    vertex->diffuse.x = (float)color.mRed;
                    vertex->diffuse.y = (float)color.mGreen;
                    vertex->diffuse.z = (float)color.mBlue;
                    vertex->diffuse.w = (float)color.mAlpha;
                }
                break;
            default:
                break;
            }
        }
    }
}

void loadBlendData(const std::vector<Vector2>& vertexWeights, Vertex* vertex)
{
    size_t size = vertexWeights.size();

    if (size >= 1)
    {
        vertex->hasWeights= true;
        vertex->blendIndices.x = vertexWeights[0].x;
        vertex->blendWeights.x = vertexWeights[0].y;
    }
    if (size >= 2)
    {
        vertex->blendIndices.y = vertexWeights[1].x;
        vertex->blendWeights.y = vertexWeights[1].y;
    }
    if (size >= 3)
    {
        vertex->blendIndices.z = vertexWeights[2].x;
        vertex->blendWeights.z = vertexWeights[2].y;
    }
    if (size >= 4)
    {
        vertex->blendIndices.w = vertexWeights[3].x;
        vertex->blendWeights.w = vertexWeights[3].y;
    }
    //vertex->normalizeBlendWeight();
}

bool loadBlendWeights(FbxMesh* fbxMesh, std::vector<std::vector<Vector2> >& weights)
{
    assert(fbxMesh);
    const int vertexCount = fbxMesh->GetControlPointsCount();

    FbxSkin* fbxSkin = NULL;
    const int deformerCount = fbxMesh->GetDeformerCount();
    for (int i = 0; i < deformerCount; ++i)
    {
        FbxDeformer* deformer = fbxMesh->GetDeformer(i);
        if (deformer->GetDeformerType() == FbxDeformer::eSkin)
        {
            fbxSkin = static_cast<FbxSkin*>(deformer);
            weights.resize(vertexCount);

            const int clusterCount = fbxSkin->GetClusterCount();
            for (int j = 0; j < clusterCount; ++j)
            {
                FbxCluster* cluster = fbxSkin->GetCluster(j);
                assert(cluster);
                const int vertexIndexCount = cluster->GetControlPointIndicesCount();
                for (int k = 0; k < vertexIndexCount; ++k)
                {
                    int index = cluster->GetControlPointIndices()[k];
                    if (index >= vertexCount)
                    {
                        continue;
                    }

                    double weight = cluster->GetControlPointWeights()[k];
                    if (weight == 0.0)
                    {
                        continue;
                    }
                    weights[index].push_back(Vector2((float)j, (float)weight));
                }
            }
            // Only the first skin deformer will be loaded.
            // There probably won't be more than one.
            break;
        }
    }
    return fbxSkin != NULL;
}

void findMinMaxTime(FbxAnimCurve* animCurve, float* startTime, float* stopTime, float* frameRate)
{
    FbxTime start, stop;
    FbxTimeSpan timeSpan;
    animCurve->GetTimeInterval(timeSpan);
    start = timeSpan.GetStart();
    stop = timeSpan.GetStop();
    *startTime = std::min(*startTime, (float)start.GetMilliSeconds());
    *stopTime = std::max(*stopTime, (float)stop.GetMilliSeconds());
    *frameRate = std::max(*frameRate, (float)stop.GetFrameRate(FbxTime::eDefaultMode));
}

void appendKeyFrame(FbxNode* fbxNode, AnimationChannel* channel, float time, const Vector3& scale, const Quaternion& rotation, const Vector3& translation)
{
    // Write key time
    channel->getKeyTimes().push_back(time);

    // Write key values
    std::vector<float>& keyValues = channel->getKeyValues();
    switch (channel->getTargetAttribute())
    {
        case Transform::ANIMATE_SCALE:
        {
            keyValues.push_back(scale.x);
            keyValues.push_back(scale.y);
            keyValues.push_back(scale.z);
        }
        break;

        case Transform::ANIMATE_SCALE_X:
        {
            keyValues.push_back(scale.x);
        }
        break;

        case Transform::ANIMATE_SCALE_Y:
        {
            keyValues.push_back(scale.y);
        }
        break;

        case Transform::ANIMATE_SCALE_Z:
        {
            keyValues.push_back(scale.z);
        }
        break;

        case Transform::ANIMATE_ROTATE:
        {
            keyValues.push_back(rotation.x);
            keyValues.push_back(rotation.y);
            keyValues.push_back(rotation.z);
            keyValues.push_back(rotation.w);
        }
        break;

        case Transform::ANIMATE_TRANSLATE:
        {
            keyValues.push_back(translation.x);
            keyValues.push_back(translation.y);
            keyValues.push_back(translation.z);
        }
        break;

        case Transform::ANIMATE_TRANSLATE_X:
        {
            keyValues.push_back(translation.x);
        }
        break;

        case Transform::ANIMATE_TRANSLATE_Y:
        {
            keyValues.push_back(translation.y);
        }
        break;

        case Transform::ANIMATE_TRANSLATE_Z:
        {
            keyValues.push_back(translation.z);
        }
        break;

        case Transform::ANIMATE_ROTATE_TRANSLATE:
        {
            keyValues.push_back(rotation.x);
            keyValues.push_back(rotation.y);
            keyValues.push_back(rotation.z);
            keyValues.push_back(rotation.w);
            keyValues.push_back(translation.x);
            keyValues.push_back(translation.y);
            keyValues.push_back(translation.z);
        }
        break;

        case Transform::ANIMATE_SCALE_ROTATE_TRANSLATE:
        {
            keyValues.push_back(scale.x);
            keyValues.push_back(scale.y);
            keyValues.push_back(scale.z);
            keyValues.push_back(rotation.x);
            keyValues.push_back(rotation.y);
            keyValues.push_back(rotation.z);
            keyValues.push_back(rotation.w);
            keyValues.push_back(translation.x);
            keyValues.push_back(translation.y);
            keyValues.push_back(translation.z);
        }
        break;

        case Transform::ANIMATE_SCALE_TRANSLATE:
        {
            keyValues.push_back(scale.x);
            keyValues.push_back(scale.y);
            keyValues.push_back(scale.z);
            keyValues.push_back(translation.x);
            keyValues.push_back(translation.y);
            keyValues.push_back(translation.z);
        }
        break;

        case Transform::ANIMATE_SCALE_ROTATE:
        {
            keyValues.push_back(scale.x);
            keyValues.push_back(scale.y);
            keyValues.push_back(scale.z);
            keyValues.push_back(rotation.x);
            keyValues.push_back(rotation.y);
            keyValues.push_back(rotation.z);
            keyValues.push_back(rotation.w);
        }
        break;

        default:
        {
            GP_WARNING(WARN_INVALID_ANIMATION_TARGET, channel->getTargetAttribute(), fbxNode->GetName());
        }
        return;
    }
}

void decompose(FbxNode* fbxNode, float time, Vector3* scale, Quaternion* rotation, Vector3* translation)
{
    FbxAMatrix fbxMatrix;
    Matrix matrix;
    FbxTime kTime;
    kTime.SetMilliSeconds((FbxLongLong)time);
    fbxMatrix = fbxNode->EvaluateLocalTransform(kTime);
    copyMatrix(fbxMatrix, matrix);
    matrix.decompose(scale, rotation, translation);
}

AnimationChannel* createAnimationChannel(FbxNode* fbxNode, unsigned int targetAttrib, const std::vector<float>& keyTimes, const std::vector<float>& keyValues)
{
    AnimationChannel* channel = new AnimationChannel();
    std::stringstream ss;
    ss << fbxNode->GetUniqueID();
    const char* name = idStore.getId(fbxNode->GetName(), ss.str()).c_str();
    channel->setTargetId(name);
    channel->setKeyTimes(keyTimes);
    channel->setKeyValues(keyValues);
    channel->setInterpolation(AnimationChannel::LINEAR);
    channel->setTargetAttribute(targetAttrib);
    return channel;
}

void addScaleChannel(Animation* animation, FbxNode* fbxNode, float startTime, float stopTime)
{
    std::vector<float> keyTimes;
    std::vector<float> keyValues;
    Vector3 scale;
    Quaternion rotation;
    Vector3 translation;

    decompose(fbxNode, startTime, &scale, &rotation, &translation);
    keyTimes.push_back(startTime);
    keyValues.push_back(scale.x);
    keyValues.push_back(scale.y);
    keyValues.push_back(scale.z);

    decompose(fbxNode, stopTime, &scale, &rotation, &translation);
    keyTimes.push_back(stopTime);
    keyValues.push_back(scale.x);
    keyValues.push_back(scale.y);
    keyValues.push_back(scale.z);

    AnimationChannel* channel = createAnimationChannel(fbxNode, Transform::ANIMATE_SCALE, keyTimes, keyValues);
    animation->add(channel);
}

void addTranslateChannel(Animation* animation, FbxNode* fbxNode, float startTime, float stopTime)
{
    std::vector<float> keyTimes;
    std::vector<float> keyValues;
    Vector3 scale;
    Quaternion rotation;
    Vector3 translation;

    decompose(fbxNode, startTime, &scale, &rotation, &translation);
    keyTimes.push_back(startTime);
    keyValues.push_back(translation.x);
    keyValues.push_back(translation.y);
    keyValues.push_back(translation.z);

    decompose(fbxNode, stopTime, &scale, &rotation, &translation);
    keyTimes.push_back(stopTime);
    keyValues.push_back(translation.x);
    keyValues.push_back(translation.y);
    keyValues.push_back(translation.z);

    AnimationChannel* channel = createAnimationChannel(fbxNode, Transform::ANIMATE_TRANSLATE, keyTimes, keyValues);
    animation->add(channel);
}

void copyMatrix(const FbxMatrix& fbxMatrix, float* matrix)
{
    int i = 0;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            matrix[i++] = (float)fbxMatrix.Get(row, col);
        }
    }
}

void copyMatrix(const FbxMatrix& fbxMatrix, Matrix& matrix)
{
    int i = 0;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            matrix.m[i++] = (float)fbxMatrix.Get(row, col);
        }
    }
}

bool isGroupAnimationPossible(FbxScene* fbxScene)
{
    FbxNode* rootNode = fbxScene->GetRootNode();
    if (rootNode)
    {
        if (isGroupAnimationPossible(rootNode))
            return true;
    }
    return false;
}

bool isGroupAnimationPossible(FbxNode* fbxNode)
{
    if (fbxNode)
    {
        FbxMesh* fbxMesh = fbxNode->GetMesh();
        if (isGroupAnimationPossible(fbxMesh))
            return true;
        const int childCount = fbxNode->GetChildCount();
        for (int i = 0; i < childCount; ++i)
        {
            if (isGroupAnimationPossible(fbxNode->GetChild(i)))
                return true;
        }
    }
    return false;
}

bool isGroupAnimationPossible(FbxMesh* fbxMesh)
{
    if (fbxMesh)
    {
        const int deformerCount = fbxMesh->GetDeformerCount();
        for (int i = 0; i < deformerCount; ++i)
        {
            FbxDeformer* deformer = fbxMesh->GetDeformer(i);
            if (deformer->GetDeformerType() == FbxDeformer::eSkin)
            {
                FbxSkin* fbxSkin = static_cast<FbxSkin*>(deformer);
                if (fbxSkin)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

#endif
