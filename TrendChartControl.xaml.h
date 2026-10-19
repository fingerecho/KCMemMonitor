#pragma once

#include "TrendChartControl.g.h"

#include <vector>

namespace winrt::KCMemMonitor::implementation
{
    struct TrendChartControl : TrendChartControlT<TrendChartControl>
    {
        TrendChartControl();

        void SetPoints(winrt::Windows::Foundation::Collections::IVector<float> const& values);
        void OnSizeChanged(winrt::Windows::Foundation::IInspectable const& sender,
                           winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& args);
        void Redraw();

    private:
        std::vector<float> m_points;
        bool m_dirty{ false };
    };
}

namespace winrt::KCMemMonitor::factory_implementation
{
    struct TrendChartControl : TrendChartControlT<TrendChartControl, implementation::TrendChartControl>
    {
    };
}
