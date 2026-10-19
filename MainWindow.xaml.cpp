#include "pch.h"
#include "MainWindow.xaml.h"
#include "DebugLog.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::KCMemMonitor::implementation
{
    namespace
    {
        constexpr double kGB = 1024.0 * 1024.0 * 1024.0;

        double BytesToGB(uint64_t bytes)
        {
            return static_cast<double>(bytes) / kGB;
        }

        hstring GBText(uint64_t bytes)
        {
            return hstring(::FormatDouble(BytesToGB(bytes), 1) + L" GB");
        }
    }

    MainWindow::MainWindow()
    {
        InitializeComponent();
        DebugLog(L"MainWindow::MainWindow() 构造完成");

        SetWindowSize();
        StartMonitoring();
    }

    void MainWindow::SetWindowSize()
    {
        // 对应设计稿推荐窗口尺寸 1280 x 720（逻辑像素，按当前 DPI 缩放）
        if (auto appWindow = AppWindow())
        {
            HWND hwnd = reinterpret_cast<HWND>(appWindow.Id().Value);
            UINT dpi = hwnd ? GetDpiForWindow(hwnd) : 96;
            if (dpi == 0)
            {
                dpi = 96;
            }

            const double scale = static_cast<double>(dpi) / 96.0;
            winrt::Windows::Graphics::SizeInt32 size;
            size.Width = static_cast<int32_t>(1280 * scale + 0.5);
            size.Height = static_cast<int32_t>(720 * scale + 0.5);
            appWindow.Resize(size);

            if (auto titleBar = appWindow.TitleBar())
            {
                titleBar.PreferredTheme(winrt::Microsoft::UI::Windowing::TitleBarTheme::Dark);
            }

            DebugLogF(L"窗口尺寸已设为 %d x %d（DPI=%u，缩放 %.2f）",
                size.Width, size.Height, dpi, scale);
        }
        else
        {
            DebugLog(L"警告：无法获取 AppWindow，保持默认尺寸");
        }
    }

    void MainWindow::StartMonitoring()
    {
        if (m_started) return;
        m_started = true;

        // 建立 1 秒刷新定时器
        winrt::Microsoft::UI::Xaml::DispatcherTimer timer;
        timer.Interval(std::chrono::milliseconds(1000));
        timer.Tick({ this, &MainWindow::OnTimerTick });
        timer.Start();
        m_timer = timer;

        DebugLog(L"内存监控定时器已启动（间隔 1000ms）");

        // 立即执行一次，避免等待第一个 tick
        UpdateMemoryStats();
    }

    void MainWindow::OnTimerTick(IInspectable const&, IInspectable const&)
    {
        UpdateMemoryStats();
    }

    void MainWindow::OnRefreshClicked(IInspectable const&, RoutedEventArgs const&)
    {
        DebugLog(L"用户点击【刷新】按钮");
        UpdateMemoryStats();
    }

    void MainWindow::UpdateMemoryStats()
    {
        // ---- 1) 读取整机物理内存 ----
        MEMORYSTATUSEX mem = {};
        mem.dwLength = sizeof(mem);
        if (!GlobalMemoryStatusEx(&mem))
        {
            DebugLogF(L"错误：GlobalMemoryStatusEx 失败，错误码 %lu", GetLastError());
            LiveText().Text(L"● 读取失败");
            LiveBadge().Background(SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0xC0, 0x3B, 0x4C)));
            return;
        }

        const uint64_t totalBytes = mem.ullTotalPhys;
        const uint64_t availBytes = mem.ullAvailPhys;
        const uint64_t usedBytes = totalBytes - availBytes;
        const double   usedPercent = (usedBytes * 100.0) / static_cast<double>(totalBytes);

        // 提交(Commit)比例，作为压力评估的次要依据
        const uint64_t commitTotal = mem.ullTotalPageFile;
        const uint64_t commitUsed = commitTotal - mem.ullAvailPageFile;
        const double   commitRatio = (commitTotal > 0)
            ? (commitUsed * 1.0) / static_cast<double>(commitTotal)
            : 0.0;

        DebugLogF(L"内存读取：总=%llu 可用=%llu 已用=%llu 占用=%.1f%% 提交比=%.2f",
            totalBytes, availBytes, usedBytes, usedPercent, commitRatio);

        // ---- 2) 刷新左侧 5 项指标 ----
        TotalMemoryText().Text(GBText(totalBytes));
        UsedMemoryText().Text(GBText(usedBytes));
        AvailableMemoryText().Text(GBText(availBytes));

        wchar_t buf[32];
        swprintf_s(buf, L"%.1f%%", usedPercent);
        PercentText().Text(hstring(buf));

        // ---- 3) 更新环形饼图 + 趋势图 ----
        RingChart().SetValue(usedPercent);
        PushHistorySample(static_cast<float>(usedPercent));

        // ---- 4) 更新内存压力指示 ----
        UpdatePressureIndicator(usedPercent, commitRatio);
    }

    void MainWindow::PushHistorySample(double percent)
    {
        m_history.push_back(static_cast<float>(percent));

        // 只保留最近 60 秒的数据点
        if (m_history.size() > 60)
        {
            m_history.erase(m_history.begin());
        }

        auto points = winrt::single_threaded_vector<float>();
        for (float value : m_history)
        {
            points.Append(value);
        }
        TrendChart().SetPoints(points);
    }

    void MainWindow::UpdatePressureIndicator(double usedPercent, double commitRatio)
    {
        // 压力等级判定：以物理内存占用为主、提交比例为辅
        PressureLevel level = PressureLevel::Low;
        if (usedPercent >= 75.0 || commitRatio >= 0.85)
        {
            level = PressureLevel::High;
        }
        else if (usedPercent >= 45.0 || commitRatio >= 0.60)
        {
            level = PressureLevel::Medium;
        }

        // 卡片文字
        switch (level)
        {
        case PressureLevel::Low:
            PressureText().Text(L"低");
            PressureText().Foreground(SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0x2E, 0xA9, 0x6B)));
            break;
        case PressureLevel::Medium:
            PressureText().Text(L"中");
            PressureText().Foreground(SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0xFF, 0xC5, 0x3D)));
            break;
        case PressureLevel::High:
            PressureText().Text(L"高");
            PressureText().Foreground(SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0xE5, 0x4D, 0x5E)));
            break;
        }

        // 指示条：滑块位置按占用比例连续移动
        const double canvasW = (PressureCanvas().ActualWidth() > 10)
            ? PressureCanvas().ActualWidth()
            : 300.0;
        const double frac = std::clamp(usedPercent / 100.0, 0.0, 1.0);
        const double thumbW = 12.0;

        PressureTrack().Width(canvasW);
        Canvas::SetLeft(PressureThumb(), frac * (canvasW - thumbW));

        // 刻度文字位置
        Canvas::SetLeft(PressureLowText(), 0.0);
        Canvas::SetLeft(PressureMidText(), canvasW / 2.0 - 6.0);
        Canvas::SetLeft(PressureHighText(), canvasW - thumbW);

        // 高亮当前等级
        const Brush dim = SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0x98, 0xA0, 0xB3));
        const Brush active = SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0xFF, 0xC5, 0x3D));
        PressureLowText().Foreground(dim);
        PressureMidText().Foreground(dim);
        PressureHighText().Foreground(dim);
        if (level == PressureLevel::Low) { PressureLowText().Foreground(active);  PressureLowText().FontWeight(Microsoft::UI::Text::FontWeights::Bold()); }
        if (level == PressureLevel::Medium) { PressureMidText().Foreground(active);  PressureMidText().FontWeight(Microsoft::UI::Text::FontWeights::Bold()); }
        if (level == PressureLevel::High) { PressureHighText().Foreground(active); PressureHighText().FontWeight(Microsoft::UI::Text::FontWeights::Bold()); }

        DebugLogF(L"压力等级=%s（占用 %.1f%% 提交比 %.2f）",
            level == PressureLevel::Low ? L"低" : (level == PressureLevel::Medium ? L"中" : L"高"),
            usedPercent, commitRatio);
    }
}

#include "MainWindow.g.cpp"
