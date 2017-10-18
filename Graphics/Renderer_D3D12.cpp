#include "Renderer_D3D12.h"

#include <AzCore/Debug/Trace.h>

#define HR(result) AZ_Assert(SUCCEEDED(result), #result)

namespace X
{
    namespace Graphics
    {
        void Renderer_D3D12::Init()
        {
            if (m_hwnd == nullptr)
            {
                return;
            }

            // ����DXGI����
            {
                CreateDXGIFactory1(IID_PPV_ARGS(&m_factory));
            }
            AZ_Assert(m_factory, "fail to create DXGI factory");

            // ѭ��ö��Adapter��Ȼ���Դ���Device
            {
                for (auto i = 0; ; ++i)
                {
                    // EnumAdapters1��ȡӲ���豸��EnumWarpAdapter��ȡWARP�豸
                    if (DXGI_ERROR_NOT_FOUND == m_factory->EnumAdapters1(i, &m_adapter))
                    {
                        break;
                    }
                    // ����Device�ɹ�������
                    if (SUCCEEDED(D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device))))
                    {
                        break;
                    }
                }
            }
            AZ_Assert(m_device, "fail to create D3D12 device");

            // ����command queue
            {
                D3D12_COMMAND_QUEUE_DESC queueDesc;
                ZeroMemory(&queueDesc, sizeof(queueDesc));

                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // �����������б������
                queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // �������ȼ�
                queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // ����ִ�з�ʽ
                queueDesc.NodeMask = 0; // ���Կ�ʱʹ��

                HR(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
            }
            AZ_Assert(m_commandQueue, "fail to create D3D12 command queue");

            // ����swap chain
            {
                DXGI_SWAP_CHAIN_DESC swapChainDesc;
                ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

                swapChainDesc.BufferDesc.Width = m_windowWidth;
                swapChainDesc.BufferDesc.Height = m_windowHeight;
                swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                swapChainDesc.SampleDesc.Count = 1; // ���ز���
                swapChainDesc.SampleDesc.Quality = 0;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ��Ⱦ������
                swapChainDesc.BufferCount = m_windowBufferCount; // ��������
                swapChainDesc.OutputWindow = m_hwnd;
                swapChainDesc.Windowed = TRUE;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                swapChainDesc.Flags = 0;

                HR(m_factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &m_swapChain)); // D3D12�У�swap chain��Ҫcommand queue����
            }
            AZ_Assert(m_commandQueue, "fail to create DXGI swap chain");

            // alt+enter�����л���ȫ��ģʽ
            HR(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

            // ��������render target view��descriptor heap
            {
                D3D12_DESCRIPTOR_HEAP_DESC descriptHeapDesc;
                ZeroMemory(&descriptHeapDesc, sizeof(descriptHeapDesc));

                descriptHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // ����render target view
                descriptHeapDesc.NumDescriptors = m_windowBufferCount; // �뻺��������ͬ
                descriptHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Ĭ��
                descriptHeapDesc.NodeMask = 0; // ���Կ�

                HR(m_device->CreateDescriptorHeap(&descriptHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
            }
            AZ_Assert(m_rtvHeap, "fail to create render target heap");

            // ����render target
            {
                auto heapHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
                for (auto i = 0; i < m_windowBufferCount; ++i)
                {
                    HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTarget[i])));
                    m_device->CreateRenderTargetView(m_renderTarget[i], nullptr, heapHandle);
                    heapHandle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                }
            }

            // ����command allocator
            HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
        }

        void Renderer_D3D12::Destroy()
        {

        }
    }
}
