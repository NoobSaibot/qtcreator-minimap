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

#include "rubberbandselectionmanipulator.h"
#include "../qdeclarativeviewobserver_p.h"

#include <QDebug>

namespace QmlJSDebugger {

RubberBandSelectionManipulator::RubberBandSelectionManipulator(QGraphicsObject *layerItem, QDeclarativeViewObserver *editorView)
    : m_selectionRectangleElement(layerItem),
    m_editorView(editorView),
    m_beginFormEditorItem(0),
    m_isActive(false)
{
    m_selectionRectangleElement.hide();
}

void RubberBandSelectionManipulator::clear()
{
    m_selectionRectangleElement.clear();
    m_isActive = false;
    m_beginPoint = QPointF();
    m_itemList.clear();
    m_oldSelectionList.clear();
}

QGraphicsItem *RubberBandSelectionManipulator::topFormEditorItem(const QList<QGraphicsItem*> &itemList)
{
    if (itemList.isEmpty())
        return 0;

    return itemList.first();
}

void RubberBandSelectionManipulator::begin(const QPointF& beginPoint)
{
    m_beginPoint = beginPoint;
    m_selectionRectangleElement.setRect(m_beginPoint, m_beginPoint);
    m_selectionRectangleElement.show();
    m_isActive = true;
    m_beginFormEditorItem = topFormEditorItem(QDeclarativeViewObserverPrivate::get(m_editorView)->selectableItems(beginPoint));
    m_oldSelectionList = m_editorView->selectedItems();
}

void RubberBandSelectionManipulator::update(const QPointF& updatePoint)
{
    m_selectionRectangleElement.setRect(m_beginPoint, updatePoint);
}

void RubberBandSelectionManipulator::end()
{
    m_oldSelectionList.clear();
    m_selectionRectangleElement.hide();
    m_isActive = false;
}

void RubberBandSelectionManipulator::select(SelectionType selectionType)
{
    QList<QGraphicsItem*> itemList = QDeclarativeViewObserverPrivate::get(m_editorView)->selectableItems(m_selectionRectangleElement.rect(),
                                                                   Qt::IntersectsItemShape);
    QList<QGraphicsItem*> newSelectionList;

    foreach (QGraphicsItem* item, itemList) {
        if (item
            && item->parentItem()
            && !newSelectionList.contains(item)
            //&& m_beginFormEditorItem->childItems().contains(item) // TODO activate this test
            )
        {
            newSelectionList.append(item);
        }
    }

    if (newSelectionList.isEmpty() && m_beginFormEditorItem)
        newSelectionList.append(m_beginFormEditorItem);

    QList<QGraphicsItem*> resultList;

    switch(selectionType) {
    case AddToSelection: {
            resultList.append(m_oldSelectionList);
            resultList.append(newSelectionList);
        }
        break;
    case ReplaceSelection: {
            resultList.append(newSelectionList);
        }
        break;
    case RemoveFromSelection: {
            QSet<QGraphicsItem*> oldSelectionSet(m_oldSelectionList.toSet());
            QSet<QGraphicsItem*> newSelectionSet(newSelectionList.toSet());
            resultList.append(oldSelectionSet.subtract(newSelectionSet).toList());
        }
    }

    m_editorView->setSelectedItems(resultList);
}


void RubberBandSelectionManipulator::setItems(const QList<QGraphicsItem*> &itemList)
{
    m_itemList = itemList;
}

QPointF RubberBandSelectionManipulator::beginPoint() const
{
    return m_beginPoint;
}

bool RubberBandSelectionManipulator::isActive() const
{
    return m_isActive;

}

}