#include "qmloutlinemodel.h"
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QtCore/QDebug>
#include <typeinfo>

using namespace QmlJS;
using namespace QmlJSEditor::Internal;

enum {
    debug = false
};

namespace {

class QmlOutlineModelSync : protected AST::Visitor
{
public:
    QmlOutlineModelSync(QmlOutlineModel *model) :
        m_model(model),
        indent(0)
    {
    }

    void operator()(Document::Ptr doc)
    {
        m_nodeToIndex.clear();

        if (debug)
            qDebug() << "QmlOutlineModel ------";
        if (doc && doc->ast())
            doc->ast()->accept(this);
    }

private:
    bool preVisit(AST::Node *node)
    {
        if (!node)
            return false;
        if (debug)
            qDebug() << "QmlOutlineModel -" << QByteArray(indent++, '-').constData() << node << typeid(*node).name();
        return true;
    }

    void postVisit(AST::Node *)
    {
        indent--;
    }

    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (id->name)
                text += id->name->asString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }



    bool visit(AST::UiObjectDefinition *objDef)
    {
        AST::SourceLocation location;
        location.offset = objDef->firstSourceLocation().offset;
        location.length = objDef->lastSourceLocation().offset
                - objDef->firstSourceLocation().offset
                + objDef->lastSourceLocation().length;

        const QString typeName = asString(objDef->qualifiedTypeNameId);
        const QString id = getId(objDef);
        QModelIndex index = m_model->enterElement(asString(objDef->qualifiedTypeNameId), id, location);
        m_nodeToIndex.insert(objDef, index);
        return true;
    }

    void endVisit(AST::UiObjectDefinition * /*objDefinition*/)
    {
        m_model->leaveElement();
    }

    bool visit(AST::UiScriptBinding *scriptBinding)
    {
        AST::SourceLocation location;
        location.offset = scriptBinding->firstSourceLocation().offset;
        location.length = scriptBinding->lastSourceLocation().offset
                - scriptBinding->firstSourceLocation().offset
                + scriptBinding->lastSourceLocation().length;

        QModelIndex index = m_model->enterProperty(asString(scriptBinding->qualifiedId), location);
        m_nodeToIndex.insert(scriptBinding, index);

        return true;
    }

    void endVisit(AST::UiScriptBinding * /*scriptBinding*/)
    {
        m_model->leaveProperty();
    }

    QString getId(AST::UiObjectDefinition *objDef) {
        QString id;
        for (AST::UiObjectMemberList *it = objDef->initializer->members; it; it = it->next) {
            if (AST::UiScriptBinding *binding = dynamic_cast<AST::UiScriptBinding*>(it->member)) {
                if (binding->qualifiedId->name->asString() == "id") {
                    AST::ExpressionStatement *expr = dynamic_cast<AST::ExpressionStatement*>(binding->statement);
                    if (!expr)
                        continue;
                    AST::IdentifierExpression *idExpr = dynamic_cast<AST::IdentifierExpression*>(expr->expression);
                    if (!idExpr)
                        continue;
                    id = idExpr->name->asString();
                    break;
                }
            }
        }
        return id;
    }


    QmlOutlineModel *m_model;
    QHash<AST::Node*, QModelIndex> m_nodeToIndex;
    int indent;
};


} // namespace

namespace QmlJSEditor {
namespace Internal {

QmlOutlineModel::QmlOutlineModel(QObject *parent) :
    QStandardItemModel(parent)
{
}

void QmlOutlineModel::update(QmlJS::Document::Ptr doc)
{
    m_treePos.clear();
    m_treePos.append(0);
    m_currentItem = invisibleRootItem();

    QmlOutlineModelSync syncModel(this);
    syncModel(doc);

    emit updated();
}

QModelIndex QmlOutlineModel::enterElement(const QString &type, const QString &id, const AST::SourceLocation &sourceLocation)
{
    QStandardItem *item = enterNode(sourceLocation);
    if (!id.isEmpty()) {
        item->setText(id);
    } else {
        item->setText(type);
    }
    item->setToolTip(type);
    item->setIcon(m_icons.objectDefinitionIcon());
    return item->index();
}

void QmlOutlineModel::leaveElement()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterProperty(const QString &name, const AST::SourceLocation &sourceLocation)
{
    QStandardItem *item = enterNode(sourceLocation);
    item->setText(name);
    item->setIcon(m_icons.scriptBindingIcon());
    return item->index();
}

void QmlOutlineModel::leaveProperty()
{
    leaveNode();
}

QStandardItem *QmlOutlineModel::enterNode(const QmlJS::AST::SourceLocation &location)
{
    int siblingIndex = m_treePos.last();
    if (siblingIndex == 0) {
        // first child
        if (!m_currentItem->hasChildren()) {
            QStandardItem *parentItem = m_currentItem;
            m_currentItem = new QStandardItem;
            m_currentItem->setEditable(false);
            parentItem->appendRow(m_currentItem);
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << parentItem->text();
        } else {
            m_currentItem = m_currentItem->child(0);
        }
    } else {
        // sibling
        if (m_currentItem->rowCount() <= siblingIndex) {
            // attach
            QStandardItem *oldItem = m_currentItem;
            m_currentItem = new QStandardItem;
            m_currentItem->setEditable(false);
            oldItem->appendRow(m_currentItem);
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << oldItem->text();
        } else {
            m_currentItem = m_currentItem->child(siblingIndex);
        }
    }

    m_treePos.append(0);
    m_currentItem->setData(QVariant::fromValue(location), SourceLocationRole);

    return m_currentItem;
}

void QmlOutlineModel::leaveNode()
{
    int lastIndex = m_treePos.takeLast();


    if (lastIndex > 0) {
        // element has children
        if (lastIndex < m_currentItem->rowCount()) {
            if (debug)
                qDebug() << "QmlOutlineModel - removeRows from " << m_currentItem->text() << lastIndex << m_currentItem->rowCount() - lastIndex;
            m_currentItem->removeRows(lastIndex, m_currentItem->rowCount() - lastIndex);
        }
        m_currentItem = parentItem();
    } else {
        if (m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - removeRows from " << m_currentItem->text() << 0 << m_currentItem->rowCount();
            m_currentItem->removeRows(0, m_currentItem->rowCount());
        }
        m_currentItem = parentItem();
    }


    m_treePos.last()++;
}

QStandardItem *QmlOutlineModel::parentItem()
{
    QStandardItem *parent = m_currentItem->parent();
    if (!parent)
        parent = invisibleRootItem();
    return parent;
}

} // namespace Internal
} // namespace QmlJSEditor
