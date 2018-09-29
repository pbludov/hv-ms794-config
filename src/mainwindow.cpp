/*
 *      Copyright 2018 Pavel Bludov <pbludov@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ms794.h"

#include "buttonedit.h"
#include "profileedit.h"
#include "pagelight.h"
#include "pagemacro.h"
#include "pagerate.h"

#include <qxtglobal.h>
#include <QCloseEvent>
#include <QMessageBox>

static void initAction(QAction *action, QStyle::StandardPixmap icon, QKeySequence::StandardKey key)
{
    action->setIcon(qApp->style()->standardIcon(icon));
    action->setShortcut(key);
    action->setToolTip(action->shortcut().toString(QKeySequence::NativeText));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mice(new MS794(this))
{
    ui->setupUi(this);
    // The Designer really lacs this functionality
    initAction(ui->actionExit, QStyle::SP_DialogCloseButton, QKeySequence::Quit);
    initAction(ui->actionSave, QStyle::SP_DialogSaveButton, QKeySequence::Save);

    ui->labelText->setText(ui->labelText->text().arg(PRODUCT_VERSION).arg(__DATE__));
    connect(mice, SIGNAL(connectChanged(bool)), this, SLOT(onMiceConnected(bool)));

    // Check the device availability
    onMiceConnected(mice->ping());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateMice()
{
    foreach (auto edit, findChildren<ButtonEdit *>())
    {
        mice->setButton((MS794::ButtonIndex)(edit->index() - 1), edit->value());
    }

    int numProfiles = 0;
    foreach (auto widget, findChildren<MiceWidget *>())
    {
        widget->save(mice);

        ProfileEdit * pe = qobject_cast<ProfileEdit *>(widget);
        if (pe && pe->enabled())
        {
            ++numProfiles;
            if (pe->active())
            {
                mice->setProfile(numProfiles);
            }
        }
    }

    if (numProfiles > 0)
    {
        mice->setNumProfiles(numProfiles);
    }
}

void MainWindow::onSave()
{
    updateMice();

    if (!mice->save())
    {
        QMessageBox::warning(this, windowTitle(), tr("Failed to save"));
    }
}

void MainWindow::onMiceConnected(bool connected)
{
    auto aboutIndex = ui->tabWidget->indexOf(ui->pageAbout);
    if (!connected)
        ui->tabWidget->setCurrentIndex(aboutIndex);

    for (int i = 0; i < ui->tabWidget->count(); ++i)
    {
        if (aboutIndex == i)
            continue;
        ui->tabWidget->setTabEnabled(i, connected);
    }

    ui->actionSave->setEnabled(connected);
}

std::pair<QString, MS794::ButtonIndex> mainButtons[] =
{
    {QCoreApplication::translate("button", "Button &Left"),    MS794::ButtonLeft},
    {QCoreApplication::translate("button", "Button &Right"),   MS794::ButtonRight},
    {QCoreApplication::translate("button", "Wheel &Click"),    MS794::WheelClick},
    {QCoreApplication::translate("button", "Button B&ack"),    MS794::ButtonBack},
    {QCoreApplication::translate("button", "Button &Forward"), MS794::ButtonForward},
    {QCoreApplication::translate("button", "Button &Plus"),    MS794::ButtonPlus},
    {QCoreApplication::translate("button", "Button &Minus"),   MS794::ButtonMinus},
};

static bool prepareButtonsPage(
    QWidget *parent, MS794 *mice, const std::pair<QString, MS794::ButtonIndex> *buttons, size_t numButtons)
{
    auto layout = new QVBoxLayout;

    for (size_t i = 0; i < numButtons; ++i)
    {
        auto value = mice->button(buttons[i].second);
        if (value == -1)
        {
            delete layout;
            return false;
        }
        auto edit = new ButtonEdit(buttons[i].first);
        edit->setValue(value);
        layout->addWidget(edit);
    }

    layout->addStretch();
    parent->setLayout(layout);
    return true;
}

static bool prepareProfilesPage(QWidget *parent, MS794 *mice)
{
    int activeProfile = mice->profile();
    if (activeProfile < 0)
        return false;

    auto layout = new QVBoxLayout;
    layout->setMargin(0);

    int enabledProfiles = 0;
    for (int i = 0; i < MS794::MaxProfile; ++i)
    {
        auto edit = new ProfileEdit(i);
        if (!edit->load(mice))
        {
            delete edit;
            delete layout;
            return false;
        }

        if (edit->enabled())
        {
            ++enabledProfiles;
        }
        edit->setActive(enabledProfiles == activeProfile);
        layout->addWidget(edit);
    }

    layout->addStretch();
    parent->setLayout(layout);
    return true;
}

bool MainWindow::initPage(QWidget *parent, MiceWidget *page)
{
    if (!page->load(mice))
    {
        delete page;
        return false;
    }

    auto layout = new QVBoxLayout;
    parent->setLayout(layout);
    layout->addWidget(page);
    return true;
}

void MainWindow::onPreparePage(int idx)
{
    auto page = ui->tabWidget->widget(idx);

    if (page->layout())
    {
        // Already prepared
        return;
    }

    bool ok = true;
    if (page == ui->pageMainButtons)
        ok = prepareButtonsPage(page, mice, mainButtons, _countof(mainButtons));
    else if (page == ui->pageMacros)
        ok = initPage(page, new PageMacro());
    else if (page == ui->pageRate)
        ok = initPage(page, new PageRate());
    else if (page == ui->pageProfiles)
        ok = prepareProfilesPage(page, mice);
    else if (page == ui->pageLight)
        ok = initPage(page, new PageLight());

    if (!ok)
    {
        QMessageBox::warning(this, windowTitle(), tr("Failed to read mouse NAND"));
    }
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
    updateMice();

    if (mice->unsavedChanges()
        && QMessageBox::question(this, windowTitle(), tr("You have unsaved changes.\nSave them now?"))
               == QMessageBox::Yes)
    {
        if (!mice->save())
        {
            QMessageBox::warning(this, windowTitle(), tr("Failed to save"));
            evt->ignore();
            return;
        }
    }

    QMainWindow::closeEvent(evt);
}
