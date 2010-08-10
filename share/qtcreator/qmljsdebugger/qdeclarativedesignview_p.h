/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QDECLARATIVEDESIGNVIEW_P_H
#define QDECLARATIVEDESIGNVIEW_P_H

#include <QWeakPointer>
#include <QPointF>
#include <QTimer>

#include "qdeclarativedesignview.h"

namespace QmlViewer {

class QDeclarativeDesignView;
class SelectionTool;
class ZoomTool;
class ColorPickerTool;
class LayerItem;
class BoundingRectHighlighter;
class SubcomponentEditorTool;
class QmlToolbar;
class CrumblePath;
class AbstractFormEditorTool;

class QDeclarativeDesignViewPrivate
{

public:

    enum ContextFlags {
        IgnoreContext,
        ContextSensitive
    };

    QDeclarativeDesignViewPrivate(QDeclarativeDesignView *);
    ~QDeclarativeDesignViewPrivate();

    QDeclarativeDesignView *q;
    QPointF cursorPos;
    QList<QWeakPointer<QGraphicsObject> > currentSelection;

    Constants::DesignTool currentToolMode;
    AbstractFormEditorTool *currentTool;

    SelectionTool *selectionTool;
    ZoomTool *zoomTool;
    ColorPickerTool *colorPickerTool;
    SubcomponentEditorTool *subcomponentEditorTool;
    LayerItem *manipulatorLayer;

    BoundingRectHighlighter *boundingRectHighlighter;

    bool designModeBehavior;

    bool executionPaused;
    qreal slowdownFactor;

    QmlToolbar *toolbar;
    int sceneChangedTimerRestartCount;
    QTimer sceneChangedTimer;
    QSet<QGraphicsObject *> sceneGraphicsObjects;

    void clearEditorItems();
    void createToolbar();
    void changeToSelectTool();
    QList<QGraphicsItem*> filterForCurrentContext(QList<QGraphicsItem*> &itemlist) const;
    QList<QGraphicsItem*> filterForSelection(QList<QGraphicsItem*> &itemlist) const;

    QList<QGraphicsItem*> selectableItems(const QPoint &pos) const;
    QList<QGraphicsItem*> selectableItems(const QPointF &scenePos) const;
    QList<QGraphicsItem*> selectableItems(const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const;

    bool hasNewGraphicsObjects(QGraphicsObject *object);

    void setSelectedItemsForTools(QList<QGraphicsItem *> items);
    void setSelectedItems(QList<QGraphicsItem *> items);
    QList<QGraphicsItem *> selectedItems();

    void changeTool(Constants::DesignTool tool,
                    Constants::ToolFlags flags = Constants::NoToolFlags);

    void clearHighlight();
    void highlight(QList<QGraphicsObject *> item, ContextFlags flags = ContextSensitive);
    void highlight(QGraphicsObject *item, ContextFlags flags = ContextSensitive);

    bool mouseInsideContextItem() const;
    bool isEditorItem(QGraphicsItem *item) const;

    QGraphicsItem *currentRootItem() const;


    void _q_reloadView();
    void _q_onStatusChanged(QDeclarativeView::Status status);
    void _q_onCurrentObjectsChanged(QList<QObject*> objects);
    void _q_applyChangesFromClient();
    void _q_createQmlObject(const QString &qml, QObject *parent,
                            const QStringList &imports, const QString &filename = QString());

    void _q_changeToSingleSelectTool();
    void _q_changeToMarqueeSelectTool();
    void _q_changeToZoomTool();
    void _q_changeToColorPickerTool();
    void _q_changeContextPathIndex(int index);
    void _q_clearComponentCache();
    void _q_sceneChanged(const QList<QRectF> &areas);
    void _q_changeDebugObjectCount(int objectCount);
    void _q_checkSceneItemCount();

    static QDeclarativeDesignViewPrivate *get(QDeclarativeDesignView *v) { return v->d_func(); }
};

} // namespace QmlViewer

#endif // QDECLARATIVEDESIGNVIEW_P_H