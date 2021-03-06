/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "analyzermanager.h"
#include "analyzerconstants.h"

#include "ianalyzertool.h"
#include "analyzerplugin.h"
#include "analyzerruncontrol.h"
#include "analyzeroptionspage.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/imode.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/target.h>

#include <extensionsystem/iplugin.h>

#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDockWidget>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QToolButton>
#include <QtGui/QComboBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QDialog>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QDialogButtonBox>

using namespace Core;
using namespace Analyzer;
using namespace Analyzer::Internal;

namespace Analyzer {
namespace Internal {

class DockWidgetEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit DockWidgetEventFilter(QObject *parent = 0) : QObject(parent) {}

signals:
    void widgetResized();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
};

bool DockWidgetEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Resize:
    case QEvent::ZOrderChange:
        emit widgetResized();
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

// AnalyzerMode ////////////////////////////////////////////////////

class AnalyzerMode : public Core::IMode
{
    Q_OBJECT

public:
    AnalyzerMode(QObject *parent = 0)
        : Core::IMode(parent)
        , m_widget(0)
    {}

    ~AnalyzerMode()
    {
        // Make sure the editor manager does not get deleted.
        if (m_widget) {
            delete m_widget;
            m_widget = 0;
        }
        Core::EditorManager::instance()->setParent(0);
    }

    QString displayName() const { return tr("Analyze"); }
    QIcon icon() const { return QIcon(":/images/analyzer_mode.png"); }
    int priority() const { return Constants::P_MODE_ANALYZE; }
    QWidget *widget() { return m_widget; }
    QString id() const { return QLatin1String(Constants::MODE_ANALYZE); }
    QString type() const { return Core::Constants::MODE_EDIT_TYPE; }
    Core::Context context() const
    {
        return Core::Context(Core::Constants::C_EDITORMANAGER, Constants::C_ANALYZEMODE,
                             Core::Constants::C_NAVIGATION_PANE);
    }
    QString contextHelpId() const { return QString(); }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QWidget *m_widget;
};

} // namespace Internal
} // namespace Analyzer

// AnalyzerManagerPrivate ////////////////////////////////////////////////////
class AnalyzerManager::AnalyzerManagerPrivate
{
public:
    AnalyzerManagerPrivate(AnalyzerManager *qq);
    ~AnalyzerManagerPrivate();

    /**
     * After calling this, a proper instance of Core::IMore is initialized
     * It is delayed since an analyzer mode makes no sense without analyzer tools
     *
     * \note Call this before adding a tool to the manager
     */
    void delayedInit();

    void setupActions();
    QWidget *createContents();
    QWidget *createMainWindow();

    void addDock(IAnalyzerTool *tool, Qt::DockWidgetArea area, QDockWidget *dockWidget);
    void startTool();

    AnalyzerManager *q;
    AnalyzerMode *m_mode;
    AnalyzerRunControlFactory *m_runControlFactory;
    ProjectExplorer::RunControl *m_currentRunControl;
    Utils::FancyMainWindow *m_mainWindow;
    QList<IAnalyzerTool*> m_tools;
    QActionGroup *m_toolGroup;
    QAction *m_startAction;
    QAction *m_stopAction;
    QMenu *m_menu;
    QComboBox *m_toolBox;
    Utils::StyledSeparator *m_toolBoxSeparator;
    ActionContainer *m_viewsMenu;
    typedef QPair<Qt::DockWidgetArea, QDockWidget*> ToolWidgetPair;
    typedef QList<ToolWidgetPair> ToolWidgetPairList;
    QMap<IAnalyzerTool*, ToolWidgetPairList> m_toolWidgets;
    DockWidgetEventFilter *m_resizeEventFilter;
    QMap<IAnalyzerTool*, QWidget*> m_toolToolbarWidgets;
    QStackedWidget *m_toolbarStackedWidget;
    QMap<IAnalyzerTool *, QSettings *> m_defaultSettings;

    // list of dock widgets to prevent memory leak
    typedef QWeakPointer<QDockWidget> DockPtr;
    QList<DockPtr> m_dockWidgets;

    bool m_initialized;
};

AnalyzerManager::AnalyzerManagerPrivate::AnalyzerManagerPrivate(AnalyzerManager *qq):
    q(qq),
    m_mode(0),
    m_runControlFactory(0),
    m_currentRunControl(0),
    m_mainWindow(0),
    m_toolGroup(0),
    m_startAction(0),
    m_stopAction(0),
    m_menu(0),
    m_toolBox(0),
    m_toolBoxSeparator(0),
    m_viewsMenu(0),
    m_resizeEventFilter(new DockWidgetEventFilter(qq)),
    m_toolbarStackedWidget(0),
    m_initialized(false)
{
    m_runControlFactory = new AnalyzerRunControlFactory();
    AnalyzerPlugin::instance()->addAutoReleasedObject(m_runControlFactory);
    connect(m_runControlFactory, SIGNAL(runControlCreated(Analyzer::Internal::AnalyzerRunControl *)),
            q, SLOT(runControlCreated(Analyzer::Internal::AnalyzerRunControl *)));

    setupActions();
}

AnalyzerManager::AnalyzerManagerPrivate::~AnalyzerManagerPrivate()
{
    // as we have to setParent(0) on dock widget that are not selected,
    // we keep track of all and make sure we don't leak any
    foreach(const DockPtr &ptr, m_dockWidgets) {
        if (ptr)
            delete ptr.data();
    }
}

void AnalyzerManager::AnalyzerManagerPrivate::setupActions()
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *mtools = am->actionContainer(ProjectExplorer::Constants::M_DEBUG);
    Core::ActionContainer *mAnalyzermenu = am->createMenu(Constants::M_TOOLS_ANALYZER);

    // Menus
    m_menu = mAnalyzermenu->menu();
    m_menu->setTitle(tr("Start &Analyzer"));
    m_menu->setEnabled(true);
    mtools->addMenu(mAnalyzermenu);

    m_toolGroup = new QActionGroup(m_menu);
    connect(m_toolGroup, SIGNAL(triggered(QAction*)),
            q, SLOT(toolSelected(QAction*)));

    const Core::Context globalcontext(Core::Constants::C_GLOBAL);

    m_startAction = new QAction(tr("Start"), m_menu);
    m_startAction->setIcon(QIcon(QLatin1String(":/images/analyzer_start_small.png")));
    Core::Command *command = am->registerAction(m_startAction,
                                                Constants::START, globalcontext);
    mAnalyzermenu->addAction(command);
    connect(m_startAction, SIGNAL(triggered()), q, SLOT(startTool()));

    m_stopAction = new QAction(tr("Stop"), m_menu);
    m_stopAction->setEnabled(false);
    m_stopAction->setIcon(QIcon(QLatin1String(":/debugger/images/debugger_stop_small.png")));
    command = am->registerAction(m_stopAction, Constants::STOP, globalcontext);
    mAnalyzermenu->addAction(command);
    connect(m_stopAction, SIGNAL(triggered()), q, SLOT(stopTool()));

    m_menu->addSeparator();

    m_viewsMenu = am->actionContainer(Core::Id(Core::Constants::M_WINDOW_VIEWS));
}

void AnalyzerManager::AnalyzerManagerPrivate::delayedInit()
{
    if (m_initialized)
        return;

    m_mode = new AnalyzerMode(q);
    m_mode->setWidget(createContents());
    AnalyzerPlugin::instance()->addAutoReleasedObject(m_mode);
    m_initialized = true;
}

QWidget *AnalyzerManager::AnalyzerManagerPrivate::createContents()
{
    // right-side window with editor, output etc.
    MiniSplitter *mainWindowSplitter = new MiniSplitter;
    mainWindowSplitter->addWidget(createMainWindow());
    mainWindowSplitter->addWidget(new OutputPanePlaceHolder(m_mode, mainWindowSplitter));
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // navigation + right-side window
    MiniSplitter *splitter = new MiniSplitter;
    splitter->addWidget(new NavigationWidgetPlaceHolder(m_mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

QWidget *AnalyzerManager::AnalyzerManagerPrivate::createMainWindow()
{
    m_mainWindow = new Utils::FancyMainWindow();
    connect(m_mainWindow, SIGNAL(resetLayout()),
            q, SLOT(resetLayout()));
    m_mainWindow->setDocumentMode(true);
    m_mainWindow->setDockNestingEnabled(true);
    m_mainWindow->setDockActionsVisible(ModeManager::instance()->currentMode()->id() ==
                                        Constants::MODE_ANALYZE);

    QBoxLayout *editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    QWidget *editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(new EditorManagerPlaceHolder(m_mode));
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

    MiniSplitter *documentAndRightPane = new MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new RightPanePlaceHolder(m_mode));
    documentAndRightPane->setStretchFactor(0, 1);
    documentAndRightPane->setStretchFactor(1, 0);

    Utils::StyledBar *analyzeToolBar = new Utils::StyledBar;
    analyzeToolBar->setProperty("topBorder", true);
    QHBoxLayout *analyzeToolBarLayout = new QHBoxLayout(analyzeToolBar);
    analyzeToolBarLayout->setMargin(0);
    analyzeToolBarLayout->setSpacing(0);
    QToolButton *startButton = toolButton(m_startAction);
    analyzeToolBarLayout->addWidget(startButton);
    analyzeToolBarLayout->addWidget(toolButton(m_stopAction));
    analyzeToolBarLayout->addWidget(new Utils::StyledSeparator);
    m_toolBox = new QComboBox;
    connect(m_toolBox, SIGNAL(currentIndexChanged(int)),
            q, SLOT(toolSelected(int)));
    analyzeToolBarLayout->addWidget(m_toolBox);
    m_toolBoxSeparator = new Utils::StyledSeparator;
    analyzeToolBarLayout->addWidget(m_toolBoxSeparator);
    m_toolbarStackedWidget = new QStackedWidget;
    analyzeToolBarLayout->addWidget(m_toolbarStackedWidget);
    analyzeToolBarLayout->addStretch();

    QDockWidget *dock = new QDockWidget(tr("Analyzer Toolbar"));
    dock->setObjectName(QLatin1String("Analyzer Toolbar"));
    dock->setWidget(analyzeToolBar);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    // hide title bar
    dock->setTitleBarWidget(new QWidget(dock));
    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dock);
    m_mainWindow->setToolBarDockWidget(dock);

    QWidget *centralWidget = new QWidget;
    m_mainWindow->setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);

    return m_mainWindow;
}

void AnalyzerManager::AnalyzerManagerPrivate::addDock(IAnalyzerTool *tool, Qt::DockWidgetArea area,
                                                      QDockWidget *dockWidget)
{
    QTC_ASSERT(tool == q->currentTool(), return)

    dockWidget->setParent(m_mainWindow);
    m_mainWindow->addDockWidget(area, dockWidget);

    Context globalContext(Core::Constants::C_GLOBAL);

    ActionManager *am = ICore::instance()->actionManager();
    QAction *toggleViewAction = dockWidget->toggleViewAction();
    toggleViewAction->setText(dockWidget->windowTitle());
    Command *cmd = am->registerAction(toggleViewAction, QString("Analyzer." + dockWidget->objectName()),
                                      globalContext);
    cmd->setAttribute(Command::CA_Hide);
    m_viewsMenu->addAction(cmd);

    /* TODO: save settings
    connect(dockWidget->toggleViewAction(), SIGNAL(triggered(bool)),
        SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
        SLOT(updateDockWidgetSettings()));
    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
        SLOT(updateDockWidgetSettings()));
    */

    // just add the dock below the toolbar by default
    m_mainWindow->splitDockWidget(m_mainWindow->toolBarDockWidget(), dockWidget,
                                  Qt::Vertical);
    dockWidget->show();
}

bool buildTypeAcceppted(IAnalyzerTool::ToolMode toolMode,
                        ProjectExplorer::BuildConfiguration::BuildType buildType)
{
    if (toolMode == IAnalyzerTool::AnyMode)
        return true;
    else if (buildType == ProjectExplorer::BuildConfiguration::Unknown)
        return true;
    else if (buildType == ProjectExplorer::BuildConfiguration::Debug &&
             toolMode == IAnalyzerTool::DebugMode)
        return true;
    else if (buildType == ProjectExplorer::BuildConfiguration::Release &&
             toolMode == IAnalyzerTool::ReleaseMode)
        return true;
    else
        return false;
}

void AnalyzerManager::AnalyzerManagerPrivate::startTool()
{
    QTC_ASSERT(!m_currentRunControl, return);

    // make sure our mode is shown
    ModeManager::instance()->activateMode(m_mode->id());

    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();

    ProjectExplorer::Project *pro = pe->startupProject();
    // ### not sure if we're supposed to check if the RunConFiguration isEnabled
    if (!pro || !pro->activeTarget()->activeRunConfiguration()->isEnabled())
        return;

    ProjectExplorer::BuildConfiguration::BuildType buildType = pro->activeTarget()->activeBuildConfiguration()->buildType();
    IAnalyzerTool::ToolMode toolMode = q->currentTool()->mode();

    // check the project for whether the build config is in the correct mode
    // if not, notify the user and urge him to use the correct mode
    if (!buildTypeAcceppted(toolMode, buildType))
    {
        const QString &toolName = q->currentTool()->displayName();
        const QString &toolMode = q->currentTool()->modeString();
        const QString currentMode = buildType == ProjectExplorer::BuildConfiguration::Debug ? tr("Debug") : tr("Release");

        QSettings *settings = Core::ICore::instance()->settings();
        const QString configKey = QString("%1/%2").arg(Constants::MODE_ANALYZE, "AnalyzeCorrectMode");
        int ret;
        if (settings->contains(configKey)) {
            ret = settings->value(configKey, QDialog::Accepted).toInt();
        } else {
            QDialog dialog;
            dialog.setWindowTitle(tr("Run %1 in %2 mode?").arg(toolName).arg(currentMode));
            QGridLayout *layout = new QGridLayout;
            QLabel *iconLabel = new QLabel;
            iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            QIcon icon = dialog.style()->standardIcon(QStyle::SP_MessageBoxInformation);
            dialog.setWindowIcon(icon);
            iconLabel->setPixmap(icon.pixmap(QSize(icon.actualSize(QSize(64, 64)))));
            layout->addWidget(iconLabel, 0, 0);
            QLabel *textLabel = new QLabel;
            textLabel->setWordWrap(true);
            textLabel->setText(tr("You are trying to run %1 on an application in %2 mode. "
                                "%1 is designed to be used in %3 mode.\n\n"
                                "Do you want to continue and run %1 in %2 mode?").arg(toolName).arg(currentMode).arg(toolMode));
            layout->addWidget(textLabel, 0, 1);
            QCheckBox *dontAskAgain = new QCheckBox;
            dontAskAgain->setText(tr("&Do not ask again"));
            layout->addWidget(dontAskAgain, 1, 0, 1, 2);
            QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::Cancel);
            connect(buttons, SIGNAL(accepted()),
                    &dialog, SLOT(accept()));
            connect(buttons, SIGNAL(rejected()),
                    &dialog, SLOT(reject()));
            layout->addWidget(buttons, 2, 0, 1, 2);
            dialog.setLayout(layout);
            ret = dialog.exec();
            if (dontAskAgain->isChecked() && ret == QDialog::Accepted)
                settings->setValue(configKey, ret);
        }
        if (ret == QDialog::Rejected)
            return;
    }

    pe->runProject(pro, Constants::MODE_ANALYZE);

    m_startAction->setEnabled(false);
    m_stopAction->setEnabled(true);
    m_toolBox->setEnabled(false);
    m_toolGroup->setEnabled(false);
}
// AnalyzerManager ////////////////////////////////////////////////////
AnalyzerManager *AnalyzerManager::m_instance = 0;

AnalyzerManager::AnalyzerManager(QObject *parent) :
    QObject(parent),
    d(new AnalyzerManagerPrivate(this))
{
    m_instance = this;

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(modeChanged(Core::IMode*)));
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(pe, SIGNAL(updateRunActions()),
            this, SLOT(updateRunActions()));
}

AnalyzerManager::~AnalyzerManager()
{
    delete d;
}

bool AnalyzerManager::isInitialized() const
{
    return d->m_initialized;
}

void AnalyzerManager::shutdown()
{
    saveToolSettings(currentTool());
}

AnalyzerManager * AnalyzerManager::instance()
{
    return m_instance;
}

void AnalyzerManager::modeChanged(IMode *mode)
{
    const bool makeVisible = mode->id() == Constants::MODE_ANALYZE;
    if (!makeVisible)
        return;

    d->m_mainWindow->setDockActionsVisible(makeVisible);
}

void AnalyzerManager::selectTool(IAnalyzerTool *tool)
{
    QTC_ASSERT(d->m_tools.contains(tool), return);
    toolSelected(d->m_tools.indexOf(tool));
}

void AnalyzerManager::toolSelected(int idx)
{
    static bool selectingTool = false;
    if (selectingTool)
        return;
    selectingTool = true;

    IAnalyzerTool *oldTool = currentTool();
    if (oldTool) {
        saveToolSettings(oldTool);

        ActionManager *am = ICore::instance()->actionManager();

        foreach(const AnalyzerManagerPrivate::ToolWidgetPair &widget, d->m_toolWidgets.value(oldTool)) {
            QAction *toggleViewAction = widget.second->toggleViewAction();
            am->unregisterAction(toggleViewAction, QString("Analyzer." + widget.second->objectName()));
            d->m_mainWindow->removeDockWidget(widget.second);
            ///NOTE: QMainWindow (and FancyMainWindow) just look at @c findChildren<QDockWidget*>()
            ///if we don't do this, all kind of havoc might happen, including:
            ///- improper saveState/restoreState
            ///- improper list of qdockwidgets in popup menu
            ///- ...
            widget.second->setParent(0);
        }
    }

    d->m_toolGroup->actions().at(idx)->setChecked(true);
    d->m_toolBox->setCurrentIndex(idx);

    IAnalyzerTool *newTool = currentTool();

    if (QWidget *toolbarWidget = d->m_toolToolbarWidgets.value(newTool))
        d->m_toolbarStackedWidget->setCurrentWidget(toolbarWidget);

    foreach(const AnalyzerManagerPrivate::ToolWidgetPair &widget, d->m_toolWidgets.value(newTool)) {
        d->addDock(newTool, widget.first, widget.second);
    }
    loadToolSettings(newTool);

    selectingTool = false;
}

void AnalyzerManager::toolSelected(QAction *action)
{
    toolSelected(d->m_toolGroup->actions().indexOf(action));
}

void AnalyzerManager::addTool(IAnalyzerTool *tool)
{
    d->delayedInit(); // be sure that there is a valid IMode instance

    Internal::AnalyzerPlugin *plugin = Internal::AnalyzerPlugin::instance();
    QAction *action = new QAction(tool->displayName(), d->m_toolGroup);
    action->setData(d->m_tools.count());
    action->setCheckable(true);

    d->m_menu->addAction(action);
    d->m_toolGroup->setVisible(d->m_toolGroup->actions().count() > 1);
    d->m_tools.append(tool);
    d->m_toolBox->addItem(tool->displayName());
    d->m_toolBox->setVisible(d->m_toolBox->count() > 1);
    d->m_toolBoxSeparator->setVisible(d->m_toolBox->isVisible());

    if (currentTool() != tool)
        selectTool(tool); // the first tool gets selected automatically due to signal emission from toolbox
    tool->initialize(plugin);

    QSettings *defaultSettings = new QSettings(this);
    d->m_defaultSettings[tool] = defaultSettings;
    d->m_mainWindow->saveSettings(defaultSettings);

    loadToolSettings(tool);
}

void AnalyzerManager::setToolbar(IAnalyzerTool *tool, QWidget *widget)
{
    d->m_toolToolbarWidgets[tool] = widget;
    d->m_toolbarStackedWidget->addWidget(widget);

    if (currentTool() == tool)
        d->m_toolbarStackedWidget->setCurrentWidget(widget);
}

QDockWidget *AnalyzerManager::createDockWidget(IAnalyzerTool *tool, const QString &title,
                                               QWidget *widget, Qt::DockWidgetArea area)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), qt_noop());

    QDockWidget *dockWidget = d->m_mainWindow->addDockForWidget(widget);
    d->m_dockWidgets << AnalyzerManagerPrivate::DockPtr(dockWidget);
    dockWidget->setWindowTitle(title);

    d->m_toolWidgets[tool] << qMakePair(area, dockWidget);
    dockWidget->installEventFilter(d->m_resizeEventFilter);

    d->addDock(tool, area, dockWidget);

    return dockWidget;
}

IAnalyzerTool *AnalyzerManager::currentTool() const
{
    if (!d->m_toolGroup->checkedAction()) {
        return 0;
    }

    return d->m_tools.value(d->m_toolGroup->checkedAction()->data().toInt());
}

QList<IAnalyzerTool *> AnalyzerManager::tools() const
{
    return d->m_tools;
}

void AnalyzerManager::startTool()
{
    d->startTool();
}

void AnalyzerManager::runControlCreated(AnalyzerRunControl *rc)
{
    QTC_ASSERT(!d->m_currentRunControl, qt_noop());
    d->m_currentRunControl = rc;
    connect(rc, SIGNAL(finished()), this, SLOT(handleToolFinished()));
}

void AnalyzerManager::stopTool()
{
    if (!d->m_currentRunControl)
        return;

    d->m_currentRunControl->stop();
    handleToolFinished();
}

void AnalyzerManager::handleToolFinished()
{
    // this may run as a consequence of calling stopMemcheck() or valgrind may have terminated
    // for a different reason (application exited through user action, segfault, ...), so we
    // duplicate some code from stopMemcheck().
    d->m_startAction->setEnabled(true);
    d->m_stopAction->setEnabled(false);
    d->m_toolBox->setEnabled(true);
    d->m_toolGroup->setEnabled(true);
    d->m_currentRunControl = 0;
}

Utils::FancyMainWindow *AnalyzerManager::mainWindow() const
{
    return d->m_mainWindow;
}

void AnalyzerManager::resetLayout()
{
    d->m_mainWindow->restoreSettings(d->m_defaultSettings.value(currentTool()));
}

void AnalyzerManager::loadToolSettings(IAnalyzerTool *tool)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + tool->id());
    if (settings->value("ToolSettingsSaved", false).toBool()) {
        d->m_mainWindow->restoreSettings(settings);
    }
    settings->endGroup();
}

void AnalyzerManager::saveToolSettings(IAnalyzerTool *tool)
{
    if (!tool)
        return; // no active tool, do nothing

    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + tool->id());
    d->m_mainWindow->saveSettings(settings);
    settings->setValue("ToolSettingsSaved", true);
    settings->endGroup();
}

void AnalyzerManager::updateRunActions()
{
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = pe->startupProject();
    bool startEnabled = !d->m_currentRunControl && pe->canRun(project, Constants::MODE_ANALYZE);
    d->m_startAction->setEnabled(startEnabled);
}

#include "analyzermanager.moc"
