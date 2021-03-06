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

#ifndef PROJECTFILEWIZARDEXTENSION2_H
#define PROJECTFILEWIZARDEXTENSION2_H

#include <coreplugin/ifilewizardextension.h>

namespace ProjectExplorer {

namespace Internal {

struct ProjectWizardContext;

/* Final file wizard processing steps:
 * 1) Add to a project file (*.pri/ *.pro)
 * 2) Initialize a version control repository (unless the path is already
 *    managed) and do 'add' if the VCS supports it.  */
class ProjectFileWizardExtension : public Core::IFileWizardExtension
{
    Q_OBJECT
public:
    explicit ProjectFileWizardExtension();
    virtual ~ProjectFileWizardExtension();

    virtual QList<QWizardPage *> extensionPages(const Core::IWizard *wizard);
    virtual bool process(const QList<Core::GeneratedFile> &files,
                         bool *removeOpenProjectAttribute, QString *errorMessage);

public slots:
    virtual void firstExtensionPageShown(const QList<Core::GeneratedFile> &files);

private:
    void initProjectChoices(const QString &generatedProjectFilePath);
    void initializeVersionControlChoices();
    bool processProject(const QList<Core::GeneratedFile> &files,
                        bool *removeOpenProjectAttribute, QString *errorMessage);
    bool processVersionControl(const QList<Core::GeneratedFile> &files, QString *errorMessage);

    ProjectWizardContext *m_context;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTFILEWIZARDEXTENSION2_H
