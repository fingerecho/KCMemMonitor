#pragma once

#include "MainWindow.g.h"

#include <vector>

namespace winrt::KCMemMonitor::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        enum class PressureLevel
        {
            Low,
            Medium,
            High
        };

        void SetWindowSize();
        void StartMonitoring();
        void OnTimerTick(winrt::Windows::Foundation::IInspectable const& sender,
                         winrt::Windows::Foundation::IInspectable const& args);
        void OnRefreshClicked(winrt::Windows::Foundation::IInspectable const& sender,
                              winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void UpdateMemoryStats();
        void PushHistorySample(double percent);
        void UpdatePressureIndicator(double usedPercent, double commitRatio);

    private:
        bool m_started{ false };
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_timer{ nullptr };
        std::vector<float> m_history;
    };
}

namespace winrt::KCMemMonitor::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
