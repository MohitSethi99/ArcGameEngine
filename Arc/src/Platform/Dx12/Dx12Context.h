#pragma once

#include "Arc/Renderer/GraphicsContext.h"

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct IUnknown;
struct ID3D12Device8;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList6;
struct ID3D12DescriptorHeap;

namespace ArcEngine
{
	class DescriptorHeap;

	class Dx12Context : public GraphicsContext
	{
	public:
		static constexpr size_t FrameCount = 3;

	public:
		explicit Dx12Context(HWND hwnd);
		~Dx12Context() override;

		void Init() override;
		void SwapBuffers() override;
		void SetSyncInterval(uint32_t value) override;

		void OnBeginFrame() const;
		void OnEndFrame() const;

		static ID3D12Device8* GetDevice();
		static ID3D12GraphicsCommandList6* GetGraphicsCommandList();
		static ID3D12GraphicsCommandList6* GetUploadCommandList();
		static D3D12_CPU_DESCRIPTOR_HANDLE GetRtv();
		static int GetSwapChainFormat();
		static uint32_t GetCurrentFrameIndex();
		static void SetDeferredReleaseFlag();

		static DescriptorHeap* GetSrvHeap();
		static DescriptorHeap* GetRtvHeap();
		static DescriptorHeap* GetDsvHeap();
		static void DeferredRelease(IUnknown* resource);

		static uint32_t GetWidth();
		static uint32_t GetHeight();

	private:
		void CreateRTV() const;
		void CreateSwapchain() const;
		void ResizeSwapchain(uint32_t width, uint32_t height);

		static void ProcessDeferredReleases(uint32_t frameIndex);

	private:
		HWND m_Hwnd;
		uint32_t m_SyncInterval = 1;
		uint32_t m_PresentFlags = 0;
		bool m_ShouldResize = false;

		friend class ImGuiLayer;
		friend class DescriptorHeap;
		friend class Dx12RendererAPI;
	};
}