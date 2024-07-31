#pragma once

#pragma once

#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include "Device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

// ERROR is already defined in wingdi.h and collides with a define in the Draco headers
#if defined(_WIN32) && defined(ERROR) && defined(TINYGLTF_ENABLE_DRACO) 
#undef ERROR
#pragma message ("ERROR constant already defined, undefining")
#endif

#define TINYGLTF_NO_STB_IMAGE_WRITE

#if defined(__ANDROID__)
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#include <android/asset_manager.h>
#endif

#define MAX_NUM_JOINTS 128u


class ModelGLTF
{

};

