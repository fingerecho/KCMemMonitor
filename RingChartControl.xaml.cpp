#include "pch.h"
#include "RingChartControl.xaml.h"
#include "DebugLog.h"

#include <cmath>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::KCMemMonitor::implementation
{
    namespace
    {
        constexpr double kCanvasSize = 240.0;
        constexpr double kCenter = kCanvasSize / 2.0;
        constexpr double kRadius = 92.0;
        constexpr double kThickness = 30.0;
        constexpr double kPi = 3.14159265358979323846;

        winrt::Windows::Foundation::Point PointOnCircle(double angleDegrees)
        {
            const double radians = angleDegrees * kPi / 180.0;
            return {
                static_cast<float>(kCenter + kRadius * std::cos(radians)),
                static_cast<float>(kCenter + kRadius * std::sin(radians))
            };
        }
    }

    RingChartControl::RingChartControl()
    {
        InitializeComponent();
        Redraw();
    }

    void RingChartControl::SetValue(double percent)
    {
        m_percent = std::clamp(percent, 0.0, 100.0);

        wchar_t buffer[16]{};
        swprintf_s(buffer, L"%.0f%%", m_percent);
        CenterValueText().Text(buffer);

        Redraw();
    }

    void RingChartControl::Redraw()
    {
        Canvas canvas = RingCanvas();
        canvas.Children().Clear();

        // 底环
        Microsoft::UI::Xaml::Shapes::Ellipse track;
        track.Width(kRadius * 2.0);
        track.Height(kRadius * 2.0);
        track.Stroke(SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0x2A, 0x34, 0x47)));
        track.StrokeThickness(kThickness);
        Canvas::SetLeft(track, kCenter - kRadius);
        Canvas::SetTop(track, kCenter - kRadius);
        canvas.Children().Append(track);

        if (m_percent <= 0.0)
        {
            return;
        }

        // 进度弧（顶部 -90° 开始，顺时针），使用蓝紫渐变
        const double sweep = std::min(m_percent / 100.0, 0.9999) * 360.0;
        const double startAngle = -90.0;
        const double endAngle = startAngle + sweep;

        LinearGradientBrush progressBrush;
        progressBrush.StartPoint({ 0.0f, 0.0f });
        progressBrush.EndPoint({ 1.0f, 1.0f });

        GradientStop firstStop;
        firstStop.Color(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0x2F, 0x6B, 0xFF));
        firstStop.Offset(0.0);
        progressBrush.GradientStops().Append(firstStop);

        GradientStop secondStop;
        secondStop.Color(winrt::Microsoft::UI::ColorHelper::FromArgb(255, 0x9B, 0x5C, 0xF6));
        secondStop.Offset(1.0);
        progressBrush.GradientStops().Append(secondStop);

        PathFigure figure;
        figure.StartPoint(PointOnCircle(startAngle));

        ArcSegment arc;
        arc.Point(PointOnCircle(endAngle));
        arc.Size({ static_cast<float>(kRadius), static_cast<float>(kRadius) });
        arc.SweepDirection(SweepDirection::Clockwise);
        arc.IsLargeArc(sweep > 180.0);
        figure.Segments().Append(arc);

        PathGeometry geometry;
        geometry.Figures().Append(figure);

        Microsoft::UI::Xaml::Shapes::Path progressArc;
        progressArc.Data(geometry);
        progressArc.Stroke(progressBrush);
        progressArc.StrokeThickness(kThickness);
        progressArc.StrokeStartLineCap(PenLineCap::Round);
        progressArc.StrokeEndLineCap(PenLineCap::Round);
        canvas.Children().Append(progressArc);

        DebugLogF(L"RingChart::SetValue 完成：占用 %.1f%%", m_percent);
    }
}

#include "RingChartControl.g.cpp"
