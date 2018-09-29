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

#include "profileedit.h"
#include "colorbutton.h"
#include "ms794.h"

#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QxtCheckComboBox>

static Qt::GlobalColor PROFILE_COLORS[] = {
    Qt::GlobalColor::black,
    Qt::GlobalColor::red,
    Qt::GlobalColor::green,
    Qt::GlobalColor::blue,
    Qt::GlobalColor::cyan,
    Qt::GlobalColor::yellow,
    Qt::GlobalColor::magenta,
    Qt::GlobalColor::white,
};

static int DPI[] = {
    0,
    500,
    750,
    1000,
    1250,
    1500,
    1750,
    2000,
    2500,
    3000,
    3500,
    4000,
};

ProfileEdit::ProfileEdit(int index, QWidget *parent)
    : MiceWidget(parent)
    , indexValue(index)
{
    const int spacing = 24;
    auto measuredWidth = fontMetrics().width(tr("DPI    000000"));
    auto layout = new QHBoxLayout;

    btnActive = new QRadioButton(tr("Profile &%1").arg(index + 1));
    layout->addWidget(btnActive);
    layout->addSpacing(spacing);
    connect(btnActive, SIGNAL(clicked(bool)), this, SLOT(onSelectProfile(bool)));

    checkEnabled = new QCheckBox(tr("&Enabled"));
    layout->addWidget(checkEnabled);
    layout->addSpacing(spacing);
    connect(checkEnabled, SIGNAL(toggled(bool)), this, SLOT(onEnableProfile(bool)));

    labelDpi = new QLabel();
    labelDpi->setMinimumWidth(measuredWidth);
    sliderDpi = new QSlider(Qt::Horizontal);
    sliderDpi->setRange(1, MS794::MaxDpi);
    labelDpi->setBuddy(sliderDpi);
    layout->addWidget(labelDpi);
    connect(sliderDpi, SIGNAL(valueChanged(int)), this, SLOT(onDpiChanged(int)));

    layout->addWidget(sliderDpi);
    layout->addSpacing(spacing);

    auto labelColor = new QLabel(tr("&Color"));
    cbColor = new QComboBox;
    cbColor->setEditable(false);
    for (const auto& color : PROFILE_COLORS)
    {
        QPixmap px(128, 128);
        px.fill(QColor(color));
        cbColor->addItem(px, "");
    }
    labelColor->setBuddy(cbColor);
    layout->addWidget(labelColor);
    layout->addWidget(cbColor);

    layout->addStretch();
    setLayout(layout);
}

bool ProfileEdit::load(class MS794 *mice)
{
    auto dpi = mice->profileDpi(indexValue);
    auto enabled = mice->profileEnabled(indexValue);
    auto color = mice->profileColorIndex(indexValue);
    if (dpi < 0 || color < 0)
        return false;

    onEnableProfile(enabled);
    checkEnabled->setChecked(enabled);
    sliderDpi->setValue(dpi);
    onDpiChanged(dpi);
    cbColor->setCurrentIndex(color);
    return true;
}

void ProfileEdit::save(class MS794 *mice)
{
    mice->setProfileDpi(indexValue, sliderDpi->value());
    mice->setProfileEnabled(indexValue, checkEnabled->isChecked());
    mice->setProfileColorIndex(indexValue, cbColor->currentIndex());
}

bool ProfileEdit::enabled() const
{
    return checkEnabled->isChecked();
}

bool ProfileEdit::active() const
{
    return btnActive->isChecked();
}

void ProfileEdit::setActive(bool value)
{
    btnActive->setChecked(value);
}

int ProfileEdit::index() const
{
    return indexValue;
}

void ProfileEdit::onDpiChanged(int value)
{
    labelDpi->setText(tr("&DPI %1").arg(DPI[value], 6));
}

void ProfileEdit::onEnableProfile(bool enabled)
{
    if (!enabled)
        btnActive->setChecked(false);

    foreach (auto widget, findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
    {
        if (checkEnabled == widget)
            continue;

        widget->setEnabled(enabled);
    }
}

void ProfileEdit::onSelectProfile(bool)
{
    foreach (auto btn, parentWidget()->findChildren<QRadioButton *>())
    {
        btn->setChecked(btn == btnActive);
    }
}
