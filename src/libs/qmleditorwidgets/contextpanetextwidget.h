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

#ifndef CONTEXTPANETEXTWIDGET_H
#define CONTEXTPANETEXTWIDGET_H

#include <qmleditorwidgets_global.h>
#include <QWidget>
#include <QVariant>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ContextPaneTextWidget;
}
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

namespace QmlEditorWidgets {

class CustomColorDialog;

class QMLEDITORWIDGETS_EXPORT ContextPaneTextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneTextWidget(QWidget *parent = 0);
    ~ContextPaneTextWidget();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setVerticalAlignmentVisible(bool);
    void setStyleVisible(bool);

public slots:
    void onTextColorButtonToggled(bool);
    void onColorButtonToggled(bool);
    void onColorDialogApplied(const QColor &color);
    void onColorDialogCancled();
    void onFontSizeChanged(int value);
    void onFontFormatChanged();
    void onBoldCheckedChanged(bool value);
    void onItalicCheckedChanged(bool value);
    void onUnderlineCheckedChanged(bool value);
    void onStrikeoutCheckedChanged(bool value);
    void onCurrentFontChanged(const QFont &font);
    void onHorizontalAlignmentChanged();
    void onVerticalAlignmentChanged();
    void onStyleComboBoxChanged(const QString &style);


signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

protected:
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *event);

private:
    Ui::ContextPaneTextWidget *ui;
    QString m_verticalAlignment;
    QString m_horizontalAlignment;
    int m_fontSizeTimer;
};

} //QmlDesigner

#endif // CONTEXTPANETEXTWIDGET_H