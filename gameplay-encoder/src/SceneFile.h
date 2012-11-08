#ifndef SCENEFILE_H_
#define SCENEFILE_H_

#include "GPBFile.h"

namespace gameplay
{
/**
 * The SceneFile class handles writing the scene-file.
 */
class SceneFile
{
public:

    /**
     * Constructor.
     */
    SceneFile(GPBFile& gpbFile);

    /**
     * Destructor.
     */
    ~SceneFile(void);

    void writeFile(FILE* file);
private:
    GPBFile _gpbFile;
};

}

#endif
