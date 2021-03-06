/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "nodeinstanceclientproxy.h"

#include <QLocalSocket>
#include <QVariant>
#include <QCoreApplication>
#include <QStringList>

#include "nodeinstanceserver.h"
#include "previewnodeinstanceserver.h"
#include "rendernodeinstanceserver.h"

#include "propertyabstractcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "removeinstancescommand.h"
#include "clearscenecommand.h"
#include "removepropertiescommand.h"
#include "reparentinstancescommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "addimportcommand.h"
#include "completecomponentcommand.h"
#include "synchronizecommand.h"

#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "valueschangedcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "componentcompletedcommand.h"

namespace QmlDesigner {

NodeInstanceClientProxy::NodeInstanceClientProxy(QObject *parent)
    : QObject(parent),
      m_nodeInstanceServer(0),
      m_blockSize(0),
      m_synchronizeId(-1)
{
    if (QCoreApplication::arguments().at(2) == QLatin1String("previewmode")) {
        m_nodeInstanceServer = new PreviewNodeInstanceServer(this);
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("editormode")) {
        m_nodeInstanceServer = new NodeInstanceServer(this);
    } else if (QCoreApplication::arguments().at(2) == QLatin1String("rendermode")) {
        m_nodeInstanceServer = new RenderNodeInstanceServer(this);
    }

    m_socket = new QLocalSocket(this);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readDataStream()));
    connect(m_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), QCoreApplication::instance(), SLOT(quit()));
    connect(m_socket, SIGNAL(disconnected()), QCoreApplication::instance(), SLOT(quit()));
    m_socket->connectToServer(QCoreApplication::arguments().at(1), QIODevice::ReadWrite | QIODevice::Unbuffered);
    m_socket->waitForConnected(-1);
}

void NodeInstanceClientProxy::writeCommand(const QVariant &command)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << quint32(0);
    out << command;
    out.device()->seek(0);
    out << quint32(block.size() - sizeof(quint32));

    m_socket->write(block);
}

void NodeInstanceClientProxy::informationChanged(const InformationChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::valuesChanged(const ValuesChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::pixmapChanged(const PixmapChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::childrenChanged(const ChildrenChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::statePreviewImagesChanged(const StatePreviewImageChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::componentCompleted(const ComponentCompletedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::flush()
{
}

void NodeInstanceClientProxy::synchronizeWithClientProcess()
{
    if (m_synchronizeId >= 0) {
        SynchronizeCommand synchronizeCommand(m_synchronizeId);
        writeCommand(QVariant::fromValue(synchronizeCommand));
    }
}

qint64 NodeInstanceClientProxy::bytesToWrite() const
{
    return m_socket->bytesToWrite();
}

void NodeInstanceClientProxy::readDataStream()
{
    QList<QVariant> commandList;

    while (!m_socket->atEnd()) {
        if (m_socket->bytesAvailable() < int(sizeof(quint32)))
            break;

        QDataStream in(m_socket);

        if (m_blockSize == 0) {
            in >> m_blockSize;
        }

        if (m_socket->bytesAvailable() < m_blockSize)
            break;

        QVariant command;
        in >> command;
        m_blockSize = 0;

        Q_ASSERT(in.status() == QDataStream::Ok);

        commandList.append(command);
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

NodeInstanceServerInterface *NodeInstanceClientProxy::nodeInstanceServer() const
{
    return m_nodeInstanceServer;
}

void NodeInstanceClientProxy::createInstances(const CreateInstancesCommand &command)
{
    nodeInstanceServer()->createInstances(command);
}

void NodeInstanceClientProxy::changeFileUrl(const ChangeFileUrlCommand &command)
{
    nodeInstanceServer()->changeFileUrl(command);
}

void NodeInstanceClientProxy::createScene(const CreateSceneCommand &command)
{
    nodeInstanceServer()->createScene(command);
}

void NodeInstanceClientProxy::clearScene(const ClearSceneCommand &command)
{
    nodeInstanceServer()->clearScene(command);
}

void NodeInstanceClientProxy::removeInstances(const RemoveInstancesCommand &command)
{
    nodeInstanceServer()->removeInstances(command);
}

void NodeInstanceClientProxy::removeProperties(const RemovePropertiesCommand &command)
{
    nodeInstanceServer()->removeProperties(command);
}

void NodeInstanceClientProxy::changePropertyBindings(const ChangeBindingsCommand &command)
{
    nodeInstanceServer()->changePropertyBindings(command);
}

void NodeInstanceClientProxy::changePropertyValues(const ChangeValuesCommand &command)
{
    nodeInstanceServer()->changePropertyValues(command);
}

void NodeInstanceClientProxy::reparentInstances(const ReparentInstancesCommand &command)
{
    nodeInstanceServer()->reparentInstances(command);
}

void NodeInstanceClientProxy::changeIds(const ChangeIdsCommand &command)
{
    nodeInstanceServer()->changeIds(command);
}

void NodeInstanceClientProxy::changeState(const ChangeStateCommand &command)
{
    nodeInstanceServer()->changeState(command);
}

void NodeInstanceClientProxy::addImport(const AddImportCommand &command)
{
    nodeInstanceServer()->addImport(command);
}

void NodeInstanceClientProxy::completeComponent(const CompleteComponentCommand &command)
{
    nodeInstanceServer()->completeComponent(command);
}

void NodeInstanceClientProxy::dispatchCommand(const QVariant &command)
{
    static const int createInstancesCommandType = QMetaType::type("CreateInstancesCommand");
    static const int changeFileUrlCommandType = QMetaType::type("ChangeFileUrlCommand");
    static const int createSceneCommandType = QMetaType::type("CreateSceneCommand");
    static const int clearSceneCommandType = QMetaType::type("ClearSceneCommand");
    static const int removeInstancesCommandType = QMetaType::type("RemoveInstancesCommand");
    static const int removePropertiesCommandType = QMetaType::type("RemovePropertiesCommand");
    static const int changeBindingsCommandType = QMetaType::type("ChangeBindingsCommand");
    static const int changeValuesCommandType = QMetaType::type("ChangeValuesCommand");
    static const int reparentInstancesCommandType = QMetaType::type("ReparentInstancesCommand");
    static const int changeIdsCommandType = QMetaType::type("ChangeIdsCommand");
    static const int changeStateCommandType = QMetaType::type("ChangeStateCommand");
    static const int addImportCommandType = QMetaType::type("AddImportCommand");
    static const int completeComponentCommandType = QMetaType::type("CompleteComponentCommand");
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");

    if (command.userType() ==  createInstancesCommandType) {
        createInstances(command.value<CreateInstancesCommand>());
    } else if (command.userType() ==  changeFileUrlCommandType)
        changeFileUrl(command.value<ChangeFileUrlCommand>());
    else if (command.userType() ==  createSceneCommandType)
        createScene(command.value<CreateSceneCommand>());
    else if (command.userType() ==  clearSceneCommandType)
        clearScene(command.value<ClearSceneCommand>());
    else if (command.userType() ==  removeInstancesCommandType)
        removeInstances(command.value<RemoveInstancesCommand>());
    else if (command.userType() ==  removePropertiesCommandType)
        removeProperties(command.value<RemovePropertiesCommand>());
    else if (command.userType() ==  changeBindingsCommandType)
        changePropertyBindings(command.value<ChangeBindingsCommand>());
    else if (command.userType() ==  changeValuesCommandType)
        changePropertyValues(command.value<ChangeValuesCommand>());
    else if (command.userType() ==  reparentInstancesCommandType)
        reparentInstances(command.value<ReparentInstancesCommand>());
    else if (command.userType() ==  changeIdsCommandType)
        changeIds(command.value<ChangeIdsCommand>());
    else if (command.userType() ==  changeStateCommandType)
        changeState(command.value<ChangeStateCommand>());
    else if (command.userType() ==  addImportCommandType)
        addImport(command.value<AddImportCommand>());
    else if (command.userType() ==  completeComponentCommandType)
        completeComponent(command.value<CompleteComponentCommand>());
    else if (command.userType() == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    } else
        Q_ASSERT(false);
}
} // namespace QmlDesigner
