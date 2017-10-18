#pragma once

namespace X
{
    namespace Graphics
    {
        class IRenderer
        {
        public:
            /// destructor
            virtual ~IRenderer() = default;
            /// set native window handler
            virtual void SetWindowHandler(void *handler) = 0;
            /// initialize renderer
            virtual void Init() = 0;
            /// destroy renderer
            virtual void Destroy() = 0;
        };
    }
}
