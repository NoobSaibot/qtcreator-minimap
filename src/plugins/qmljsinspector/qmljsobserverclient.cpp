/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmljsobserverclient.h"
#include "qmljsclientproxy.h"
#include "qmljsinspectorconstants.h"

#include <QtGui/QColor>

using namespace QmlJSDebugger;

namespace QmlJSInspector {
namespace Internal {

QmlJSObserverClient::QmlJSObserverClient(QDeclarativeDebugConnection *client,
                                         QObject * /*parent*/)
    : QDeclarativeDebugClient(QLatin1String("QDeclarativeObserverMode"), client) ,
    m_connection(client)
{
}

void QmlJSObserverClient::statusChanged(Status status)
{
    emit connectedStatusChanged(status);
}

void QmlJSObserverClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    ObserverProtocol::Message type;
    ds >> type;

    switch (type) {
    case ObserverProtocol::CurrentObjectsChanged: {
        int objectCount;
        ds >> objectCount;

        log(LogReceive, type, QString("%1 [list of debug ids]").arg(objectCount));

        m_currentDebugIds.clear();

        for (int i = 0; i < objectCount; ++i) {
            int debugId;
            ds >> debugId;
            if (debugId != -1)
                m_currentDebugIds << debugId;
        }

        emit currentObjectsChanged(m_currentDebugIds);
        break;
    }
    case ObserverProtocol::ToolChanged: {
        int toolId;
        ds >> toolId;

        log(LogReceive, type, QString::number(toolId));

        if (toolId == Constants::ColorPickerMode) {
            emit colorPickerActivated();
        } else if (toolId == Constants::ZoomMode) {
            emit zoomToolActivated();
        } else if (toolId == Constants::SelectionToolMode) {
            emit selectToolActivated();
        } else if (toolId == Constants::MarqueeSelectionToolMode) {
            emit selectMarqueeToolActivated();
        }
        break;
    }
    case ObserverProtocol::AnimationSpeedChanged: {
        qreal slowdownFactor;
        ds >> slowdownFactor;

        log(LogReceive, type, QString::number(slowdownFactor));

        emit animationSpeedChanged(slowdownFactor);
        break;
    }
    case ObserverProtocol::SetDesignMode: {
        bool inDesignMode;
        ds >> inDesignMode;

        log(LogReceive, type, QLatin1String(inDesignMode ? "true" : "false"));

        emit designModeBehaviorChanged(inDesignMode);
        break;
    }
    case ObserverProtocol::ShowAppOnTop: {
        bool showAppOnTop;
        ds >> showAppOnTop;

        log(LogReceive, type, QLatin1String(showAppOnTop ? "true" : "false"));

        emit showAppOnTopChanged(showAppOnTop);
        break;
    }
    case ObserverProtocol::Reloaded: {
        log(LogReceive, type);
        emit reloaded();
        break;
    }
    case ObserverProtocol::ColorChanged: {
        QColor col;
        ds >> col;

        log(LogReceive, type, col.name());

        emit selectedColorChanged(col);
        break;
    }
    case ObserverProtocol::ContextPathUpdated: {
        QStringList contextPath;
        ds >> contextPath;

        log(LogReceive, type, contextPath.join(", "));

        emit contextPathUpdated(contextPath);
        break;
    }
    default:
        qWarning() << "Warning: Not handling message:" << type;
    }
}

QList<int> QmlJSObserverClient::currentObjects() const
{
    return m_currentDebugIds;
}

void QmlJSObserverClient::setCurrentObjects(const QList<int> &debugIds)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    if (debugIds == m_currentDebugIds)
        return;

    m_currentDebugIds = debugIds;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::SetCurrentObjects;
    ds << cmd
       << debugIds.length();

    foreach (int id, debugIds) {
        ds << id;
    }

    log(LogSend, cmd, QString("%1 [list of ids]").arg(debugIds.length()));

    sendMessage(message);
}

void recurseObjectIdList(const QDeclarativeDebugObjectReference &ref, QList<int> &debugIds, QList<QString> &objectIds)
{
    debugIds << ref.debugId();
    objectIds << ref.idString();
    foreach (const QDeclarativeDebugObjectReference &child, ref.children())
        recurseObjectIdList(child, debugIds, objectIds);
}

void QmlJSObserverClient::setObjectIdList(const QList<QDeclarativeDebugObjectReference> &objectRoots)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QList<int> debugIds;
    QList<QString> objectIds;

    foreach (const QDeclarativeDebugObjectReference &ref, objectRoots)
        recurseObjectIdList(ref, debugIds, objectIds);

    ObserverProtocol::Message cmd = ObserverProtocol::ObjectIdList;
    ds << cmd
       << debugIds.length();

    Q_ASSERT(debugIds.length() == objectIds.length());

    for(int i = 0; i < debugIds.length(); ++i) {
        ds << debugIds[i] << objectIds[i];
    }

    log(LogSend, cmd, QString("%1 %2 [list of debug / object ids]").arg(debugIds.length()));

    sendMessage(message);
}

void QmlJSObserverClient::setContextPathIndex(int contextPathIndex)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::SetContextPathIdx;
    ds << cmd
       << contextPathIndex;

    log(LogSend, cmd, QString::number(contextPathIndex));

    sendMessage(message);
}

void QmlJSObserverClient::clearComponentCache()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::ClearComponentCache;
    ds << cmd;

    log(LogSend, cmd);

    sendMessage(message);
}

void QmlJSObserverClient::reloadViewer()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::Reload;
    ds << cmd;

    log(LogSend, cmd);

    sendMessage(message);
}

void QmlJSObserverClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::SetDesignMode;
    ds << cmd
       << inDesignMode;

    log(LogSend, cmd, QLatin1String(inDesignMode ? "true" : "false"));

    sendMessage(message);
}

void QmlJSObserverClient::setAnimationSpeed(qreal slowdownFactor)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::SetAnimationSpeed;
    ds << cmd
       << slowdownFactor;


    log(LogSend, cmd, QString::number(slowdownFactor));

    sendMessage(message);
}

void QmlJSObserverClient::changeToColorPickerTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::ChangeTool;
    ObserverProtocol::Tool tool = ObserverProtocol::ColorPickerTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, ObserverProtocol::toString(tool));

    sendMessage(message);
}

void QmlJSObserverClient::changeToSelectTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::ChangeTool;
    ObserverProtocol::Tool tool = ObserverProtocol::SelectTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, ObserverProtocol::toString(tool));

    sendMessage(message);
}

void QmlJSObserverClient::changeToSelectMarqueeTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::ChangeTool;
    ObserverProtocol::Tool tool = ObserverProtocol::SelectMarqueeTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, ObserverProtocol::toString(tool));

    sendMessage(message);
}

void QmlJSObserverClient::changeToZoomTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::ChangeTool;
    ObserverProtocol::Tool tool = ObserverProtocol::ZoomTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, ObserverProtocol::toString(tool));

    sendMessage(message);
}

void QmlJSObserverClient::showAppOnTop(bool showOnTop)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::ShowAppOnTop;
    ds << cmd << showOnTop;

    log(LogSend, cmd, QLatin1String(showOnTop ? "true" : "false"));

    sendMessage(message);
}

void QmlJSObserverClient::createQmlObject(const QString &qmlText, int parentDebugId,
                                             const QStringList &imports, const QString &filename)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::CreateObject;
    ds << cmd
       << qmlText
       << parentDebugId
       << imports
       << filename;

    log(LogSend, cmd, QString("%1 %2 [%3] %4").arg(qmlText, QString::number(parentDebugId),
                                                   imports.join(","), filename));

    sendMessage(message);
}

void QmlJSObserverClient::destroyQmlObject(int debugId)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::DestroyObject;
    ds << cmd << debugId;

    log(LogSend, cmd, QString::number(debugId));

    sendMessage(message);
}

void QmlJSObserverClient::reparentQmlObject(int debugId, int newParent)
{
    if (!m_connection || !m_connection->isConnected())
        return;
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    ObserverProtocol::Message cmd = ObserverProtocol::MoveObject;
    ds << cmd
       << debugId
       << newParent;

    log(LogSend, cmd, QString("%1 %2").arg(QString::number(debugId),
                                           QString::number(newParent)));

    sendMessage(message);
}


void QmlJSObserverClient::applyChangesToQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSObserverClient::applyChangesFromQmlFile()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    // TODO
}

void QmlJSObserverClient::log(LogDirection direction, ObserverProtocol::Message message,
                              const QString &extra)
{
    QString msg;
    if (direction == LogSend)
        msg += QLatin1String(" sending ");
    else
        msg += QLatin1String(" receiving ");

    msg += ObserverProtocol::toString(message);
    msg += QLatin1Char(' ');
    msg += extra;
    emit logActivity(name(), msg);
}

} // namespace Internal
} // namespace QmlJSInspector
