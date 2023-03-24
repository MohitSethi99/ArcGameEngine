#include "arcpch.h"
#include "Dx12Buffer.h"

#include "d3dx12.h"
#include "Dx12Allocator.h"

namespace ArcEngine
{
	//////////////////////////////////////////////////////////////////////
	// VertexBuffer //////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	static void CreateBuffer(D3D12MA::Allocation** allocation, uint32_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = size;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		Dx12Allocator::CreateBufferResource(heapType, &resourceDesc, initialState, allocation);
	}

	static void SetBufferData(const D3D12MA::Allocation* allocation, const void* data, uint32_t size, uint32_t offset = 0)
	{
		ID3D12Resource* resource = allocation->GetResource();
		constexpr D3D12_RANGE readRange{ 0, 0 };
		void* dst;
		resource->Map(0, &readRange, &dst);
		dst = (char*)dst + offset;
		memcpy(dst, data, size);
		resource->Unmap(0, nullptr);
	}

	Dx12VertexBuffer::Dx12VertexBuffer(uint32_t size, uint32_t stride)
	{
		ARC_PROFILE_SCOPE()

		CreateBuffer(&m_Allocation, size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
		m_BufferView.BufferLocation = m_Allocation->GetResource()->GetGPUVirtualAddress();
		m_BufferView.StrideInBytes = stride;
		m_BufferView.SizeInBytes = size;

		CreateBuffer(&m_UploadAllocation, size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	Dx12VertexBuffer::Dx12VertexBuffer(const float* vertices, uint32_t size, uint32_t stride)
	{
		ARC_PROFILE_SCOPE()

		CreateBuffer(&m_Allocation, size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
		m_BufferView.BufferLocation = m_Allocation->GetResource()->GetGPUVirtualAddress();
		m_BufferView.StrideInBytes = stride;
		m_BufferView.SizeInBytes = size;

		CreateBuffer(&m_UploadAllocation, size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		SetBufferData(m_UploadAllocation, vertices, size);
		Dx12Context::GetUploadCommandList()->CopyBufferRegion(m_Allocation->GetResource(), 0, m_UploadAllocation->GetResource(), 0, size);
	}

	Dx12VertexBuffer::~Dx12VertexBuffer()
	{
		ARC_PROFILE_SCOPE()

		if (m_UploadAllocation)
			m_UploadAllocation->Release();
		if (m_Allocation)
			m_Allocation->Release();
	}

	void Dx12VertexBuffer::Bind() const
	{
		ARC_PROFILE_SCOPE()

		Dx12Context::GetGraphicsCommandList()->IASetVertexBuffers(0, 1, &m_BufferView);
	}

	void Dx12VertexBuffer::SetData(const void* data, uint32_t size)
	{
		ARC_PROFILE_SCOPE()

		ID3D12Resource* resource = m_Allocation->GetResource();
		ID3D12Resource* uploadResource = m_UploadAllocation->GetResource();

		const auto toCommonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		Dx12Context::GetUploadCommandList()->ResourceBarrier(1, &toCommonBarrier);

		SetBufferData(m_UploadAllocation, data, size);
		Dx12Context::GetUploadCommandList()->CopyBufferRegion(resource, 0, uploadResource, 0, size);

		const auto toVertexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		Dx12Context::GetUploadCommandList()->ResourceBarrier(1, &toVertexBarrier);
	}


	//////////////////////////////////////////////////////////////////////
	// IndexBuffer ///////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	Dx12IndexBuffer::Dx12IndexBuffer(const uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		ARC_PROFILE_SCOPE()

		const uint32_t size = count * sizeof(uint32_t);

		CreateBuffer(&m_Allocation, size, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
		m_BufferView.BufferLocation = m_Allocation->GetResource()->GetGPUVirtualAddress();
		m_BufferView.Format = DXGI_FORMAT_R32_UINT;
		m_BufferView.SizeInBytes = size;

		CreateBuffer(&m_UploadAllocation, size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		SetBufferData(m_UploadAllocation, indices, size);
		Dx12Context::GetUploadCommandList()->CopyBufferRegion(m_Allocation->GetResource(), 0, m_UploadAllocation->GetResource(), 0, size);
	}

	Dx12IndexBuffer::~Dx12IndexBuffer()
	{
		ARC_PROFILE_SCOPE()

		if (m_UploadAllocation)
			m_UploadAllocation->Release();
		if (m_Allocation)
			m_Allocation->Release();
	}

	void Dx12IndexBuffer::Bind() const
	{
		ARC_PROFILE_SCOPE()

		Dx12Context::GetGraphicsCommandList()->IASetIndexBuffer(&m_BufferView);
	}


	Dx12ConstantBuffer::Dx12ConstantBuffer(uint32_t size, uint32_t count, uint32_t registerIndex)
	{
		ARC_PROFILE_SCOPE()

		m_RegisterIndex = registerIndex;
		m_Size = size;
		m_Count = count;
		m_AlignedSize = (size + 255) &~ 255;

		const uint32_t allocationSize = m_AlignedSize * count;

		D3D12MA::ALLOCATION_DESC allocationDesc{};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(allocationSize);

		for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
		{
			Dx12Allocator::CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &m_Allocation[i]);

			D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
			desc.BufferLocation = m_Allocation[i]->GetResource()->GetGPUVirtualAddress();
			desc.SizeInBytes = allocationSize;
			m_Handle[i] = Dx12Context::GetSrvHeap()->Allocate();
			Dx12Context::GetDevice()->CreateConstantBufferView(&desc, m_Handle[i].CPU);
		}
	}

	Dx12ConstantBuffer::~Dx12ConstantBuffer()
	{
		ARC_PROFILE_SCOPE()

		for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
		{
			Dx12Context::GetSrvHeap()->Free(m_Handle[i]);

			if (m_Allocation[i])
				m_Allocation[i]->Release();
		}
	}

	void Dx12ConstantBuffer::SetData(const void* data, uint32_t size, uint32_t index)
	{
		ARC_PROFILE_SCOPE()

		ARC_CORE_ASSERT(m_Count > index, "Constant buffer index can't be greater than count! Overflow!")

		SetBufferData(m_Allocation[Dx12Context::GetCurrentFrameIndex()], data, size == 0 ? m_Size : size, m_AlignedSize * index);
	}

	void Dx12ConstantBuffer::Bind(uint32_t index) const
	{
		ARC_PROFILE_SCOPE()

		ARC_CORE_ASSERT(m_Count > index, "Constant buffer index can't be greater than count! Overflow!")

		const auto gpuVirtualAddress = m_Allocation[Dx12Context::GetCurrentFrameIndex()]->GetResource()->GetGPUVirtualAddress() + m_AlignedSize * index;
		Dx12Context::GetGraphicsCommandList()->SetGraphicsRootConstantBufferView(m_RegisterIndex, gpuVirtualAddress);
	}
}
