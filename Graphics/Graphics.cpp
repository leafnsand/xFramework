#include "Graphics.h"
#include "Renderer_D3D12.h"

namespace X
{
    namespace Graphics
    {
        IRenderer* GetRenderer()
        {
            static Renderer_D3D12 s_renderer;
            return &s_renderer;
        }
    }
}