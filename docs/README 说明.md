# KCMemMonitor —— Windows 纯系统内存监控（C++ WinUI3）

一个**不含进程列表 / ListView** 的整机物理内存监控界面，严格对照设计稿实现。
只展示 5 项整机指标：**总内存 / 已用内存 / 可用内存 / 内存占用百分比 / 内存压力等级（低·中·高）**，
并带环形饼图与「近 60 秒」动态趋势图。

- 技术栈：C++ / WinRT + WinUI 3（Windows App SDK）
- 窗口尺寸：**1280 × 720 逻辑像素**（运行时会根据系统 DPI 自动缩放）
- 已适配深色主题，标题栏与界面配色保持一致

---

## 目录结构

```
KCMemMonitor/
├── KCMemMonitor.sln / KCMemMonitor.slnx  # 解决方案
├── KCMemMonitor.vcxproj                  # 工程文件
├── KCMemMonitor.vcxproj.filters
├── Package.appxmanifest                  # MSIX 打包清单
├── app.manifest                          # DPI 感知声明
├── pch.h / pch.cpp                       # 预编译头
├── DebugLog.h                            # 调试日志助手（OutputDebugString）
├── App.xaml / App.xaml.h / App.xaml.cpp
├── MainWindow.xaml                       # ★ 界面布局（三区：指标卡 / 环形图 / 趋势图）
├── MainWindow.xaml.h / .cpp              # ★ 数据刷新逻辑 + 定时器
├── RingChartControl.*                    # 环形饼图控件（渐变进度弧 + 中央数值）
├── TrendChartControl.*                   # 近60秒折线面积图控件（网格/刻度/折线/面积）
└── Assets/                               # 打包占位图标
```

---

## 界面 → 代码对照

| 设计稿区域 | 实现 |
|---|---|
| 顶部标题栏「内存监控 / 实时更新 / 刷新」 | `MainWindow.xaml` 顶部 Grid + `LiveBadge`、`RefreshButton` |
| 左侧 5 项指标卡 | 5 个 `MetricCardStyle` 卡片，数值绑定 `TotalMemoryText` 等 |
| 中央环形饼图 | `RingChartControl`，蓝紫渐变圆弧 + 中央大数字 + 图例 |
| 底部压力条 低·中·高 | `PressureCanvas` 轨道 + 黄色滑块 + 三档刻度高亮 |
| 右侧「使用趋势 近60秒」 | `TrendChartControl`，网格 + 百分比刻度 + 时间刻度 + 折线面积图 |

数据来源：`GlobalMemoryStatusEx()`（每秒由 `DispatcherTimer` 刷新）。
内存压力等级 = 物理内存占用为主、提交(Commit)比例为辅，映射为 低/中/高。

---

## 环境要求

- Windows 10 2004+ / Windows 11
- Visual Studio 2022 或更高版本（勾选「使用 C++ 的桌面开发」与 MSIX 打包工具）
- 首次构建由 NuGet 自动还原（工程内已包含 `packages` 缓存）：
  - `Microsoft.WindowsAppSDK` **2.5.1**
  - `Microsoft.Windows.CppWinRT` **3.0.260818.1**

## 构建与运行

1. 用 Visual Studio 打开 `KCMemMonitor.sln`（或 `KCMemMonitor.slnx`）
2. 平台选择 `x64`，配置选择 `Debug`
3. 直接按 **F5** 启动（VS 会自动部署并运行）
4. 调试日志：**视图 → 输出 → 显示来自：调试**

## 调试日志

`DebugLog.h` 通过 `OutputDebugStringW` 输出（VS 输出窗口或 DebugView 可见），关键点：
- App / Window 构造
- 定时器启动
- 每次内存读取的总/可用/已用/占用%/提交比
- 压力等级判定与图表重绘点数

## 说明 / 限制

- `Assets/*.png` 为临时占位图标，可自行替换为正式图标。
- 左侧卡片图标使用系统 `SymbolIcon`，如需精确复刻设计稿可替换为自定义字体图标。
