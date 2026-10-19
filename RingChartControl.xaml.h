#pragma once

#include "RingChartControl.g.h"

namespace winrt::KCMemMonitor::implementation
{
    struct RingChartControl : RingChartControlT<RingChartControl>
    {
        RingChartControl();

        void SetValue(double percent);
        void Redraw();

    private:
        double m_percent{ 0.0 };
    };
}

namespace winrt::KCMemMonitor::factory_implementation
{
    struct RingChartControl : RingChartControlT<RingChartControl, implementation::RingChartControl>
    {
    };
}
