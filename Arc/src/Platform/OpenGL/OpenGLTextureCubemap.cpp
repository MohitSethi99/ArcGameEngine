#include "arcpch.h"
#include "OpenGLTextureCubemap.h"

#include "Arc/Renderer/Renderer3D.h"
#include "Arc/Renderer/Shader.h"

#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ArcEngine
{
	static Ref<Shader> s_EquirectangularToCubemapShader;
	static Ref<Shader> s_IrradianceShader;

	OpenGLTextureCubemap::OpenGLTextureCubemap(const std::string& path)
		: m_Path(path)
	{
		ARC_PROFILE_FUNCTION();

		const uint32_t cubemapSize = 2048;
		const uint32_t irradianceMapSize = 32;
		
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		float* data = nullptr;
		{
			ARC_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
			
			data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
		}
		ARC_CORE_ASSERT(data, "Failed to load image!");
		m_Width = width;
		m_Height = height;

		const GLenum internalFormat = GL_RGB16F, dataFormat = GL_RGB;

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		ARC_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");
		
		uint32_t captureFBO, captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemapSize, cubemapSize);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);



		glGenTextures(1, &m_HRDRendererID);
		glBindTexture(GL_TEXTURE_2D, m_HRDRendererID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		stbi_image_free(data);




		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
		for (size_t i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, cubemapSize, cubemapSize, 0, dataFormat, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] = 
		{
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		// convert HDR equirectangular environment map to cubemap equivalent
		if (!s_EquirectangularToCubemapShader)
			s_EquirectangularToCubemapShader = Shader::Create("assets/shaders/EquirectangularToCubemap.glsl");

		s_EquirectangularToCubemapShader->Bind();
		s_EquirectangularToCubemapShader->SetInt("u_EquirectangularMap", 0);
		s_EquirectangularToCubemapShader->SetMat4("u_Projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_HRDRendererID);

		glViewport(0, 0, cubemapSize, cubemapSize); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			s_EquirectangularToCubemapShader->SetMat4("u_View", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
								   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_RendererID, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			Renderer3D::DrawCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
		glGenTextures(1, &m_IrradianceRendererID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceRendererID);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradianceMapSize, irradianceMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceMapSize, irradianceMapSize);

		// solve diffuse integral by convolution to create an irradiance (cube)map.
		if (!s_IrradianceShader)
			s_IrradianceShader = Shader::Create("assets/shaders/Irradiance.glsl");
		
		s_IrradianceShader->Bind();
		s_IrradianceShader->SetInt("u_EnvironmentMap", 0);
		s_IrradianceShader->SetMat4("u_Projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

		glViewport(0, 0, irradianceMapSize, irradianceMapSize); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			s_IrradianceShader->SetMat4("u_View", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceRendererID, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			Renderer3D::DrawCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glDeleteFramebuffers(1, &captureFBO);
		glDeleteRenderbuffers(1, &captureRBO);
	}

	OpenGLTextureCubemap::~OpenGLTextureCubemap()
	{
		ARC_PROFILE_FUNCTION();
		
		glDeleteTextures(1, &m_HRDRendererID);
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCubemap::SetData(void* data, uint32_t size)
	{
		ARC_PROFILE_FUNCTION();
		
	}

	void OpenGLTextureCubemap::Bind(uint32_t slot) const
	{
		ARC_PROFILE_FUNCTION();
		
		if (slot == 0)
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
		else
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceRendererID);
	}
}