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

#ifndef QMLPROJECTFILE_H
#define QMLPROJECTFILE_H

#include <coreplugin/ifile.h>

namespace QmlProjectManager {

class QmlProject;

namespace Internal {

class QmlProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    QmlProjectFile(QmlProject *parent, QString fileName);
    virtual ~QmlProjectFile();

    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const;
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);

private:
    QmlProject *m_project;
    QString m_fileName;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTFILE_H
