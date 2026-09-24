#include "App/Window.h"

#include <windowsx.h>

#include "Engine/Input/IInputReceiver.h"
#include "Engine/Input/IWindowEventListener.h"
#include "backends/imgui_impl_win32.h"

// ImGui用のウィンドウプロシージャハンドラ
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace /* anonymous */
{
/// @brief ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // ImGuiのウィンドウプロシージャハンドラを呼び出す
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    auto instance = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE)
    {
        auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        instance           = reinterpret_cast<Window*>(pCreateStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    if (instance)
    {
        return instance->HandleMessage(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ウィンドウクラス名
const auto ClassName = TEXT("d3d12-engineWindowClass");

} // namespace

bool Window::Create(int width, int height, const wchar_t* title)
{
    // インスタンスハンドルを取得
    m_hInst = GetModuleHandle(nullptr);
    if (m_hInst == nullptr)
    {
        return false;
    }

    // ウィンドウクラスの設定
    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // GetSysColorBrush(COLOR_BACKGROUND);
    wc.hInstance     = m_hInst;
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = ClassName;

    // ウィンドウの登録
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    // ウィンドウサイズの設定
    RECT rc           = {};
    rc.right          = static_cast<LONG>(width);
    rc.bottom         = static_cast<LONG>(height);
    const DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rc, style, FALSE);

    // ウィンドウの作成
    m_hWnd = CreateWindowEx(0, ClassName, title, style, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
        rc.bottom - rc.top, nullptr, nullptr, m_hInst, this);

    if (m_hWnd == nullptr)
    {
        return false;
    }

    // ウィンドウの表示
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    return true;
}

void Window::Destroy()
{
    if (m_hInst != nullptr)
    {
        UnregisterClass(ClassName, m_hInst);
    }

    m_hInst = nullptr;
    m_hWnd  = nullptr;
}

bool Window::ProcessMessages()
{
    MSG msg = {};

    // メッセージループ
    // そのフレームのメッセージをすべて処理する
    // QUITメッセージが来たらfalseを返す
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

LRESULT Window::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATE: {
        m_isActive = (wParam != WA_INACTIVE);
    }
    break;

    case WM_DESTROY: {
        PostQuitMessage(0);
    }
    break;

    case WM_WINDOWPOSCHANGED: {
        if (m_windowEventListener)
        {
            m_windowEventListener->OnWindowMoved();
        }
        // DefWindowProcに渡すことで，WM_SIZEやWM_MOVEが送信される
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    case WM_KEYDOWN: {
        // キーボード押下
        if (m_inputReceiver)
        {
            m_inputReceiver->OnKeyDown(static_cast<uint32_t>(wParam));
        }
    }
    break;

    case WM_KEYUP: {
        // キーボード解放
        if (m_inputReceiver)
        {
            m_inputReceiver->OnKeyUp(static_cast<uint32_t>(wParam));
        }
    }
    break;

    case WM_MOUSEMOVE: {
        // マウス移動
        if (m_inputReceiver)
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            m_inputReceiver->OnMouseMove(x, y);
        }
    }
    break;

    case WM_LBUTTONDOWN: {
        // マウス左ボタン押下
        SetCapture(hWnd); // マウスキャプチャを開始
        if (m_inputReceiver)
        {
            m_inputReceiver->OnMouseDown(Button::Left);
        }
    }
    break;

    case WM_LBUTTONUP: {
        // マウス左ボタン解放
        if (m_inputReceiver)
        {
            m_inputReceiver->OnMouseUp(Button::Left);
        }
        // 他のボタンが押されていなければマウスキャプチャを解放
        if ((wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) == 0)
        {
            ReleaseCapture();
        }
    }
    break;

    case WM_RBUTTONDOWN: {
        // マウス右ボタン押下
        SetCapture(hWnd); // マウスキャプチャを開始
        if (m_inputReceiver)
        {
            m_inputReceiver->OnMouseDown(Button::Right);
        }
    }
    break;

    case WM_RBUTTONUP: {
        // マウス右ボタン解放
        if (m_inputReceiver)
        {
            m_inputReceiver->OnMouseUp(Button::Right);
        }
        // 他のボタンが押されていなければマウスキャプチャを解放
        if ((wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) == 0)
        {
            ReleaseCapture();
        }
    }
    break;

    case WM_CAPTURECHANGED: {
        // マウスキャプチャが解除された場合の処理
        if (m_inputReceiver)
        {
            // すべてのボタンを解放状態にする
            m_inputReceiver->OnMouseUp(Button::Left);
            m_inputReceiver->OnMouseUp(Button::Right);
            m_inputReceiver->OnMouseUp(Button::Middle);
        }
    }
    break;

    case WM_KILLFOCUS: {
        // ウィンドウがフォーカスを失った場合、すべてのボタンを解放状態にする
        if (m_inputReceiver)
        {
            m_inputReceiver->OnMouseUp(Button::Left);
            m_inputReceiver->OnMouseUp(Button::Right);
            m_inputReceiver->OnMouseUp(Button::Middle);
        }
    }
    break;

    case WM_ERASEBKGND: {
        // 背景の消去を行わない（ちらつき防止）
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        // ウィンドウサイズ変更
        // 最小化
        if (wParam == SIZE_MINIMIZED)
        {
            m_isMinimized = true;
            break;
        }
        m_isMinimized = false;

        // 幅と高さ
        uint32_t w = static_cast<uint32_t>(LOWORD(lParam));
        uint32_t h = static_cast<uint32_t>(HIWORD(lParam));

        // 幅または高さが0の場合は無視する
        if (w == 0 || h == 0)
            break;

        m_width  = w;
        m_height = h;

        // サイズ変更中はイベントを送信しない
        if (m_isSizeMoving)
            break;

        // ドラッグ無しでサイズ変更された場合はイベントを送信する
        NotifyResize();
    }
    break;

    case WM_ENTERSIZEMOVE: {
        m_isSizeMoving = true;
    }
    break;

    case WM_EXITSIZEMOVE: {
        m_isSizeMoving = false;
        NotifyResize();
    }
    break;

    case WM_MENUCHAR: {
        // Alt+任意キーでエラー音が鳴るのを防ぐ
        return MAKELRESULT(0, MNC_CLOSE);
    }

    default: {
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    }

    return 0;
}

void Window::ToggleFullscreen()
{
    if (!m_isFullscreen)
    {
        // フルスクリーンに切り替える場合の処理
        // フルスクリーンにする前に切り替え前のWINDOWPLACEMENTとスタイルを保存する（戻す時に使う）
        m_windowPlacement.length = sizeof(WINDOWPLACEMENT);
        if (!GetWindowPlacement(m_hWnd, &m_windowPlacement))
        {
            return; // 失敗した場合は中止
        }
        m_windowStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);

        // 現在のモニタ情報を取得する
        HMONITOR monitor        = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
        if (!GetMonitorInfo(monitor, &monitorInfo))
        {
            return; // 失敗した場合は中止
        }

        // タイトルバーと枠を外す
        SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        // 現在のモニタのサイズに合わせる
        SetWindowPos(m_hWnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED);

        m_isFullscreen = true;
    }
    else
    {
        // ウィンドウモードに戻す場合の処理
        SetWindowLongPtr(m_hWnd, GWL_STYLE, m_windowStyle);
        SetWindowPlacement(m_hWnd, &m_windowPlacement);
        SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        m_isFullscreen = false;
    }
}

void Window::NotifyResize()
{
    // サイズが変わっていなければ通知しない
    if (m_width == m_notifiedWidth && m_height == m_notifiedHeight)
    {
        return;
    }
    m_notifiedWidth  = m_width;
    m_notifiedHeight = m_height;

    if (m_windowEventListener)
    {
        m_windowEventListener->OnWindowResized(m_width, m_height);
    }
}
