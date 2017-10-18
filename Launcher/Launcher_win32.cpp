#include <Windows.h>

#include <Graphics/Graphics.h>

bool s_running = false;

LRESULT CALLBACK WindowProc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        break;

    case WM_QUIT:
    case WM_CLOSE:
        s_running = false;
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASS wnd;
    ZeroMemory(&wnd, sizeof(wnd));
    wnd.style = CS_HREDRAW | CS_VREDRAW;
    wnd.lpfnWndProc = WindowProc;
    wnd.hInstance = hInstance;
    wnd.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wnd.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wnd.lpszClassName = "xFramework";
    RegisterClass(&wnd);

    auto hwnd = CreateWindow("xFramework"
        , "xFramework"
        , WS_OVERLAPPEDWINDOW | WS_VISIBLE
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , nullptr
        , nullptr
        , hInstance
        , nullptr
    );

    auto* renderer = X::Graphics::GetRenderer();
    renderer->SetWindowHandler(hwnd);
    renderer->Init();

    s_running = true;

    MSG msg;
    while (s_running)
    {
        while (0 != PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    renderer->Destroy();

    DestroyWindow(hwnd);

    UnregisterClass("xFramework", hInstance);

    return 0;
}

namespace X
{
    namespace Launcher
    {
    }
}