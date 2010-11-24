#ifndef NODEINSTANCESERVERPROXY_H
#define NODEINSTANCESERVERPROXY_H

#include "nodeinstanceserverinterface.h"

#include <QDataStream>
#include <QWeakPointer>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QProcess;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceClientInterface;
class NodeInstanceView;
class NodeInstanceClientProxy;

class NodeInstanceServerProxy : public NodeInstanceServerInterface
{
    Q_OBJECT
public:
    explicit NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView);
    ~NodeInstanceServerProxy();
    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void changeState(const ChangeStateCommand &command);

    void setBlockUpdates(bool block);

protected:
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command);
    NodeInstanceClientInterface *nodeInstanceClient() const;

signals:
    void processCrashed();

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readDataStream();

private:
    QWeakPointer<QLocalServer> m_localServer;
    QWeakPointer<QLocalSocket> m_socket;
    QWeakPointer<NodeInstanceView> m_nodeInstanceView;
    QWeakPointer<QProcess> m_qmlPuppetProcess;
    quint32 m_blockSize;
};

} // namespace QmlDesigner

#endif // NODEINSTANCESERVERPROXY_H