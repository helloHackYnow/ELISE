#include "Bloom.h"
 

namespace Odin
{

	Bloom::Bloom()
	{
	}

	void Bloom::Init()
	{
		InitVAO();

		// Init fbos
		for (int i = 0; i < 7; i++)
		{
			FrameBuffer* fbo = new FrameBuffer();
			std::unique_ptr<FrameBuffer> fbo_ptr(fbo);
			fbos.push_back(std::move(fbo_ptr));
			fbos.at(i)->Init(1, 1, true);
		}

		// Impossible values to make sure we are rescaling during the first draw
		width = 0;
		height = 0;

		//Init shaders
		AddShader(bs_prefilter_frag);
		AddShader(bs_blur_frag);
		AddShader(bs_downsample_frag);
		AddShader(bs_upsample_frag);
		AddShader(bs_combine_frag);
	}

	void Bloom::Apply(FrameBuffer& fbo_in, FrameBuffer& fbo_out, int w_width, int w_height)
	{
		ClearAll();

		if (w_width != width || w_height != height)
		{
			Rescale(w_width, w_height);
			width = w_width;
			height = w_height;
		}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		
		// First pass (filtering)
		shaders.at(0).use();
		shaders.at(0).setFloat("knee", knee);
		shaders.at(0).setFloat("threshold", threshold);
		shaders.at(0).setInt("screenTexture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fbo_in.GetTexture());
		Draw(*fbos.at(0));



	   // Second pass (horizontal blur)
	   shaders.at(1).use();
	   shaders.at(1).setVec2("blur_dir", 1, 0);
	   shaders.at(1).setInt("screenTexture", 0);
	   glBindTexture(GL_TEXTURE_2D, fbos.at(0)->GetTexture());
	   Draw(*fbos.at(1));

	   // Third pass (vertical blur)
	   shaders.at(1).setVec2("blur_dir", 0, 1);
	   glBindTexture(GL_TEXTURE_2D, fbos.at(1)->GetTexture());
	   Draw(*fbos.at(0));
	

		//Fourth pass (first down sampling) : x2
		shaders.at(2).use();
		shaders.at(2).setInt("screenTexture", 0);
		glBindTexture(GL_TEXTURE_2D, fbos.at(0)->GetTexture());
		glViewport(0, 0, width / 2, height / 2);
		Draw(*fbos.at(2));

		//Fifth pass (second downsampling) : x4
		glBindTexture(GL_TEXTURE_2D, fbos.at(2)->GetTexture());
		glViewport(0, 0, width / 4, height / 4);

		Draw(*fbos.at(3));

		//Sixth pass (third downsampling) : x8
		glBindTexture(GL_TEXTURE_2D, fbos.at(3)->GetTexture());
		glViewport(0, 0, width / 8, height / 8);

		Draw(*fbos.at(4));

		//Seventh pass (fourth downsampling) : x16
		glBindTexture(GL_TEXTURE_2D, fbos.at(4)->GetTexture());
		glViewport(0, 0, width / 16, height / 16);

		Draw(*fbos.at(5));

		//Eighth pass (fifth downsampling) : x32
		glBindTexture(GL_TEXTURE_2D, fbos.at(5)->GetTexture());
		glViewport(0, 0, width / 32, height / 32);

		Draw(*fbos.at(6));

		// Up sampling
		// Enable additive blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);

		shaders.at(3).use();
		shaders.at(3).setInt("screenTexture", 0);
		shaders.at(3).setFloat("filterRadius", 0.005f);

		for (int i = 5; i > 0; i--)
		{
			glBindTexture(GL_TEXTURE_2D, fbos.at(i + 1)->GetTexture());
			glViewport(0, 0, width / (int)pow(2, i - 1), height / (int)pow(2, i - 1));
			Draw(*fbos.at(i));
		}

		//Disable additive blending
		glDisable(GL_BLEND);

		// Mix main image and computed bloom
		shaders.at(4).use();
		shaders.at(4).setInt("mainTex", 0);
		shaders.at(4).setInt("bloomTex", 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fbo_in.GetTexture());

		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, fbos.at(1)->GetTexture());
		Draw(fbo_out);


		glEnable(GL_DEPTH_TEST);
	}

	void Bloom::Rescale(int w_width, int w_height)
	{
		fbos.at(0)->Rescale(w_width, w_height);
		fbos.at(1)->Rescale(w_width, w_height);
		fbos.at(2)->Rescale(w_width / 2, w_height / 2);
		fbos.at(3)->Rescale(w_width / 4, w_height / 4);
		fbos.at(4)->Rescale(w_width / 8, w_height / 8);
		fbos.at(5)->Rescale(w_width / 16, w_height / 16);
		fbos.at(6)->Rescale(w_width / 32, w_height / 32);
	}

	void Bloom::Draw(FrameBuffer& out)
	{
		glBindVertexArray(VAO);
		out.Use();

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Bloom::ClearAll()
	{
		glClearColor(0, 0, 0, 0);

		for (int i = 0; i < fbos.size(); i++)
		{
			fbos.at(i)->Use();
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

}