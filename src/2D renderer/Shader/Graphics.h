
#ifndef S_GRAPHICS
#define S_GRAPHICS

#include "Shader.h"

class GShader : public Shader
{
public:
	GShader();
	GShader(const char* vertex, const char* fragment, bool from_data = false);
};

#endif