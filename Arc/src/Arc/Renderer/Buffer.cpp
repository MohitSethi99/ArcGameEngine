#include "arcpch.h"
#include "Arc/Renderer/Buffer.h"

#include "Arc/Renderer/Renderer.h"

#include "Platform/Dx12/Dx12Buffer.h"

namespace ArcEngine
{
	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, uint32_t stride)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	ARC_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
			case RendererAPI::API::Dx12:	return CreateRef<Dx12VertexBuffer>(size, stride);
		}

		ARC_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(const float* vertices, uint32_t size, uint32_t stride)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	ARC_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
			case RendererAPI::API::Dx12:	return CreateRef<Dx12VertexBuffer>(vertices, size, stride);
		}

		ARC_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, const size_t count)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	ARC_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
			case RendererAPI::API::Dx12:	return CreateRef<Dx12IndexBuffer>(indices, static_cast<uint32_t>(count));
		}

		ARC_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<ConstantBuffer> ConstantBuffer::Create(uint32_t stride, uint32_t count, uint32_t registerIndex)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	ARC_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
			case RendererAPI::API::Dx12:	return CreateRef<Dx12ConstantBuffer>(stride, count, registerIndex);
		}

		ARC_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<StructuredBuffer> StructuredBuffer::Create(uint32_t stride, uint32_t count, uint32_t registerIndex)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	ARC_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
			case RendererAPI::API::Dx12:	return CreateRef<Dx12StructuredBuffer>(stride, count, registerIndex);
		}

		ARC_CORE_ASSERT(false, "Unknown RendererAPI!")
			return nullptr;
	}
}
