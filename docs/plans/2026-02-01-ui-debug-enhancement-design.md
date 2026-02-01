# UI Debug Enhancement Design

**Date:** 2026-02-01
**Status:** Approved
**Author:** Claude (based on user requirements)

## Overview

Enhance the existing `DebugOverlay` system to display UI element hierarchy and position information when clicking on UI elements like `UIChannelSelect`, `UIWorldSelect`, etc.

## Requirements

When clicking on a UI element, display:
- UI element type (e.g., "UIButton", "UIChannelSelect")
- Local position (relative to parent)
- Absolute position (screen coordinates)
- Size (width × height)
- Z-order (rendering depth)
- Parent element information
- Child element count
- Associated layers

## Design Decisions

### 1. Integration Approach
**Decision:** Integrate into existing `DebugOverlay` system
**Rationale:** Unified debug interface, no additional hotkeys needed

### 2. Information Scope
**Decision:** Display complete position and hierarchy information
**Rationale:** Provides sufficient information for debugging position issues without information overload

### 3. UI-Layer Association
**Decision:** UIElement base class auto-registration via `SetLayer()`
**Rationale:**
- All UI elements automatically support debug
- No manual registration needed in derived classes
- Maintains graphics layer independence

### 4. Information Organization
**Decision:** UI element-centric display with selection list for overlapping elements
**Rationale:**
- Maintains existing DebugOverlay interaction pattern
- Focuses on UI elements (the debug target)
- Shows all layers belonging to selected UI element

## Architecture

### Modified Files

1. **`src/debug/DebugOverlay.h/.cpp`**
   - Add `UIElement*` to `LayerEntry`
   - Add `RegisterUIElement()` method
   - Replace `FindCanvasesAt()` with `FindUIElementsAt()`
   - Modify `RenderInfoPopup()` to show UI element information

2. **`src/ui/UIElement.h/.cpp`**
   - Auto-register in `SetLayer()` method
   - Add `GetDebugTypeName()` virtual method
   - Auto-unregister in destructor

3. **Derived UI classes** (optional)
   - Override `GetDebugTypeName()` for friendly names

## Data Structures

### LayerEntry (Modified)
```cpp
struct LayerEntry {
    std::weak_ptr<WzGr2DLayer> layer;
    std::string name;
    UIElement* uiElement = nullptr;  // NEW: Associated UI element
};
```

### UIElementHitInfo (New)
```cpp
struct UIElementHitInfo {
    UIElement* element;              // UI element pointer
    std::string typeName;            // Type name (e.g., "UIButton")
    Point2D localPos;                // Local position (relative to parent)
    Point2D absolutePos;             // Absolute position (screen coords)
    int width, height;               // Size
    int zOrder;                      // Z-order
    std::string parentName;          // Parent element type name
    int childCount;                  // Number of children
    std::vector<std::string> layerNames;  // All layers of this element
};
```

## Registration Flow

### 1. Auto-registration in UIElement::SetLayer()
```cpp
void UIElement::SetLayer(std::shared_ptr<WzGr2DLayer> layer) {
    m_pLayer = std::move(layer);

    #ifdef MS_DEBUG_CANVAS
    if (m_pLayer) {
        auto typeName = GetDebugTypeName();
        DebugOverlay::GetInstance().RegisterUIElement(
            this, m_pLayer, typeName + " Main Layer"
        );
    }
    #endif
}
```

### 2. Multi-layer UI elements (e.g., UIEdit)
```cpp
// In UIEdit::CreateLayer()
m_pLayer = gr.CreateLayer(...);       // Background layer
m_pTextLayer = gr.CreateLayer(...);   // Text layer
m_pCaretLayer = gr.CreateLayer(...);  // Caret layer

#ifdef MS_DEBUG_CANVAS
DebugOverlay::GetInstance().RegisterUIElement(this, m_pLayer, "UIEdit Background");
DebugOverlay::GetInstance().RegisterUIElement(this, m_pTextLayer, "UIEdit Text");
DebugOverlay::GetInstance().RegisterUIElement(this, m_pCaretLayer, "UIEdit Caret");
#endif
```

### 3. Auto-unregistration in destructor
```cpp
UIElement::~UIElement() {
    #ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().UnregisterUIElement(this);
    #endif
}
```

## Click Handling Flow

### FindUIElementsAt() Algorithm
```
1. Iterate through all registered layers
2. For each layer:
   a. Check if layer is valid and visible
   b. Perform hit test at click position
   c. If hit, get associated UIElement
   d. If UIElement not already processed:
      i. Collect complete information
      ii. Add to hit list
3. Sort hit list by Z-order (descending)
4. Return deduplicated UI element list
```

### OnMouseClick() Behavior
```
1. Find all UI elements at click position
2. If none found, return false
3. If one found:
   - Show info popup directly
4. If multiple found:
   - Show selection list
   - User clicks to select
   - Show selected element's info popup
```

## Display Format

### Selection List (Multiple Overlapping Elements)
```
┌─ UI Elements (click to inspect) ────────────────┐
│ [Z:1001] UIButton (50, 100)                     │
│ [Z:1000] UIChannelSelect (203, 194)             │
│ [Z:999] UIEdit (50, 130)                        │
└──────────────────────────────────────────────────┘
```

### Info Popup (Selected Element)
```
┌─ UIButton ────────────────────────────────────┐
│ Local Pos:    (50, 100)                       │
│ Absolute Pos: (253, 294)                      │
│ Size:         120 × 40                        │
│ Z-order:      1001                            │
│ Parent:       UIChannelSelect                 │
│ Children:     0                               │
│ ───────────────────────────────────────────── │
│ Layers (2):                                   │
│   • UIButton Normal                           │
│   • UIButton MouseOver                        │
│                                               │
│ Click anywhere or press ESC to close          │
└───────────────────────────────────────────────┘
```

## Error Handling

### 1. UIElement Deletion
- Unregister in destructor
- Clear all references in LayerEntry
- Close popup if displaying deleted element

### 2. Layer Deletion
- Use `weak_ptr`, skip if expired
- Continue processing other layers

### 3. Null Pointer Safety
- Check `entry.uiElement` before use
- Check selected index bounds
- Validate element pointer before rendering

### 4. Screen Boundary
- Adjust popup position to stay on screen
- Minimum padding from edges

### 5. GetDebugTypeName() Implementation
- Base class: use `typeid(*this).name()`
- Derived classes: override with friendly names
- Examples: "UIButton", "UIEdit", "UIChannelSelect"

## Implementation Plan

### Phase 1: Core Infrastructure
1. Modify `DebugOverlay.h` data structures
2. Add `RegisterUIElement()` / `UnregisterUIElement()` methods
3. Implement `FindUIElementsAt()` with deduplication

### Phase 2: UIElement Integration
4. Add `GetDebugTypeName()` virtual method to UIElement
5. Implement auto-registration in `SetLayer()`
6. Implement auto-unregistration in destructor

### Phase 3: Display Implementation
7. Modify `OnMouseClick()` to use new flow
8. Implement new `RenderSelectionList()` for UI elements
9. Implement new `RenderInfoPopup()` with full UI info

### Phase 4: Derived Classes
10. Override `GetDebugTypeName()` in UIButton, UIEdit, UIChannelSelect
11. Add multi-layer registration in UIEdit::CreateLayer()
12. Test with various UI elements

### Phase 5: Polish
13. Add screen boundary adjustment
14. Add error handling for edge cases
15. Test deletion scenarios

## Testing Strategy

1. **Single UI element click:** Verify info popup shows correct information
2. **Overlapping UI elements:** Verify selection list appears
3. **Multi-layer element (UIEdit):** Verify all 3 layers are listed
4. **Parent-child hierarchy:** Verify parent/child information correct
5. **Element deletion:** Verify no crashes, popup closes properly
6. **Screen boundary:** Verify popup stays on screen
7. **ESC key:** Verify popup closes

## Design Principles

- **Debug-only:** All code within `#ifdef MS_DEBUG_CANVAS`
- **Zero-cost in release:** No performance impact when disabled
- **Minimal modifications:** Leverage existing infrastructure
- **Auto-registration:** Derived classes work without changes
- **Type safety:** Check pointers before use

## Future Enhancements (Not in Scope)

- Canvas WZ path alongside UI element info
- Interactive tree view of UI hierarchy
- Click-through to parent/children
- Copy debug info to clipboard
- Filter by UI element type
