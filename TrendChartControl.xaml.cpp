#include "pch.h"
#include "TrendChartControl.xaml.h"
#include "DebugLog.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::KCMemMonitor::implementation
{
    namespace
    {
        winrt::Windows::UI::Color Rgba(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
        {
            return winrt::Microsoft::UI::ColorHelper::FromArgb(a, r, g, b);
        }

        TextBlock MakeLabel(const hstring& text, double fontSize, Brush brush, double left, double top)
        {
            TextBlock tb;
            tb.Text(text);
            tb.FontSize(fontSize);
            tb.Foreground(brush);
            Canvas::SetLeft(tb, left);
            Canvas::SetTop(tb, top);
            return tb;
        }

        Microsoft::UI::Xaml::Shapes::Line MakeGridLine(double x1, double y1, double x2, double y2, Brush brush, double thickness)
        {
            Microsoft::UI::Xaml::Shapes::Line ln;
            ln.X1(x1); ln.Y1(y1); ln.X2(x2); ln.Y2(y2);
            ln.Stroke(brush);
            ln.StrokeThickness(thickness);
            return ln;
        }
    }

    TrendChartControl::TrendChartControl()
    {
        InitializeComponent();
        SizeChanged({ this, &TrendChartControl::OnSizeChanged });
    }

    void TrendChartControl::SetPoints(winrt::Windows::Foundation::Collections::IVector<float> const& values)
    {
        m_points.clear();
        m_points.reserve(values.Size());
        for (float value : values)
        {
            m_points.push_back(value);
        }

        m_dirty = true;
        if (PlotCanvas().ActualWidth() > 0 && PlotCanvas().ActualHeight() > 0)
        {
            Redraw();
        }
        else
        {
            // 尺寸尚未布局完成，等待 SizeChanged 触发重绘
            DebugLog(L"TrendChart::SetPoints 暂缓绘制，等待控件完成布局");
        }
    }

    void TrendChartControl::OnSizeChanged([[maybe_unused]] IInspectable const& sender,
        SizeChangedEventArgs const& e)
    {
        if (e.NewSize().Width <= 0 || e.NewSize().Height <= 0) return;
        m_dirty = true;
        Redraw();
    }

    void TrendChartControl::Redraw()
    {
        Canvas canvas = PlotCanvas();
        canvas.Children().Clear();

        const double W = canvas.ActualWidth();
        const double H = canvas.ActualHeight();
        if (W <= 10 || H <= 10)
        {
            DebugLogF(L"TrendChart::Redraw 跳过：画布尺寸无效 W=%.0f H=%.0f", W, H);
            return;
        }

        // 绘图区（四周留边距，给刻度文字）
        const double padL = 42.0, padR = 14.0, padT = 14.0, padB = 30.0;
        const double plotW = W - padL - padR;
        const double plotH = H - padT - padB;

        const double axisMaxPct = 100.0;   // 纵轴最大刻度
        const int gridCount = 5;           // 水平网格线数量(含0%)

        SolidColorBrush gridBrush(Rgba(255, 0x2A, 0x34, 0x47));
        SolidColorBrush labelBrush(Rgba(255, 0x80, 0x88, 0x9C));
        SolidColorBrush cyan(Rgba(255, 0x35, 0xC9, 0xFF));

        // ---- 1) 水平网格线 + 左侧百分比刻度 ----
        for (int i = 0; i < gridCount; ++i)
        {
            double pct = axisMaxPct * i / (gridCount - 1);
            double y = padT + plotH - (pct / axisMaxPct) * plotH;

            auto ln = MakeGridLine(padL, y, padL + plotW, y, gridBrush, 1.0);
            canvas.Children().Append(ln);

            wchar_t buf[16];
            swprintf_s(buf, L"%.0f%%", pct);
            TextBlock lbl = MakeLabel(buf, 11, labelBrush, padL - 38, y - 7);
            canvas.Children().Append(lbl);
        }

        // ---- 2) 底部时间刻度（近60秒） ----
        const wchar_t* timeLabels[] = { L"-60s", L"-45s", L"-30s", L"-15s", L"0s" };
        for (int i = 0; i < 5; ++i)
        {
            double x = padL + plotW * i / 4.0;
            TextBlock lbl = MakeLabel(timeLabels[i], 11, labelBrush, x - 12, padT + plotH + 8);
            canvas.Children().Append(lbl);
        }

        // ---- 3) 数据点采样：默认一条示例曲线（先让画面有内容，随后被真实数据覆盖） ----
        if (m_points.empty())
        {
            m_points = { 30, 34, 32, 38, 36, 42, 40, 45, 43, 48, 46, 52, 50, 55, 53, 58, 56, 60 };
        }

        const size_t n = m_points.size();
        if (n < 2) return;

        // 计算折线点坐标
        std::vector<winrt::Windows::Foundation::Point> pts;
        pts.reserve(n);
        for (size_t i = 0; i < n; ++i)
        {
            double v = std::clamp(static_cast<double>(m_points[i]), 0.0, 100.0);
            double x = padL + plotW * static_cast<double>(i) / static_cast<double>(n - 1);
            double y = padT + plotH - (v / axisMaxPct) * plotH;
            pts.push_back({ static_cast<float>(x), static_cast<float>(y) });
        }

        // ---- 4) 面积填充（折线下方的半透明青色） ----
        PathGeometry areaGeo;
        PathFigure areaFig;
        areaFig.StartPoint(pts.front());
        for (size_t i = 1; i < pts.size(); ++i)
        {
            LineSegment seg;
            seg.Point(pts[i]);
            areaFig.Segments().Append(seg);
        }
        // 折回到左下角闭合
        LineSegment down;
        down.Point({ pts.back().X, static_cast<float>(padT + plotH) });
        areaFig.Segments().Append(down);
        LineSegment left;
        left.Point({ pts.front().X, static_cast<float>(padT + plotH) });
        areaFig.Segments().Append(left);
        areaFig.IsClosed(true);
        areaGeo.Figures().Append(areaFig);

        Microsoft::UI::Xaml::Shapes::Path area;
        area.Data(areaGeo);
        area.Fill(SolidColorBrush(Rgba(120, 0x35, 0xC9, 0xFF)));
        canvas.Children().Append(area);

        // ---- 5) 折线主体 ----
        Microsoft::UI::Xaml::Shapes::Polyline line;
        line.Stroke(cyan);
        line.StrokeThickness(2.5);
        line.StrokeLineJoin(PenLineJoin::Round);
        auto linePts = line.Points();          // IVector<Point>
        for (auto& p : pts) linePts.Append(p);
        canvas.Children().Append(line);

        m_dirty = false;
        DebugLogF(L"TrendChart::Redraw 完成：点数=%zu 画布=%.0fx%.0f", n, W, H);
    }
}

#include "TrendChartControl.g.cpp"
