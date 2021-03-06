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

#include "mobileappwizardpages.h"
#include "ui_mobileappwizardgenericoptionspage.h"
#include "ui_mobileappwizardmaemooptionspage.h"
#include "ui_mobileappwizardsymbianoptionspage.h"
#include <coreplugin/coreconstants.h>

#include <QtCore/QTemporaryFile>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardGenericOptionsPagePrivate
{
    Ui::MobileAppWizardGenericOptionsPage ui;
    friend class MobileAppWizardGenericOptionsPage;
};

class MobileAppWizardSymbianOptionsPagePrivate
{
    Ui::MobileAppWizardSymbianOptionsPage ui;
    QString svgIcon;
    friend class MobileAppWizardSymbianOptionsPage;
};

class MobileAppWizardMaemoOptionsPagePrivate
{
    Ui::MobileAppWizardMaemoOptionsPage ui;
    QString pngIcon;
    friend class MobileAppWizardMaemoOptionsPage;
};


MobileAppWizardGenericOptionsPage::MobileAppWizardGenericOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardGenericOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Automatically Rotate Orientation"),
        AbstractMobileApp::ScreenOrientationAuto);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Landscape Orientation"),
        AbstractMobileApp::ScreenOrientationLockLandscape);
    m_d->ui.orientationBehaviorComboBox->addItem(tr("Lock to Portrait Orientation"),
        AbstractMobileApp::ScreenOrientationLockPortrait);
}

MobileAppWizardGenericOptionsPage::~MobileAppWizardGenericOptionsPage()
{
    delete m_d;
}

void MobileAppWizardGenericOptionsPage::setOrientation(AbstractMobileApp::ScreenOrientation orientation)
{
    QComboBox *const comboBox = m_d->ui.orientationBehaviorComboBox;
    for (int i = 0; i < comboBox->count(); ++i)
        if (comboBox->itemData(i).toInt() == static_cast<int>(orientation)) {
            comboBox->setCurrentIndex(i);
            break;
        }
}

AbstractMobileApp::ScreenOrientation MobileAppWizardGenericOptionsPage::orientation() const
{
    const int index = m_d->ui.orientationBehaviorComboBox->currentIndex();
    return static_cast<AbstractMobileApp::ScreenOrientation>(m_d->ui.orientationBehaviorComboBox->itemData(index).toInt());
}


MobileAppWizardSymbianOptionsPage::MobileAppWizardSymbianOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardSymbianOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    const QIcon open = QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
    m_d->ui.appIconLoadToolButton->setIcon(open);
    connect(m_d->ui.appIconLoadToolButton, SIGNAL(clicked()), SLOT(openSvgIcon()));
}

MobileAppWizardSymbianOptionsPage::~MobileAppWizardSymbianOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardSymbianOptionsPage::svgIcon() const
{
    return m_d->svgIcon;
}

void MobileAppWizardSymbianOptionsPage::setSvgIcon(const QString &icon)
{
    QPixmap iconPixmap(icon);
    if (!iconPixmap.isNull()) {
        const int symbianIconSize = 44;
        if (iconPixmap.height() > symbianIconSize || iconPixmap.width() > symbianIconSize)
            iconPixmap = iconPixmap.scaledToHeight(symbianIconSize, Qt::SmoothTransformation);
        m_d->ui.appIconPreview->setPixmap(iconPixmap);
        m_d->svgIcon = icon;
    }
}

QString MobileAppWizardSymbianOptionsPage::symbianUid() const
{
    return m_d->ui.uid3LineEdit->text();
}

void MobileAppWizardSymbianOptionsPage::setSymbianUid(const QString &uid)
{
    m_d->ui.uid3LineEdit->setText(uid);
}

void MobileAppWizardSymbianOptionsPage::setNetworkEnabled(bool enableIt)
{
    m_d->ui.enableNetworkCheckBox->setChecked(enableIt);
}

bool MobileAppWizardSymbianOptionsPage::networkEnabled() const
{
    return m_d->ui.enableNetworkCheckBox->isChecked();
}

void MobileAppWizardSymbianOptionsPage::openSvgIcon()
{
    const QString svgIcon = QFileDialog::getOpenFileName(
            this,
            m_d->ui.appIconLabel->text(),
            QDesktopServices::storageLocation(QDesktopServices::PicturesLocation),
            QLatin1String("*.svg"));
    if (!svgIcon.isEmpty())
        setSvgIcon(svgIcon);
}


MobileAppWizardMaemoOptionsPage::MobileAppWizardMaemoOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , m_d(new MobileAppWizardMaemoOptionsPagePrivate)
{
    m_d->ui.setupUi(this);
    connect(m_d->ui.pngIconButton, SIGNAL(clicked()), this, SLOT(openPngIcon()));
}

MobileAppWizardMaemoOptionsPage::~MobileAppWizardMaemoOptionsPage()
{
    delete m_d;
}

QString MobileAppWizardMaemoOptionsPage::pngIcon() const
{
    return m_d->pngIcon;
}

void MobileAppWizardMaemoOptionsPage::setPngIcon(const QString &icon)
{
    QString error;
    QPixmap iconPixmap(icon);
    if (iconPixmap.isNull()) {
        QMessageBox::critical(this, tr("Invalid Icon"),
            tr("The file is not a valid image."));
        return;
    }

    const QSize iconSize(64, 64);
    QString actualIconPath;
    if (iconPixmap.size() == iconSize) {
        actualIconPath = icon;
    } else {
        const QMessageBox::StandardButton button = QMessageBox::warning(this,
            tr("Wrong Icon Size"), tr("The icon needs to be 64x64 pixels big, "
                "but is not. Do you want Creator to scale it?"),
            QMessageBox::Ok | QMessageBox::Cancel);
        if (button != QMessageBox::Ok)
            return;
        iconPixmap = iconPixmap.scaled(iconSize);
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(false);
        const char * const format = QFileInfo(icon).suffix().toAscii().data();
        if (!tmpFile.open() || !iconPixmap.save(&tmpFile, format)) {
            QMessageBox::critical(this, tr("File Error"),
                tr("Could not copy icon file."));
            return;
        }
        actualIconPath = tmpFile.fileName();
    }

    m_d->ui.pngIconButton->setIcon(iconPixmap);
    m_d->pngIcon = actualIconPath;
}

void MobileAppWizardMaemoOptionsPage::openPngIcon()
{
    const QString iconPath = QFileDialog::getOpenFileName(this,
        m_d->ui.appIconLabel->text(), m_d->pngIcon,
        QLatin1String("*.png"));
    if (!iconPath.isEmpty())
        setPngIcon(iconPath);
}

} // namespace Internal
} // namespace Qt4ProjectManager
