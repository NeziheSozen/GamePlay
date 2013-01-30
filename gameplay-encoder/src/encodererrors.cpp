
#include "encodererrors.h"
#define ERROR_BUILD_ARRAY
#include "encodererrors.h"

#include <assert.h>

void encoderErr2msg(int code, const char** name, const char** desc)
{
    for (int i = 0; error_table[i].name; ++i)
        if (error_table[i].value == code) {
            
            *(name) = error_table[i].name;
            *(desc) = error_table[i].desc;
        }
}
