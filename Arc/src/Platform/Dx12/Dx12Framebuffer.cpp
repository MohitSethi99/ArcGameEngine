#include "arcpch.h"
#include "Dx12Framebuffer.h"

#include "DxHelper.h"
#include "Dx12Allocator.h"

namespace ArcEngine
{
	Dx12Framebuffer::Dx12Framebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		ARC_PROFILE_SCOPE();

		Invalidate();
	}

	Dx12Framebuffer::~Dx12Framebuffer()
	{
		ARC_PROFILE_SCOPE();

		for (DepthFrame& depthAttachment : m_DepthAttachment)
		{
			depthAttachment.Release();
		}

		for (eastl::vector<ColorFrame>& colorAttachments : m_ColorAttachments)
		{
			for (ColorFrame& attachment : colorAttachments)
			{
				attachment.Release();
			}
		}
	}

	void Dx12Framebuffer::Invalidate()
	{
		ARC_PROFILE_SCOPE();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		auto* device = Dx12Context::GetDevice();
		DescriptorHeap* srvDescriptorHeap = Dx12Context::GetSrvHeap();
		DescriptorHeap* rtvDescriptorHeap = Dx12Context::GetRtvHeap();
		DescriptorHeap* dsvDescriptorHeap = Dx12Context::GetDsvHeap();

		eastl::vector<FramebufferTextureSpecification>& attachments = m_Specification.Attachments.Attachments;
		for (FramebufferTextureSpecification& attachment : attachments)
		{
			DXGI_FORMAT format = GetDxgiFormat(attachment.TextureFormat);

			if (attachment.TextureFormat == FramebufferTextureFormat::DEPTH24STENCIL8)
			{
				constexpr D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

				D3D12_CLEAR_VALUE defaultDepthClear{};
				defaultDepthClear.Format = format;
				defaultDepthClear.DepthStencil.Depth = m_ClearDepth.r;
				defaultDepthClear.DepthStencil.Stencil = static_cast<uint8_t>(m_ClearDepth.g);

				D3D12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, m_Specification.Width, m_Specification.Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

				srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
				dsvDesc.Format = format;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

				int i = 0;
				for (DepthFrame& depthAttachment : m_DepthAttachment)
				{
					D3D12MA::Allocation* allocation;
					DescriptorHandle srvHandle = srvDescriptorHeap->Allocate();
					DescriptorHandle dsvHandle = dsvDescriptorHeap->Allocate();

					Dx12Allocator::CreateRtvResource(D3D12_HEAP_TYPE_DEFAULT, &depthDesc, state, &defaultDepthClear, &allocation);
					ID3D12Resource* resource = allocation->GetResource();
					device->CreateShaderResourceView(resource, &srvDesc, srvHandle.CPU);
					device->CreateDepthStencilView(resource, &dsvDesc, dsvHandle.CPU);

					depthAttachment = { allocation, srvHandle, dsvHandle, srvDescriptorHeap->HeapIndexGPU(srvHandle), state};

					std::string resourceName = std::format("Depth Resource ({}): {}", i, m_Specification.Name);
					NameResource(allocation, resourceName.c_str());
					++i;
				}
			}
			else
			{
				constexpr D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;

				D3D12_CLEAR_VALUE defaultColorClear{};
				defaultColorClear.Format = format;
				memcpy(&defaultColorClear.Color, glm::value_ptr(m_ClearColor), sizeof(glm::vec4));

				D3D12_RESOURCE_DESC colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, m_Specification.Width, m_Specification.Height, 1, 1, m_Specification.Samples, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

				srvDesc.Format = format;
				rtvDesc.Format = format;

				int i = 0;
				for (eastl::vector<ColorFrame>& colorAttachment : m_ColorAttachments)
				{
					D3D12MA::Allocation* allocation;
					DescriptorHandle srvHandle = srvDescriptorHeap->Allocate();
					DescriptorHandle rtvHandle = rtvDescriptorHeap->Allocate();

					Dx12Allocator::CreateRtvResource(D3D12_HEAP_TYPE_DEFAULT, &colorDesc, state, &defaultColorClear, &allocation);
					ID3D12Resource* resource = allocation->GetResource();
					device->CreateShaderResourceView(resource, &srvDesc, srvHandle.CPU);
					device->CreateRenderTargetView(resource, &rtvDesc, rtvHandle.CPU);

					colorAttachment.emplace_back(allocation, srvHandle, rtvHandle, srvDescriptorHeap->HeapIndexGPU(srvHandle), state);
					m_RtvHandles[i].emplace_back(rtvHandle.CPU);

					std::string resourceName = std::format("Color Resource ({}): {}", i, m_Specification.Name);
					NameResource(allocation, resourceName.c_str());
					++i;
				}
			}
		}
	}

	void Dx12Framebuffer::Bind(GraphicsCommandList commandList)
	{
		ARC_PROFILE_SCOPE();

		D3D12GraphicsCommandList* cmdList = reinterpret_cast<D3D12GraphicsCommandList*>(commandList);

		const uint32_t backFrame = Dx12Context::GetCurrentFrameIndex();

		D3D12_RESOURCE_BARRIER barriers[20];
		int numBarriers = 0;
		eastl::vector<ColorFrame>& colorAttachments = m_ColorAttachments[backFrame];
		for (ColorFrame& attachment : colorAttachments)
		{
			if (attachment.State == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			{
				barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(attachment.Allocation->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				attachment.State = D3D12_RESOURCE_STATE_RENDER_TARGET;
				++numBarriers;
			}
		}
		if (m_DepthAttachment[backFrame].Allocation)
		{
			if (m_DepthAttachment[backFrame].State == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			{
				barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(m_DepthAttachment[backFrame].Allocation->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				m_DepthAttachment[backFrame].State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				++numBarriers;
			}
		}
		if (numBarriers > 0)
			reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->ResourceBarrier(numBarriers, barriers);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DepthAttachment[backFrame].DsvHandle.CPU;
		cmdList->OMSetRenderTargets(static_cast<UINT>(m_RtvHandles[backFrame].size()), m_RtvHandles[backFrame].data(), false, dsvHandle.ptr != 0 ? &dsvHandle : nullptr);

		const D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_Specification.Width, (float)m_Specification.Height, 0.0f, 1.0f };
		const D3D12_RECT scissor = { 0, 0, static_cast<LONG>(m_Specification.Width), static_cast<LONG>(m_Specification.Height) };

		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &scissor);
	}

	void Dx12Framebuffer::Transition(GraphicsCommandList commandList)
	{
		ARC_PROFILE_SCOPE();

		const uint32_t backFrame = Dx12Context::GetCurrentFrameIndex();

		D3D12_RESOURCE_BARRIER barriers[20];
		int numBarriers = 0;
		eastl::vector<ColorFrame>& colorAttachments = m_ColorAttachments[backFrame];
		for (ColorFrame& attachment : colorAttachments)
		{
			barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(attachment.Allocation->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			attachment.State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			++numBarriers;
		}
		if (m_DepthAttachment[backFrame].Allocation)
		{
			barriers[numBarriers] = CD3DX12_RESOURCE_BARRIER::Transition(m_DepthAttachment[backFrame].Allocation->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_DepthAttachment[backFrame].State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			++numBarriers;
		}
		if (numBarriers > 0)
			reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->ResourceBarrier(numBarriers, barriers);
	}

	void Dx12Framebuffer::Clear(GraphicsCommandList commandList)
	{
		ARC_PROFILE_SCOPE();

		D3D12GraphicsCommandList* cmdList = reinterpret_cast<D3D12GraphicsCommandList*>(commandList);
		const uint32_t backFrame = Dx12Context::GetCurrentFrameIndex();

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DepthAttachment[backFrame].DsvHandle.CPU;

		const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvHandles = m_RtvHandles[backFrame];
		for (const D3D12_CPU_DESCRIPTOR_HANDLE rtv : rtvHandles)
			cmdList->ClearRenderTargetView(rtv, glm::value_ptr(m_ClearColor), 0, nullptr);
		if (dsvHandle.ptr != 0)
			cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, m_ClearDepth.r, static_cast<UINT8>(m_ClearDepth.g), 0, nullptr);
	}

	void Dx12Framebuffer::BindColorAttachment(GraphicsCommandList commandList, uint32_t index, uint32_t slot)
	{
		ARC_PROFILE_SCOPE();

		const uint32_t backFrame = Dx12Context::GetCurrentFrameIndex();
		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootDescriptorTable(slot, m_ColorAttachments[backFrame][index].SrvHandle.GPU);
	}

	void Dx12Framebuffer::BindDepthAttachment(GraphicsCommandList commandList, uint32_t slot)
	{
		ARC_PROFILE_SCOPE();

		const uint32_t backFrame = Dx12Context::GetCurrentFrameIndex();
		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootDescriptorTable(slot, m_DepthAttachment[backFrame].SrvHandle.GPU);
	}

	void Dx12Framebuffer::Resize(uint32_t width, uint32_t height)
	{
		ARC_PROFILE_SCOPE();

		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			ARC_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
			return;
		}

		m_Specification.Width = width;
		m_Specification.Height = height;

		for (DepthFrame& depthAttachment : m_DepthAttachment)
			depthAttachment.Release();
		for (eastl::vector<ColorFrame>& attachments : m_ColorAttachments)
		{
			for (ColorFrame& attachment : attachments)
				attachment.Release();
		}

		for (DepthFrame& depthAttachment : m_DepthAttachment)
			depthAttachment = {};
		for (eastl::vector<ColorFrame>& colorAttachments : m_ColorAttachments)
			colorAttachments.clear();
		for (eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvHandles : m_RtvHandles)
			rtvHandles.clear();
			
		Invalidate();
	}
}
