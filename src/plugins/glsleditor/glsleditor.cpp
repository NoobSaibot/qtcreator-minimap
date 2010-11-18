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

#include "glsleditor.h"
#include "glsleditoreditable.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"
#include "glslhighlighter.h"

#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslengine.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/tooltip/tooltip.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <utils/changeset.h>
#include <utils/uncommentselection.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <QtGui/QMenu>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>

using namespace GLSL;
using namespace GLSLEditor;
using namespace GLSLEditor::Internal;

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 150
};

GLSLTextEditor::GLSLTextEditor(QWidget *parent) :
    TextEditor::BaseTextEditor(parent),
    m_outlineCombo(0)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    //setIndenter(new Indenter);

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));

    baseTextDocument()->setSyntaxHighlighter(new Highlighter(document()));

//    if (m_modelManager) {
//        m_semanticHighlighter->setModelManager(m_modelManager);
//        connect(m_modelManager, SIGNAL(documentUpdated(GLSL::Document::Ptr)),
//                this, SLOT(onDocumentUpdated(GLSL::Document::Ptr)));
//        connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,GLSL::LibraryInfo)),
//                this, SLOT(forceSemanticRehighlight()));
//        connect(this->document(), SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
//    }
}

GLSLTextEditor::~GLSLTextEditor()
{
}

int GLSLTextEditor::editorRevision() const
{
    //return document()->revision();
    return 0;
}

bool GLSLTextEditor::isOutdated() const
{
//    if (m_semanticInfo.revision() != editorRevision())
//        return true;

    return false;
}

Core::IEditor *GLSLEditorEditable::duplicate(QWidget *parent)
{
    GLSLTextEditor *newEditor = new GLSLTextEditor(parent);
    newEditor->duplicateFrom(editor());
    GLSLEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editableInterface();
}

QString GLSLEditorEditable::id() const
{
    return QLatin1String(GLSLEditor::Constants::C_GLSLEDITOR_ID);
}

bool GLSLEditorEditable::open(const QString &fileName)
{
    bool b = TextEditor::BaseTextEditorEditable::open(fileName);
    editor()->setMimeType(Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    return b;
}

Core::Context GLSLEditorEditable::context() const
{
    return m_context;
}

void GLSLTextEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    Highlighter *highlighter = qobject_cast<Highlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    /*
        NumberFormat,
        StringFormat,
        TypeFormat,
        KeywordFormat,
        LabelFormat,
        CommentFormat,
        VisualWhitespace,
     */
    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_NUMBER)
                   << QLatin1String(TextEditor::Constants::C_STRING)
                   << QLatin1String(TextEditor::Constants::C_TYPE)
                   << QLatin1String(TextEditor::Constants::C_KEYWORD)
                   << QLatin1String(TextEditor::Constants::C_FIELD)
                   << QLatin1String(TextEditor::Constants::C_COMMENT)
                   << QLatin1String(TextEditor::Constants::C_VISUAL_WHITESPACE);
    }

    highlighter->setFormats(fs.toTextCharFormats(categories));
    highlighter->rehighlight();
}

QString GLSLTextEditor::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    const QChar ch = characterAt(tc.position() - 1);
    // make sure that we're not at the start of the next word.
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word = tc.selectedText();
    return word;
}

TextEditor::BaseTextEditorEditable *GLSLTextEditor::createEditableInterface()
{
    GLSLEditorEditable *editable = new GLSLEditorEditable(this);
    createToolBar(editable);
    return editable;
}

void GLSLTextEditor::createToolBar(GLSLEditorEditable *editable)
{
    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);

    // ### m_outlineCombo->setModel(m_outlineModel);

    QTreeView *treeView = new QTreeView;
    treeView->header()->hide();
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    m_outlineCombo->setView(treeView);
    treeView->expandAll();

    //m_outlineCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_outlineCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_outlineCombo->setSizePolicy(policy);

    QToolBar *toolBar = static_cast<QToolBar*>(editable->toolBar());

    QList<QAction*> actions = toolBar->actions();
    toolBar->insertWidget(actions.first(), m_outlineCombo);
}

bool GLSLTextEditor::event(QEvent *e)
{
    return BaseTextEditor::event(e);
}

void GLSLTextEditor::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

void GLSLTextEditor::updateDocument()
{
    m_updateDocumentTimer->start();
}

void GLSLTextEditor::updateDocumentNow()
{
    m_updateDocumentTimer->stop();

    const int variant = Lexer::Variant_GLSL_Qt | // ### hardcoded
                        Lexer::Variant_GLSL_ES_100 |
                        Lexer::Variant_VertexShader |
                        Lexer::Variant_FragmentShader;
    const QString contents = toPlainText(); // get the code from the editor
    const QByteArray preprocessedCode = contents.toLatin1(); // ### use the QtCreator C++ preprocessor.

    Engine engine;
    Parser parser(&engine, preprocessedCode.constData(), preprocessedCode.size(), variant);
    TranslationUnit *ast = parser.parse();
    // ### process the ast
    (void) ast;
}