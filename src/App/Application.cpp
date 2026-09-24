#include "App/Application.h"

Application::Application()  = default;
Application::~Application() = default;

int Application::Run()
{
    if (!Init())
    {
        return -1;
    }

    MainLoop();

    Term();

    return 0;
}

// 初期化（ウィンドウの作成，D3D・ゲームロジックの初期化）
bool Application::Init()
{
    // COM初期化
    auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        return false;
    }

    // ウィンドウの作成
    // サイズは物理ピクセルで指定する（DPI対応済みのため，表示スケールによる拡大は行わない）
    // 描画解像度とウィンドウのクライアントサイズを一致させるため，論理ピクセルには変換しない
    const int windowWidth  = 1280;
    const int windowHeight = 720;

    if (!m_Window.Create(windowWidth, windowHeight, L"d3d12-engine"))
    {
        CoUninitialize();
        return false;
    }

    // エンジンの初期化
    if (!m_Engine.Initialize(m_Window.GetHwnd(), windowWidth, windowHeight))
    {
        CoUninitialize();
        return false;
    }

    // InputSystemの登録
    m_Window.SetInputReceiver(&m_Engine.GetInputSystem());

    // WindowEventListenerの登録
    m_Window.SetWindowEventListener(&m_Engine.GetWindowEventListener());

    // ゲームロジックの初期化
    m_Game.Init(&m_Engine);

    return true;
}

void Application::Term()
{
    // エンジンの終了処理
    m_Engine.Shutdown();

    // ウィンドウの破棄
    m_Window.Destroy();

    // COMの終了処理
    CoUninitialize();
}

void Application::MainLoop()
{
    // 実行中フラグを立てる
    m_isRunning = true;

    // 時間の初期化
    m_lastFrameTime = std::chrono::high_resolution_clock::now();

    // メインループ
    while (m_isRunning)
    {
        // 1. 経過時間の計測
        auto now        = std::chrono::high_resolution_clock::now();
        m_deltaTime     = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;

        // 2. 入力処理のフレーム開始
        m_Engine.GetInputSystem().BeginFrame();

        // 3. メッセージポンプ
        // OSからのメッセージを処理する
        if (!m_Window.ProcessMessages())
        {
            m_isRunning = false;
            break;
        }

        // フルスクリーンの切り替え
        if (m_Engine.GetInputSystem().WasKeyPressed(VK_F11))
        {
            m_Window.ToggleFullscreen();
        }

        // 4. ゲームロジックの更新
        m_Game.Tick(m_deltaTime);

        // 5. 描画処理
        // 最小化中は描画処理をスキップする
        if (m_Window.IsMinimized())
        {
            // 最小化中はCPU使用率を下げるために待機する
            // QS_ALLINPUT：あらゆる入力/メッセージで反応
            MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

            continue;
        }

        // フレーム開始
        m_Engine.BeginFrame();

        // 定数バッファの書き込み
        m_Engine.Update();

        // 描画コマンド発行
        m_Engine.Render();

        // フレーム終了
        m_Engine.EndFrame();

        // 画面表示
        m_Engine.Present();
    }
}
