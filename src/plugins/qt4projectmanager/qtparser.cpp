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

#include "qtparser.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/taskwindow.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::Task;

namespace {
    // opt. drive letter + filename: (2 brackets)
    const char * const FILE_PATTERN = "^(([A-Za-z]:)?[^:]+\\.[^:]+):";
}

QtParser::QtParser()
{
    m_mocRegExp.setPattern(QString::fromLatin1(FILE_PATTERN) + "(\\d+):\\s(Warning|Error):\\s(.+)$");
    m_mocRegExp.setMinimal(true);
}

void QtParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (m_mocRegExp.indexIn(lne) > -1) {
        bool ok;
        int lineno = m_mocRegExp.cap(3).toInt(&ok);
        if (!ok)
            lineno = -1;
        Task task(Task::Error,
                  m_mocRegExp.cap(5).trimmed(),
                  m_mocRegExp.cap(1) /* filename */,
                  lineno,
                  ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
        if (m_mocRegExp.cap(4) == QLatin1String("Warning"))
            task.type = Task::Warning;
        emit addTask(task);
        return;
    }
    IOutputParser::stdError(line);
}

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qt4projectmanagerplugin.h"
#   include <projectexplorer/projectexplorerconstants.h>
#   include <projectexplorer/metatypedeclarations.h>
#   include <projectexplorer/outputparser_test.h>

using namespace ProjectExplorer;

void Qt4ProjectManagerPlugin::testQtOutputParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext")
            << QList<ProjectExplorer::Task>()
            << QString();

    QTest::newRow("moc warning")
            << QString::fromLatin1("..\\untitled\\errorfile.h:0: Warning: No relevant classes found. No output generated.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>() << Task(Task::Warning,
                                                       QLatin1String("No relevant classes found. No output generated."),
                                                       QLatin1String("..\\untitled\\errorfile.h"), 0,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_COMPILE))
            << QString();
}

void Qt4ProjectManagerPlugin::testQtOutputParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new QtParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel, tasks, childStdOutLines, childStdErrLines, outputLines);
}
#endif