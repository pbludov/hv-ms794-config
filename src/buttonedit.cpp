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

#include "buttonedit.h"
#include "usbscancodeedit.h"
#include "mousebuttonbox.h"
#include "ms794.h"

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSpinBox>
#include <QxtCheckComboBox>

ButtonEdit::ButtonEdit(QString labelText, QWidget *parent)
    : QWidget(parent)
    , buttonIndex(-1)
{
    auto layout = new QHBoxLayout;
    layout->setMargin(4);

    label = new QLabel(labelText);
    auto measuredWidth = fontMetrics().width("12345678901234");
    label->setMinimumWidth(measuredWidth);
    layout->addWidget(label);
    cbMode = new QComboBox();
    cbMode->setEditable(false);
    cbMode->addItem(tr("Key"), MS794::EventKey);
    cbMode->addItem(tr("Button"), MS794::EventButton);
    cbMode->addItem(tr("Profile"), MS794::EventProfile);
    cbMode->addItem(tr("Macro"), MS794::EventMacro);
    cbMode->addItem(tr("Sequence"), MS794::EventSequence);
    cbMode->addItem(tr("Triple click"), MS794::EventTripleClick);
    cbMode->addItem(tr("Disabled"), MS794::EventDisabled);
    cbMode->addItem(tr("Custom"), MS794::EventCustom);
    layout->addWidget(cbMode);
    label->setBuddy(cbMode);
    connect(cbMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onModeChanged(int)));

    //
    // Keyboard key or combo
    //
    cbModifiers = new QxtCheckComboBox;
    cbModifiers->addItem(tr("LCtrl"), 0x01);
    cbModifiers->addItem(tr("LShift"), 0x02);
    cbModifiers->addItem(tr("LAlt"), 0x04);
    cbModifiers->addItem(tr("LSuper"), 0x08);
    cbModifiers->addItem(tr("RCtrl"), 0x10);
    cbModifiers->addItem(tr("RShift"), 0x20);
    cbModifiers->addItem(tr("RAlt"), 0x40);
    cbModifiers->addItem(tr("RSuper"), 0x80);
    cbModifiers->setDefaultText(tr("None"));
    cbModifiers->setMinimumWidth(measuredWidth * 2);
    layout->addWidget(cbModifiers);

    for (size_t i = 0; i < _countof(editScans); ++i)
    {
        editScans[i] = new UsbScanCodeEdit();
        layout->addWidget(editScans[i]);
    }

    //
    // Mouse or sequence
    //
    cbButton = new MouseButtonBox;
    layout->addWidget(cbButton);
    spinCount = new QSpinBox;
    spinCount->setRange(2, 0xFF);
    spinCount->setPrefix(tr("repeat  "));
    spinCount->setSuffix(tr("  times"));
    layout->addWidget(spinCount);
    spinDelay = new QSpinBox;
    spinDelay->setRange(0, 0xFF);
    spinDelay->setPrefix(tr("delay  "));
    spinDelay->setSuffix(tr("  msec"));
    layout->addWidget(spinDelay);

    //
    // Profile
    //
    cbProfile = new QComboBox;
    cbProfile->addItem(tr("Next"), MS794::NextProfile);
    cbProfile->addItem(tr("Previous"), MS794::PreviousProfile);
    cbProfile->addItem(tr("Cycle"), MS794::CycleProfile);
    cbProfile->addItem(tr("DPI Lock to 500"),  MS794::LockToProfile + 1);
    cbProfile->addItem(tr("DPI Lock to 750"),  MS794::LockToProfile + 2);
    cbProfile->addItem(tr("DPI Lock to 1000"), MS794::LockToProfile + 3);
    cbProfile->addItem(tr("DPI Lock to 1250"), MS794::LockToProfile + 4);
    cbProfile->addItem(tr("DPI Lock to 1500"), MS794::LockToProfile + 5);
    cbProfile->addItem(tr("DPI Lock to 1750"), MS794::LockToProfile + 6);
    cbProfile->addItem(tr("DPI Lock to 2000"), MS794::LockToProfile + 7);
    cbProfile->addItem(tr("DPI Lock to 2500"), MS794::LockToProfile + 8);
    cbProfile->addItem(tr("DPI Lock to 3000"), MS794::LockToProfile + 9);
    cbProfile->addItem(tr("DPI Lock to 3500"), MS794::LockToProfile + 10);
    cbProfile->addItem(tr("DPI Lock to 4000"), MS794::LockToProfile + 11);
    layout->addWidget(cbProfile);

    //
    // Macro
    //
    spinMacroIndex = new QSpinBox;
    spinMacroIndex->setRange(1, MS794::MaxMacroNum);
    layout->addWidget(spinMacroIndex);
    labelRepeat = new QLabel(tr("&repeat"));
    labelRepeat->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    layout->addWidget(labelRepeat);
    cbRepeatMode = new QComboBox;
    cbRepeatMode->addItem(tr("number of times"), MS794::MacroRepeatCount);
    cbRepeatMode->addItem(tr("until next key"), MS794::MacroRepeatUntilNextKey);
    cbRepeatMode->addItem(tr("until released"), MS794::MacroRepeatWhileHold);
    cbRepeatMode->setEditable(false);
    layout->addWidget(cbRepeatMode);
    labelRepeat->setBuddy(cbRepeatMode);

    //
    // Expert mode
    //
    editCustom = new QLineEdit;
    editCustom->setInputMask("HH HH HH HH");
    editCustom->setMaximumWidth(measuredWidth);
    layout->addWidget(editCustom);
    layout->addStretch();
    setLayout(layout);
}

void ButtonEdit::setValue(quint32 value)
{
    buttonIndex = 0x0F & value;
    quint8 mode = 0xF0 & value;
    quint8 arg1 = 0xFF & (value >> 8);
    quint8 arg2 = 0xFF & (value >> 16);
    quint8 arg3 = 0xFF & (value >> 24);
    setUpdatesEnabled(false);
    editCustom->setText(QString("%1 %2 %3 %4")
                            .arg(arg3, 2, 16, QChar('0'))
                            .arg(arg2, 2, 16, QChar('0'))
                            .arg(arg1, 2, 16, QChar('0'))
                            .arg(mode, 2, 16, QChar('0')));

    // Hide everything, will show some ot them later
    hideWidgets();

    switch (mode)
    {
    case MS794::EventButton:
        cbButton->setValue(arg1);
        cbButton->setVisible(true);
        break;

    case MS794::EventSequence:
        editScans[0]->setValue(arg1);
        editScans[0]->setVisible(true);
        spinCount->setValue(arg3);
        spinCount->setVisible(true);
        spinDelay->setValue(arg2);
        spinDelay->setVisible(true);
        break;

    case MS794::EventTripleClick:
        break;

    case MS794::EventKey:
        cbModifiers->setMask(arg1);
        cbModifiers->setVisible(true);
        editScans[0]->setValue(arg2);
        editScans[0]->setVisible(true);
        editScans[1]->setValue(arg3);
        editScans[1]->setVisible(true);
        break;

    case MS794::EventProfile:
        cbProfile->setCurrentIndex(cbProfile->findData(arg1));
        cbProfile->setVisible(true);
        break;

    case MS794::EventMacro:
        spinMacroIndex->setValue(arg1 >> 4);
        spinMacroIndex->setVisible(true);
        labelRepeat->setVisible(true);
        cbRepeatMode->setCurrentIndex(cbRepeatMode->findData(arg1 & 0x0F));
        cbRepeatMode->setVisible(true);
        break;

    case MS794::EventDisabled:
        // Everything is already hidden, so nothing to do.
        break;

    default:
        mode = MS794::EventCustom;
        editCustom->setVisible(true);
        break;
    }

    auto block = cbMode->blockSignals(true);
    cbMode->setCurrentIndex(cbMode->findData(mode));
    cbMode->blockSignals(block);
    setUpdatesEnabled(true);
}

quint32 ButtonEdit::value() const
{
    return extractValue(cbMode->currentData().toUInt());
}

int ButtonEdit::index() const
{
    return buttonIndex;
}

quint32 ButtonEdit::extractValue(quint32 mode) const
{
    quint8 arg1 = 0, arg2 = 0, arg3 = 0;

    switch (mode)
    {
    case MS794::EventKey:
        arg1 = 0xFF & cbModifiers->mask();
        arg2 = 0xFF & editScans[0]->value();
        arg3 = 0xFF & editScans[1]->value();
        break;

    case MS794::EventButton:
        arg1 = 0xFF & cbButton->value();
        break;

    case MS794::EventTripleClick:
        break;

    case MS794::EventProfile:
        arg1 = 0xFF & cbProfile->currentData().toUInt();
        break;

    case MS794::EventMacro:
        arg1 = ((0x0F & spinMacroIndex->value()) << 4)
             | (0xF & cbRepeatMode->currentData().toUInt());
        break;

    case MS794::EventSequence:
        arg1 = 0xFF & editScans[0]->value();
        arg3 = 0xFF & spinCount->value();
        arg2 = 0xFF & spinDelay->value();
        break;

    case MS794::EventDisabled:
        arg1 = 1;
        break;

    case MS794::EventCustom:
        return editCustom->text().replace(" ", "").toUInt(nullptr, 16) | buttonIndex;

    default:
        Q_ASSERT(!"Unhandled mode");
        break;
    }

    return (arg3 << 24) | (arg2 << 16) | (arg1 << 8) | mode | buttonIndex;
}

void ButtonEdit::onModeChanged(int idx)
{
    setUpdatesEnabled(false);
    auto newMode = cbMode->itemData(idx).toUInt();
    auto oldMode = editCustom->text().right(2).toUInt(nullptr, 16) & 0xF0;
    quint32 value = extractValue(oldMode) & ~0xF0;

    hideWidgets();

    if (newMode == MS794::EventCustom)
    {
        editCustom->setText(QString("%1 %2 %3 %4")
                                .arg(0xFF & value >> 24, 2, 16, QChar('0'))
                                .arg(0xFF & value >> 16, 2, 16, QChar('0'))
                                .arg(0xFF & value >> 8, 2, 16, QChar('0'))
                                .arg(oldMode, 2, 16, QChar('0')));
        editCustom->show();
    }
    else
    {
        setValue(value | newMode);
    }

    setUpdatesEnabled(true);
}

void ButtonEdit::hideWidgets()
{
    foreach (auto widget, findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
    {
        if (widget == label || widget == cbMode)
            continue;

        widget->hide();
    }
}
