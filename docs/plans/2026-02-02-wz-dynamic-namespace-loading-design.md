# WZ 動態命名空間載入系統設計

## 概述

將 WZ 資源管理器從靜態載入順序改為動態命名空間發現系統。透過 Base.wz 的結構自動發現並載入所有相關的 WZ 檔案，支援多檔案分片（Map.wz, Map1.wz, Map2.wz, Map001.wz 等）。

## 目標

1. 移除硬編碼的 `WZ_LOAD_ORDER` 陣列
2. 支援任意數量的命名空間分片檔案
3. 統一資源存取路徑（使用 `Map/Back/...` 而非 `Map2/Back/...`）
4. Base.wz 成為檔案組織的唯一真實來源

## 架構變更

### 現有方式

```cpp
// 硬編碼載入順序
static constexpr std::array<const char*, 15> WZ_LOAD_ORDER = {
    "Character", "Mob", "Skill", ...
};

// 獨立存儲每個檔案
std::unordered_map<std::string, std::shared_ptr<WzFile>> m_wzFiles;
```

### 新方式

```cpp
// 命名空間到檔案列表的映射
std::unordered_map<std::string, std::vector<std::shared_ptr<WzFile>>> m_namespaces;
```

## 初始化流程

```
Initialize()
  ├─> InitializeBase()
  │     └─> 載入 Base.wz
  │
  └─> DiscoverAndLoadNamespaces()
        ├─> 遍歷 Base.wz 根節點
        ├─> 對每個 WzNodeType::Directory 節點：
        │     └─> LoadNamespaceFiles(namespaceName)
        │           ├─> 掃描無編號檔案：{namespace}.wz
        │           ├─> 掃描單位數編號：{namespace}1.wz, {namespace}2.wz, ...
        │           └─> 掃描三位數編號：{namespace}001.wz, {namespace}002.wz, ...
        │
        └─> 設定 m_bInitialized = true
```

## 檔案掃描規則

### 編號模式支援

同時支援三種編號模式：
- 無編號：`Map.wz`
- 單位數：`Map1.wz`, `Map2.wz`, `Map3.wz`, ...
- 三位數：`Map001.wz`, `Map002.wz`, `Map003.wz`, ...

### 載入順序

嚴格按照以下順序載入：
```
Map.wz → Map1.wz → Map2.wz → ... → Map001.wz → Map002.wz → ...
```

### 掃描終止條件

- 對於單位數編號：掃描 1, 2, 3... 直到檔案不存在
- 對於三位數編號：掃描 001, 002, 003... 直到檔案不存在
- 連續不存在即停止該模式的掃描

### 錯誤處理

| 情況 | 處理方式 |
|------|---------|
| Base.wz 不存在 | 初始化失敗（ERROR） |
| Base.wz 無法開啟 | 初始化失敗（ERROR） |
| 命名空間無任何檔案 | 忽略，記錄 DEBUG |
| 檔案存在但無法開啟 | 初始化失敗（ERROR） |
| 檔案不存在 | 停止該編號模式掃描 |

## 資源查找策略

### GetProperty 實作

```cpp
auto GetProperty(const std::string& path) -> std::shared_ptr<WzProperty>
{
    // 1. 檢查快取
    if (cached) return cached;

    // 2. 解析路徑："Map/Back/sky.img" -> namespace="Map", subPath="Back/sky.img"
    auto [namespaceName, subPath] = ParsePath(path);

    // 3. 取得命名空間的檔案列表
    auto* files = GetNamespaceFiles(namespaceName);
    if (!files) return emptyProperty;

    // 4. 順序查找（按載入順序）
    for (const auto& wzFile : *files) {
        auto prop = wzFile->FindNode(subPath);
        if (prop && prop->GetNodeType() != WzNodeType::NotSet) {
            m_propertyCache[path] = prop;
            return prop;  // 第一個找到的立即返回
        }
    }

    // 5. 找不到，返回空 Property
    return emptyProperty;
}
```

### 衝突處理

當多個檔案包含相同路徑時（例如 Map1.wz 和 Map2.wz 都有 `Map/Back/sky.img`）：
- 使用第一個找到的資源
- 按照檔案載入順序優先（Map.wz > Map1.wz > Map2.wz > Map001.wz...）

## 資料結構變更

### WzResMan.h

**新增**：
```cpp
// 命名空間映射
std::unordered_map<std::string, std::vector<std::shared_ptr<WzFile>>> m_namespaces;

// 新增私有函數
auto DiscoverAndLoadNamespaces() -> bool;
auto LoadNamespaceFiles(const std::string& namespaceName) -> bool;
auto GetNamespaceFiles(const std::string& namespaceName)
    -> const std::vector<std::shared_ptr<WzFile>>*;
```

**移除**：
```cpp
// 移除硬編碼順序
// static constexpr std::array<const char*, 15> WZ_LOAD_ORDER = {...};

// 移除舊的檔案映射
// std::unordered_map<std::string, std::shared_ptr<WzFile>> m_wzFiles;

// 移除相關函數
// auto InitializeWzFiles() -> bool;
// auto GetWzFile(const std::string& name) -> std::shared_ptr<WzFile>;
// auto LoadWzFile(const std::string& name) -> bool;
```

**保留**：
```cpp
std::map<std::string, std::shared_ptr<WzProperty>> m_propertyCache;
std::unordered_map<std::string, std::int32_t> m_wzVersions;
std::string m_sBasePath;
bool m_bInitialized;
```

## 向後相容性

### 不受影響的 API

以下 API 保持完全相容：
- `GetProperty(path)` - 路徑格式不變
- `LoadSoundData(prop)` - 不受影響
- `Initialize()`, `Shutdown()` - 簽名不變
- `SetBasePath()`, `GetBasePath()`, `IsInitialized()` - 不變
- `FlushCachedObjects(flags)` - 不變

### 需要調整的程式碼

1. **src/stage/Logo.cpp**
   - 目前使用 `WZ_LOAD_ORDER` 顯示載入進度
   - 需改為顯示命名空間載入進度
   - 可選：顯示「正在載入 Map (5 個檔案)...」

2. **任何直接呼叫 `LoadWzFile()` 的程式碼**
   - 改為依賴自動載入機制
   - 或改用新的 API（如需要）

3. **任何使用 `WZ_LOAD_ORDER` 的程式碼**
   - 需要找到替代方案或移除

## 日誌設計

### INFO 級別
```
INFO: Initializing WZ Resource Manager...
INFO: Loaded Base.wz (version 1029)
INFO: Loaded namespace 'Map' with 5 files: Map.wz, Map1.wz, Map2.wz, Map001.wz, Map002.wz
INFO: Loaded namespace 'UI' with 1 file: UI.wz
...
INFO: WZ Resource Manager initialized: 8 namespaces, 32 total files
```

### DEBUG 級別
```
DEBUG: Namespace 'DataSvr' has no WZ files, skipping
DEBUG: Loaded Map2.wz (version 1029)
```

### ERROR 級別
```
ERROR: Base.wz not found at: ./Base.wz
ERROR: Failed to open Map2.wz: file exists but cannot be loaded
```

## 測試策略

### 單元測試

1. **命名空間發現**
   - Base.wz 有多個 Directory 節點
   - Base.wz 有混合節點（Directory + Image）
   - Base.wz 為空或只有 Image 節點

2. **檔案掃描**
   - 只有無編號檔案
   - 只有單位數編號
   - 只有三位數編號
   - 混合所有三種
   - 編號不連續

3. **資源查找**
   - 資源在第一個檔案
   - 資源在後續檔案
   - 多個檔案有相同路徑
   - 資源不存在

4. **錯誤處理**
   - Base.wz 不存在
   - 檔案存在但損壞
   - 命名空間無檔案

### 整合測試

- 使用真實 WZ 檔案結構測試
- 驗證所有 UI 元素能正確載入
- 驗證 Logo.cpp 載入進度正常

## 實作順序

1. 修改 WzResMan.h 資料結構
2. 實作 DiscoverAndLoadNamespaces()
3. 實作 LoadNamespaceFiles()
4. 修改 GetProperty() 查找邏輯
5. 移除舊的 InitializeWzFiles() 和 WZ_LOAD_ORDER
6. 調整 Logo.cpp
7. 編寫單元測試
8. 整合測試

## 風險與考量

### 效能考量

- **快取機制**：保持現有的 m_propertyCache，避免重複查找
- **掃描開銷**：檔案系統掃描只在初始化時執行一次
- **查找開銷**：最壞情況需要遍歷一個命名空間的所有檔案，但通常第一個檔案就能找到

### 相容性風險

- Logo.cpp 可能需要較大調整
- 需要確保所有使用 WZ_LOAD_ORDER 的地方都被找到並修改

### 測試覆蓋

- 需要模擬各種檔案組合情況
- 需要測試檔案損壞、權限問題等邊界情況

## 設計決策記錄

| 問題 | 決策 | 原因 |
|------|------|------|
| 檔案編號模式 | 同時支援無編號、單位數、三位數 | 最大靈活性 |
| 命名空間判斷 | 使用 WzNodeType::Directory | 明確的類型判斷 |
| 衝突處理 | 按檔案順序，第一個優先 | 簡單且可預測 |
| 載入順序 | 無編號 → 單位數 → 三位數 | 符合常見命名習慣 |
| 掃描終止 | 連續不存在即停止 | 避免不必要的檔案系統存取 |
| 載入失敗 | 檔案存在但無法開啟則終止 | 確保資料完整性 |
| 命名空間發現 | 掃描所有 Directory 節點 | 動態且靈活 |
| 檔案不存在 | 可忽略，記錄 DEBUG | 允許可選的命名空間 |
| 資源查找 | 順序查找直到找到 | 高效且符合優先順序 |
| 資料結構 | namespace → vector<WzFile> | 清晰的組織結構 |

## 總結

此設計將 WZ 資源管理從靜態配置改為動態發現，提供更好的靈活性和可維護性。關鍵改變包括移除硬編碼的載入順序、支援多檔案命名空間、以及統一的資源存取路徑。實作時需要特別注意向後相容性和錯誤處理。
