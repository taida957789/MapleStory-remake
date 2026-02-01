# LayoutMan 使用指南

## 概述

LayoutMan 是 UI 自動化構建和管理系統，從 WZ 資源自動創建 UI 元素。

## 基本使用

### 初始化

```cpp
auto layoutMan = std::make_unique<LayoutMan>();
layoutMan->Init(parentElement, 0, 0);
```

### 自動構建

```cpp
layoutMan->AutoBuild(
    L"UI/Login.img/WorldSelect/BtChannel/test",  // WZ 根路徑
    0,      // ID 起始值
    0,      // X 偏移
    0,      // Y 偏移
    true,   // 設置 tooltip
    false   // 允許相同 ID
);
```

### 查找控件

```cpp
auto pButton = layoutMan->ABGetButton(L"GoWorld");
auto pLayer = layoutMan->ABGetLayer(L"bg");
```

### 批量操作

```cpp
layoutMan->ABSetButtonShowAll(false);
layoutMan->ABSetButtonEnableAll(false);
layoutMan->ABSetButtonEnable(L"GoWorld", true);
```

## WZ 資源命名約定

AutoBuild 使用 `type:name` 格式：

- `button:GoWorld` - 按鈕，名稱 "GoWorld"
- `layer:bg` - 圖層，名稱 "bg"

## IDA Pro 參考

完整實作基於 IDA Pro 分析：
- AutoBuild: 0xb36170
- AddButton: 0xb30310
- ABGetButton: 0xb300f0

詳見：`docs/plans/2026-02-01-layoutman-design.md`
