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

#ifndef CONTEXTPANEWIDGET_H
#define CONTEXTPANEWIDGET_H

#include <qmleditorwidgets_global.h>
#include <QFrame>
#include <QVariant>
#include <QGraphicsEffect>
#include <QWeakPointer>
#include <QToolButton>

namespace QmlJS {
    class PropertyReader;
}

namespace QmlEditorWidgets {

class CustomColorDialog;
class ContextPaneTextWidget;
class EasingContextPane;
class ContextPaneWidgetRectangle;
class ContextPaneWidgetImage;

class QMLEDITORWIDGETS_EXPORT DragWidget : public QFrame
{
    Q_OBJECT

public:
    explicit DragWidget(QWidget *parent = 0);
    void setSecondaryTarget(QWidget* w)
    { m_secondaryTarget = w; }

protected:
    QPoint m_pos;
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void virtual protectedMoved();

private:
    QGraphicsDropShadowEffect *m_dropShadowEffect;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPoint m_oldPos;
    QWeakPointer<QWidget> m_secondaryTarget;
};

class QMLEDITORWIDGETS_EXPORT ContextPaneWidget : public DragWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidget(QWidget *parent = 0);
    ~ContextPaneWidget();
    void activate(const QPoint &pos, const QPoint &alternative, const QPoint &alternative2, bool pinned);
    void rePosition(const QPoint &pos, const QPoint &alternative , const QPoint &alternative3, bool pinned);
    void deactivate();
    void setOptions(bool enabled, bool pinned);
    CustomColorDialog *colorDialog();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setPath(const QString &path);
    bool setType(const QStringList &types);
    bool acceptsType(const QStringList &types);
    QWidget* currentWidget() const { return m_currentWidget; }

public slots:
    void onTogglePane();
    void onShowColorDialog(bool, const QPoint &);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool);
    void pinnedChanged(bool);
    void enabledChanged(bool);

private slots:
    void onDisable(bool);
    void onResetPosition(bool toggle);

protected:

    void protectedMoved();

    QToolButton *m_toolButton;
    QWidget *createFontWidget();
    QWidget *createEasingWidget();
    QWidget *createImageWidget();
    QWidget *createBorderImageWidget();
    QWidget *createRectangleWidget();

    void setPinButton();
    void setLineButton();

private:
    QWidget *m_currentWidget;
    ContextPaneTextWidget *m_textWidget;
    EasingContextPane *m_easingWidget;
    ContextPaneWidgetImage *m_imageWidget;
    ContextPaneWidgetImage *m_borderImageWidget;
    ContextPaneWidgetRectangle *m_rectangleWidget;
    QWeakPointer<CustomColorDialog> m_bauhausColorDialog;
    QWeakPointer<QAction> m_resetAction;
    QWeakPointer<QAction> m_disableAction;
    QString m_colorName;
    QPoint m_originalPos;
    bool m_pinned;
};

} //QmlDesigner

#endif // CONTEXTPANEWIDGET_H