/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmltransitionnodeinstance.h"
#include <private/qdeclarativetransition_p.h>
#include <nodemetainfo.h>
#include "invalidnodeinstanceexception.h"

namespace QmlDesigner {
namespace Internal {

QmlTransitionNodeInstance::QmlTransitionNodeInstance(QDeclarativeTransition *transition)
    : ObjectNodeInstance(transition)
{
}

QmlTransitionNodeInstance::Pointer QmlTransitionNodeInstance::create(QObject *object)
{
     QDeclarativeTransition *transition = qobject_cast<QDeclarativeTransition*>(object);
     if (transition == 0)
         throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

     Pointer instance(new QmlTransitionNodeInstance(transition));

     instance->populateResetValueHash();

     transition->setToState("invalidState");
     transition->setFromState("invalidState");

     return instance;
}

bool QmlTransitionNodeInstance::isTransition() const
{
    return true;
}

void QmlTransitionNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "from" || name == "to")
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

QDeclarativeTransition *QmlTransitionNodeInstance::qmlTransition() const
{
    Q_ASSERT(qobject_cast<QDeclarativeTransition*>(object()));
    return static_cast<QDeclarativeTransition*>(object());
}
}
}
