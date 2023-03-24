#pragma once

#include "Arc/Utils/StringUtils.h"

namespace ArcEngine
{
	struct PipelineSpecification;
	class PipelineState;

	enum class ShaderType
	{
		None = 0,
		Vertex,
		Pixel,

		Fragment = Pixel
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Recompile(const std::filesystem::path& path) = 0;

		[[nodiscard]] virtual const std::string& GetName() const = 0;

		[[nodiscard]] static Ref<Shader> Create(const std::filesystem::path& filepath);
	};

	class PipelineLibrary
	{
	public:
		Ref<PipelineState> Load(const std::filesystem::path& shaderPath, const PipelineSpecification& spec);
		void ReloadAll();

		[[nodiscard]] Ref<PipelineState> Get(const std::string& name);

		[[nodiscard]] bool Exists(const std::string& name) const;
	private:

		std::unordered_map<std::string, Ref<PipelineState>, UM_StringTransparentEquality> m_Pipelines;
		std::unordered_map<std::string, std::string, UM_StringTransparentEquality> m_ShaderPaths;
	};
}
