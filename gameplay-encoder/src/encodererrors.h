#if defined(ERROR_BUILD_ARRAY)

#undef ERROR_START
#undef ERROR_END
#undef ERRDEF

struct err_defn {
    int value;
    const char* name;
    const char* desc;
};

#define ERROR_START \
        static const err_defn error_table[] = {
#define ERRDEF(name, num, str, desc) { name, str, desc },
#define ERROR_END { 0, 0, 0 } };

#elif !defined(ERROR_ENUM_DEFINED)

#define ERROR_START \
        typedef enum encoder_errno_t {
#define ERRDEF(name, num, str, desc) /** str */ name = num,
#define ERROR_END ERR_LAST } encoder_errno_t;

#define ERROR_ENUM_DEFINED
#endif

void encoderErr2msg(int code, const char** name, const char** desc);

////////////////////////////////////////
// Define new error and warnings here //
////////////////////////////////////////

#define ERR_NUM 1000

#define WARN_NUM 2000 // start at error number for warnings at 2000

#define INFO_NUM 3000

#define ERR_NUM_INC ERR_NUM +1
#define WARN_NUM_INC WARN_NUM +1

ERROR_START

// ERRORS
ERRDEF(ERR_ONLY_PNG_SUPPORTED, 1001, "Only PNG Supported", "Encoder can handle only png's. Please use png's and export your file again (%s)")
ERRDEF(ERR_MATERIAL, 1002, "Error in Material", "Error occurred in Material: %s")

ERRDEF(ERR_NO_TEXTURE_SUPPORT_IN_MATERIAL, 1003, "No <texture> Support", "Currently no support for <texture> in %s channel")

// collada
ERRDEF(ERR_COLLADA_DOM, 1004, "COLLADA dom error", "COLLADA failed to write the dom for file %s")
ERRDEF(ERR_COLLADA_OPEN_FILE, 1005, "COLLADA file error", "COLLADA failed to open file %s")
ERRDEF(ERR_COLLADA_NODE_FAILED, 1006, "COLLADA node error", "COLLADA file loaded to the dom, but failed to load node %s")
ERRDEF(ERR_COLLADA_NODE_NOT_FOUND, 1007, "COLLADA node id error", "COLLADA file loaded to the dom, but node was not found with node ID %s")
ERRDEF(ERR_COLLADA_QUERY_FAILED, 1008, "COLLADA query failed", "COLLADA file loaded to the dom, but query for the dom assets failed")
ERRDEF(ERR_COLLADA_MISSING_VSCENE, 1009, "COLLADA missing <visual_scene>", "COLLADA file loaded to the dom, but missing <visual_scene>")

ERRDEF(ERR_WRITING_TEXT_FILE, 1010, "Error writing text file", "Error writing text file: %s")
ERRDEF(ERR_WRITING_BINARY_FILE, 1011, "Error writing binary file", "Error writing binary file: %s")

ERRDEF(ERR_RESOLVING_GEOMETRY_URL, 1012, "Error resolving geometry url", "Failed to resolve geometry uri: %s")

ERRDEF(ERR_FBX_IMPORTER_NOT_INITIALIZED, 1013, "FbxImporter not initialized", "Call to FbxImporter::Initialize() failed for path: %s.")
ERRDEF(ERR_FBX_ERROR_MESSAGE, 1014, "Fbx Error", "Last Fbx error returned: %s")


ERRDEF(ERR_FAILED_TO_OPEN_WRITE_FILE, 1015, "Failed to open file", "Failed to open file for writing: %s")
ERRDEF(ERR_WRITE_STRUCT_FAILED, 1016, "Failed to write struct", "PNG Write struct creation failed: %s")
ERRDEF(ERR_FAILED_TO_WRITE_INFO_STRUCT, 1017, "Failed to create info struct", "Failed to create PNG info struct: %s")

ERRDEF(ERR_FILE_NOT_FOUND, 1018, "File not found", "File not found: %s")
ERRDEF(ERR_UNSUPPORTED_FILE_FORMAT, 1019, "Unsupported file format", "Unsupported file format: %s")

ERRDEF(ERR_FBX_NOT_ENABLED, 1020, "FBX not enabled", "Install the FBX SDK and use the preprocessor definition USE_FBX %s.")

ERRDEF(ERR_INIT_TTF_LIBRARY, 1021, "TTF Library not initialized", "Error initializing TTF Library: %d")
ERRDEF(ERR_INIT_FONT_FACE, 1022, "Font face not initialized", "Error initializing font face: %d")
ERRDEF(ERR_SET_CHAR_SIZE, 1023, "Char Size not set", "Error setting char size: %d")
ERRDEF(ERR_LOAD_CHAR, 1024, "Char not loaded", "Error loading char: %d")

ERRDEF(ERR_IMAGE_SIZE_EXCEEDED, 1025, "Image size exceeded!", "Image size exceeded: %d")
ERRDEF(ERR_DIRECTORY_DOES_NOT_EXIST, 1026, "Directory does not exist", "The specified directory does not exist or is not wirtable: %s")
ERRDEF(ERR_TEX_COPY, 1027, "Could not copy textures!", "Could not copy textures: src=%s dest=%s - error: %s")

ERRDEF(ERR_FILE_NOT_COPIED, 1028, "File not copied", "Error: %d. The specified file could not be copied into the destination directory: %s")
ERRDEF(ERR_CONVERT_JPG_PNG, 1029, "Error at conversion from jpg to png", "File could not be converted from jpg to png. jpg: %s png: %s")


// WARNINGS
ERRDEF(WARN_DUPLICATE_KEYFRAMES, 2001, "Duplicate Keyframes", "Removed %ld duplicate keyframes from channel.\n")

ERRDEF(WARN_DELETE_RANGE, 2002, "Delete Range", "Deleting Range: %s")



ERRDEF(WARN_TRANSFORM_ROTATE_NOT_SUPPORTED, 2005, "Could not apply rotation", "Node %s: \"Rotate\" transform found but not supported.\n Use \"Bake Transform\" options when exporting")
ERRDEF(WARN_TRANSFORM_SCALE_NOT_SUPPORTED, 2006, "Could not apply rotation", "Node %s: \"Scale\" transform found but not supported.\n Use \"Bake Transform\" options when exporting")
ERRDEF(WARN_TRANSFORM_TRANSLATE_NOT_SUPPORTED, 2007, "Could not apply rotation", "Node %s: \"Translate\" transform found but not supported.\n Use \"Bake Transform\" options when exporting")

ERRDEF(WARN_TRANSFORM_NOT_SUPPORTED, 2008, "Transform not supported", "%s transform found but not supported")

ERRDEF(WARN_MATERIAL_NOT_FOUND, 2009, "Material not found", "Couldnt find material: %s")
ERRDEF(WARN_JOINTS_NOT_FOUND, 2010, "Joints not found", "No joints found for skin: %s")
ERRDEF(WARN_MESH_NOT_FOUND, 2011, "Mesh not found", "No mesh found fro geometry: %s")
ERRDEF(WARN_TRIANGLES_NOT_FOUND, 2012, "Triangles not found", "Geometry mesh has no triangles: %s")

ERRDEF(WARN_UNSUPPORTED_VERTEX_SEMANTIC, 2013, "Vertex semantic is invalid/unsupported", "Vertex semantic (%s) is invalid/unsypported for geometry mesh: %s")
ERRDEF(WARN_UNSUPPORTED_SEMANTIC, 2014, "Semantic is invalid/unsupported", "Semantic (%s) is invalid/unsupported for geometry mesh: %s")

ERRDEF(WARN_UNEVEN_INPUT_SOURCES_FOR_TRIANGLES, 2015, "Uneven number of input sources", "Triangles do not all have the same number of input sources for geometry mesh: %s")

ERRDEF(WARN_GENERIC_FBX_LOG, 2016, "Generic Fbx message", "Fbx message: %s")

ERRDEF(WARN_UNKNOWN_CAMERA_TYPE, 2017, "Unknowm camera type", "Unknown camera type in node: %s")
ERRDEF(WARN_UNKNOWN_LIGHT_TYPE, 2018, "Unknowm light type", "Unknown light type in node: %s")

ERRDEF(WARN_INVALID_ANIMATION_TARGET, 2019, "Invalid animation target", "Invalid animation target (%d) attribute for node: %s")

ERRDEF(WARN_OPTIMIZING_CHANNELS_FOR_ANIMATION, 2020, "Optimizing animation channels", "Optimizing %d channel(s) in animation '%s'.")
ERRDEF(WARN_OPTIMIZING_ANIMATION_CHANNEL, 2021, "Optimizing animation channel", "Optimizing animation channel %s:%d")

ERRDEF(WARN_SCALE_CHANNEL, 2022, "Scale channel", "%s scale channel")
ERRDEF(WARN_ROTATION_CHANNEL, 2023, "Rotation channel", "%s scale channel")
ERRDEF(WARN_TRANSLATION_CHANNEL, 2024, "Translation channel", "%s scale channel")
ERRDEF(WARN_ROTATION_NOT_IDENTITY, 2025, "Rotation not identity", "Rotation not identity: %u")


ERRDEF(WARN_GENERATING_HEIGHTMAP, 2026, "Generating heightmap", "Generating heightmap: %s")

ERRDEF(WARN_HEIGHTMAP_NODE_HAS_NO_MESH, 2027, "Heightmap node has no mesh", "Node passed to heightmap argument does not have a mesh: %s")
ERRDEF(WARN_HEIGHTMAP_NODE_FAILED_TO_LOCATE, 2028, "Heightmap node not found for argument", "Failed to locate node for heightmap argument: %s")
ERRDEF(WARN_HEIGHTMAP_NO_NODES_FOUND, 2029, "Skipping heightmap generation", "Skipping generation of heightmap '%s'. No nodes found")
ERRDEF(WARN_HEIGHTMAP_FAILED_CREATING_WORKER_THREAD, 2030, "Failed to spawn thread", "Failed to spawn worker thread for generation of heightmap: %s")

ERRDEF(WARN_TRIANGLE_INTERSECTIONS_FAILED_FOR_HEIGHTMAP, 2031, "Triangle intersections failed", "%d triangle intersections failed for heightmap: %s")


ERRDEF(WARN_SPOTLIGHT_FALLOFF_EXPONENT, 2032, "Spot light falloff exponent", "Spot light falloff exponent must be 1.0: %f")

ERRDEF(WARN_SYMBOL_NOT_FOUND, 2033, "Symbol not found", "Symbol not found: %s")


ERRDEF(WARN_NO_MATERIAL_ASSIGNED_FOR_MESH, 2034, "Material not Assigned", "Mesh '%s' has no material assigned")

ERRDEF(WARN_MULTIPLE_MATERIALS_ASSIGNED_FOR_MESH, 2035, "Multiple Materials Assigned", "Mesh '%s' has multiple materials assigned -> using first assigned material.")


ERRDEF(WARN_NO_TEXTURE_FOUND_USING_FIRST_LAYERED_ONE, 2036, "No texture found", "Using first layered texture because no other texture was assigned to mesh '%s'.")
ERRDEF(WARN_LAYERED_TEXTURES_NOT_SUPPORTED_USING_FIRST_TEXTURE_ONLY, 2037, "Layered textures not supported", "Layered textures are generally not supported. Using only first texture for mesh '%s'.")

ERRDEF(WARN_TEXTURES_NONPOWER_OF_2, 2038, "Non power of two textures are used", "One of the textures has a non power of two width/height. Please change the size to power of two (eg. 512x512, 1024x1024). Texture-Path: %s")
ERRDEF(WARN_NORMALMAP_NOT_SUPPORTED, 2039, "Normal Maps not supported", "Normal maps are currently not supported")
ERRDEF(WARN_TEXTURE_NOT_FOUND, 2040, "Texture-file not found", "filepath:%s - error: %s\n")

// INOFS
ERRDEF(INFO_LOG, 3001, "Generic info", "log: %s")

ERRDEF(INFO_SAVE_DEBUG_FILE, 3002, "Saving debug file", "Saving debug file: %s")
ERRDEF(INFO_SAVE_BINARY_FILE, 3003, "Saving binary file", "Saving binary file: %s")
ERRDEF(INFO_TEXTURE_CONVERTED2PNG, 3004, "Texture was converted to png", "old: %s - new: %s\n")

ERROR_END
