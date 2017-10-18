#pragma once

#include "IRenderer.h"

#include <d3d12.h>
#include <dxgi1_4.h>

namespace X
{
    namespace Graphics
    {
        class Renderer_D3D12 : public IRenderer
        {
        public:
            /// destructor
            ~Renderer_D3D12() override = default;
            /// initialize renderer
            void Init() override;
            /// destroy renderer
            void Destroy() override;
            /// set hwnd
            void SetWindowHandler(void *handler) override { m_hwnd = reinterpret_cast<HWND>(handler); }

        private:
            static const UINT32 m_windowBufferCount = 2;
            static const UINT32 m_windowWidth = 1024;
            static const UINT32 m_windowHeight = 768;

            HWND m_hwnd = nullptr;

            IDXGIFactory4* m_factory = nullptr;
            IDXGIAdapter1* m_adapter = nullptr;
            IDXGISwapChain* m_swapChain = nullptr;

            ID3D12Device* m_device = nullptr;
            ID3D12CommandQueue* m_commandQueue = nullptr;
            ID3D12DescriptorHeap* m_rtvHeap = nullptr;
            ID3D12Resource* m_renderTarget[m_windowBufferCount];
            ID3D12CommandAllocator* m_commandAllocator = nullptr;
        };
    }
}