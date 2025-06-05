#include "Effect.h"

namespace Odin
{
	Effect::Effect()
	{

	}

	int Effect::AddShader(const char* fragment_data)
	{
		shaders.push_back(GShader(pp_default_vertex, fragment_data, true));

		return shaders.size() - 1;
	}

	void Effect::InitVAO()
	{
		float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};
		//Create VAO & VBO
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}

	void Effect::Init()
	{
		InitVAO();

		AddShader(pp_default_fragment);
	}

	void Effect::Apply(FrameBuffer& fbo_in, FrameBuffer& fbo_out, int w_width, int w_height)
	{
		glDisable(GL_DEPTH_TEST);
		if (shaders.size() > 0)
		{
			shaders.at(0).use();
		}
		shaders.at(0).setInt("screenTexture", 0);

		glBindTexture(GL_TEXTURE_2D, fbo_in.GetTexture());

		glBindVertexArray(VAO);
		fbo_out.Use();

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);
	}
}