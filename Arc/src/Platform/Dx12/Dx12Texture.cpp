#include "arcpch.h"
#include "Dx12Texture.h"

#include "Dx12Allocator.h"

#include "DxHelper.h"
#include "Arc/Renderer/PipelineState.h"
#include "Arc/Renderer/Renderer.h"

namespace ArcEngine
{
	Dx12Texture2D::Dx12Texture2D(uint32_t width, uint32_t height, TextureFormat format)
		: m_Format(format), m_Width(width), m_Height(height)
	{
		m_Channels = ChannelCountFromFormat(m_Format);
		m_Handle = Dx12Context::GetSrvHeap()->Allocate();
		m_HeapStart = Dx12Context::GetSrvHeap()->GpuStart();
		GraphicsCommandList commandList = Dx12Context::BeginRecordingCommandList();
		Dx12Utils::CreateTexture(reinterpret_cast<D3D12GraphicsCommandList*>(commandList), &m_ImageAllocation, &m_UploadImageAllocation, D3D12_SRV_DIMENSION_TEXTURE2D, m_Format, m_Width, m_Height, 1, nullptr, &m_Handle, nullptr);
		Dx12Context::EndRecordingCommandList(commandList);
	}

	Dx12Texture2D::Dx12Texture2D(const eastl::string& path, TextureFormat format)
		: m_Format(format)
	{
		ARC_PROFILE_SCOPE();

		m_Channels = ChannelCountFromFormat(m_Format);

		stbi_set_flip_vertically_on_load(0);

		int width, height, channels;
		void* data = nullptr;
		{
			ARC_PROFILE_SCOPE_NAME("stbi_load Texture");
			switch (m_Format)
			{
				case TextureFormat::None:		data = stbi_load(path.c_str(), &width, &height, &channels, m_Channels); break;
				case TextureFormat::RGBA32F:
				case TextureFormat::RGB32F:
				case TextureFormat::RG32F:
				case TextureFormat::R32F:
				case TextureFormat::RGBA16F:
				case TextureFormat::RG16F:
				case TextureFormat::R16F:		data = stbi_loadf(path.c_str(), &width, &height, &channels, m_Channels); break;
				case TextureFormat::RGBA8:
				case TextureFormat::RG8:
				case TextureFormat::R8:			data = stbi_load(path.c_str(), &width, &height, &channels, m_Channels); break;
				default:						data = stbi_load(path.c_str(), &width, &height, &channels, m_Channels); break;
			}
			
		}
		ARC_CORE_ASSERT(data, "Failed to load image!");

		m_Width = width;
		m_Height = height;

		m_Handle = Dx12Context::GetSrvHeap()->Allocate();
		m_HeapStart = Dx12Context::GetSrvHeap()->GpuStart();
		GraphicsCommandList commandList = Dx12Context::BeginRecordingCommandList();
		Dx12Utils::CreateTexture(reinterpret_cast<D3D12GraphicsCommandList*>(commandList), &m_ImageAllocation, &m_UploadImageAllocation, D3D12_SRV_DIMENSION_TEXTURE2D, m_Format, m_Width, m_Height, 1, data, &m_Handle, nullptr);
		Dx12Context::EndRecordingCommandList(commandList);

		stbi_image_free(data);

		m_Path = path;
	}

	Dx12Texture2D::~Dx12Texture2D()
	{
		ARC_PROFILE_SCOPE();

		Dx12Context::GetSrvHeap()->Free(m_Handle);

		if (m_UploadImageAllocation)
			m_UploadImageAllocation->Release();
		if (m_ImageAllocation)
			m_ImageAllocation->Release();
	}

	uint32_t Dx12Texture2D::GetIndex() const
	{
		return static_cast<uint32_t>(m_Handle.GPU.ptr - m_HeapStart.ptr) / Dx12Context::GetSrvHeap()->DescriptorSize();
	}

	void Dx12Texture2D::SetData(GraphicsCommandList commandList, const TextureData data, [[maybe_unused]] uint32_t size)
	{
		ARC_PROFILE_SCOPE();

		ARC_CORE_ASSERT(m_Width * m_Height == size / m_Channels);

		Dx12Utils::SetTextureData(reinterpret_cast<D3D12GraphicsCommandList*>(commandList), m_ImageAllocation, m_UploadImageAllocation, m_Format, m_Width, m_Height, data);
	}

	void Dx12Texture2D::Bind(GraphicsCommandList commandList, uint32_t slot) const
	{
		ARC_PROFILE_SCOPE();

		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootDescriptorTable(slot, m_Handle.GPU);
	}




	Dx12TextureCube::Dx12TextureCube(const eastl::string& path, TextureFormat format)
		: m_Format(format)
	{
		ARC_PROFILE_SCOPE();

		m_HDRHandle = Dx12Context::GetSrvHeap()->Allocate();
		m_UavHandle = Dx12Context::GetSrvHeap()->Allocate();
		m_SrvHandle = Dx12Context::GetSrvHeap()->Allocate();
		m_HeapStart = Dx12Context::GetSrvHeap()->GpuStart();


		GraphicsCommandList cmdList = Dx12Context::BeginRecordingCommandList();
		D3D12GraphicsCommandList* cmdListNative = reinterpret_cast<D3D12GraphicsCommandList*>(cmdList);
		{
			int width, height, channels;
			float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 4);
			ARC_CORE_ASSERT(data, "Failed to load image!");
			Dx12Utils::CreateTexture(cmdListNative, &m_HDRImageAllocation, &m_HDRUploadImageAllocation, D3D12_SRV_DIMENSION_TEXTURE2D, TextureFormat::RGBA32F, width, height, 1, data, &m_HDRHandle, nullptr);
			stbi_image_free(data);
			m_Path = path;
		}








		m_Width = 2048;
		m_Height = 2048;

		Dx12Utils::CreateTexture(cmdListNative, &m_ImageAllocation, &m_UploadImageAllocation, D3D12_SRV_DIMENSION_TEXTURECUBE, m_Format, m_Width, m_Height, 6, nullptr, &m_SrvHandle, &m_UavHandle);

		PipelineLibrary& pipelineLibrary = Renderer::GetPipelineLibrary();
		const PipelineSpecification computeSpec = { .Type = ShaderType::Compute };
		Ref<PipelineState> equirectToCubemapPipeline;
		if (pipelineLibrary.Exists("EquirectangularToCubemap.hlsl"))
			equirectToCubemapPipeline = pipelineLibrary.Get("EquirectangularToCubemap.hlsl");
		else
			equirectToCubemapPipeline = pipelineLibrary.Load("assets/shaders/EquirectangularToCubemap.hlsl", computeSpec);

		equirectToCubemapPipeline->Bind(cmdList);
		const D3D12_RESOURCE_BARRIER barrierIn[]
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_HDRImageAllocation->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_ImageAllocation->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		};
		cmdListNative->ResourceBarrier(2, barrierIn);

		cmdListNative->SetComputeRootDescriptorTable(equirectToCubemapPipeline->GetSlot("InputTexture"), m_HDRHandle.GPU);
		cmdListNative->SetComputeRootDescriptorTable(equirectToCubemapPipeline->GetSlot("OutputTexture"), m_UavHandle.GPU);
		cmdListNative->Dispatch(m_Width / 32, m_Height / 32, 6);

		const D3D12_RESOURCE_BARRIER barrierOut[]
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_HDRImageAllocation->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_ImageAllocation->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		};
		cmdListNative->ResourceBarrier(2, barrierOut);

		RenderCommand::ExecuteCommandList(cmdList);
	}

	Dx12TextureCube::~Dx12TextureCube()
	{
		ARC_PROFILE_SCOPE();

		Dx12Context::GetSrvHeap()->Free(m_HDRHandle);
		Dx12Context::GetSrvHeap()->Free(m_UavHandle);
		Dx12Context::GetSrvHeap()->Free(m_SrvHandle);

		if (m_HDRUploadImageAllocation)
			m_HDRUploadImageAllocation->Release();
		if (m_HDRImageAllocation)
			m_HDRImageAllocation->Release();

		if (m_UploadImageAllocation)
			m_UploadImageAllocation->Release();
		if (m_ImageAllocation)
			m_ImageAllocation->Release();
	}

	uint32_t Dx12TextureCube::GetIndex() const
	{
		return static_cast<uint32_t>(m_SrvHandle.GPU.ptr - m_HeapStart.ptr) / Dx12Context::GetSrvHeap()->DescriptorSize();
	}

	void Dx12TextureCube::SetData(GraphicsCommandList commandList, const TextureData data, [[maybe_unused]] uint32_t size)
	{
		ARC_PROFILE_SCOPE();

		ARC_CORE_ASSERT(m_Width * m_Height == size / ChannelCountFromFormat(m_Format));

		Dx12Utils::SetTextureData(reinterpret_cast<D3D12GraphicsCommandList*>(commandList), m_ImageAllocation, m_UploadImageAllocation, m_Format, m_Width, m_Height, data);
	}

	void Dx12TextureCube::Bind(GraphicsCommandList commandList, uint32_t slot) const
	{
		ARC_PROFILE_SCOPE();

		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootDescriptorTable(slot, m_SrvHandle.GPU);
	}
}
