#ifndef QMLVIEWERCONSTANTS_H
#define QMLVIEWERCONSTANTS_H

namespace QmlViewer {
namespace Constants {

enum DesignTool {
    NoTool = 0,
    SelectionToolMode = 1,
    MarqueeSelectionToolMode = 2,
    MoveToolMode = 3,
    ResizeToolMode = 4,
    ColorPickerMode = 5,
    ZoomMode = 6
};

enum ToolFlags {
    NoToolFlags = 0,
    UseCursorPos = 1
};

static const int DragStartTime = 50;

static const int DragStartDistance = 20;

static const double ZoomSnapDelta = 0.04;

static const int EditorItemDataKey = 1000;

enum GraphicsItemTypes {
    EditorItemType = 0xEAAA,
    ResizeHandleItemType = 0xEAEA
};


} // namespace Constants
} // namespace QmlViewer

#endif // QMLVIEWERCONSTANTS_H