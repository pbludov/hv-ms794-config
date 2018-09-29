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

#include "pagelight.h"
#include "ui_pagelight.h"
#include "colorbutton.h"
#include "ms794.h"

#include <QCheckBox>

PageLight::PageLight(QWidget *parent)
    : MiceWidget(parent)
    , ui(new Ui::PageLight)
{
    ui->setupUi(this);

    ui->cbDirection->addItems(QStringList() << tr("Up") << tr("Down"));

    for (int i = 0; i < MS794::MaxLightColor; ++i)
    {
        auto btn = new ColorButton;
        auto chk = new QCheckBox(tr("Color &%1").arg(i + 1));
        ui->layout->addRow(chk, btn);
        colorLabels.push_back(chk);
        buttons.push_back(btn);
    }

    ui->cbType->addItems(QStringList()
                         << tr("Off")
                         << tr("Colorful Streaming")
                         << tr("Steady")
                         << tr("Breathing")
                         << tr("Tail")
                         << tr("Neon")
                         << tr("Colorful Steady")
                         << tr("Fliker")
                         << tr("Response")
                         << tr("Streaming")
                         << tr("Wave")
                         << tr("Trailing")
                         );
}

PageLight::~PageLight()
{
    delete ui;
}

bool PageLight::load(MS794 *mice)
{
    auto type = mice->lightType();
    auto value = mice->lightValue();
    if (type < 0 || value < 0)
        return false;

    auto subType = type & 0x0f;
    type >>= 4;

    ui->cbType->setCurrentIndex(type);

    int enabledColors = 0;
    switch (type)
    {
    case MS794::LightSteady:
        enabledColors = 1;
        break;
    case MS794::LightBreathing:
    case MS794::LightColorfulSteady:
    case MS794::LightResponse:
        enabledColors = value;
        break;
    case MS794::LightFliker:
        enabledColors = 2;
        break;
    }

    ui->cbDirection->setCurrentIndex(value ? 0 : 1);
    ui->sliderValue->setValue(subType);
    ui->checkRandomColor->setChecked(value & 0x80);

    for (int i = 0; i < MS794::MaxLightColor; ++i)
    {
        buttons[i]->setValue(mice->lightColor(i));
        colorLabels[i]->setChecked(i < enabledColors);
    }

    onLightTypeChanged(type);
    return true;
}

void PageLight::save(MS794 *mice)
{
    auto type = ui->cbType->currentIndex();
    auto speed = ui->sliderValue->value();
    auto value = 0;

    if (type == MS794::LightColorfulStreaming || type == MS794::LightStreaming)
    {
        value = ui->cbDirection->currentIndex() > 0 ? 0 : 0x80;
    }
    else if (type == MS794::LightResponse && ui->checkRandomColor->isChecked())
    {
        value = 0x80;
    }
    else
    {
        for (int i = 0; i < MS794::MaxLightColor; ++i)
        {
            if (colorLabels[i]->isChecked())
            {
                mice->setLightColor(value, buttons[i]->value());
                ++value;
            }
        }
    }

    mice->setLightType(type << 4 | speed);
    mice->setLightValue(value);
}

void PageLight::onRandomColorToggled(bool value)
{
    for (int i = 0; i < MS794::MaxLightColor; ++i)
    {
        buttons[i]->setHidden(value);
        colorLabels[i]->setHidden(value);
    }
}

void PageLight::onLightTypeChanged(int value)
{
    hideWidgets();
    int numColors = value == MS794::LightSteady ? 1
        : value == MS794::LightFliker ? 2
        : value == MS794::LightBreathing
            || value == MS794::LightColorfulSteady
            || value == MS794::LightResponse ? MS794::MaxLightColor
        : 0;

    if (value == MS794::LightResponse)
    {
        ui->labelRandomColor->show();
        ui->checkRandomColor->show();
        if (ui->checkRandomColor->isChecked())
        {
            numColors = 0;
        }
    }

    for (int i = 0; i < numColors; ++i)
    {
        buttons[i]->show();
        colorLabels[i]->show();
    }

    if (value == MS794::LightColorfulStreaming || value == MS794::LightStreaming)
    {
        ui->labelDirection->show();
        ui->cbDirection->show();
    }

    if (value != MS794::LightOff && value != MS794::LightFliker
        && value != MS794::LightColorfulSteady)
    {
        ui->labelValue->setText(value == MS794::LightSteady ? tr("Brightn&ess") : tr("&Speed"));
        ui->labelValue->show();
        ui->sliderValue->setMaximum(value == MS794::LightSteady ? 10 : 3);
        ui->sliderValue->show();
    }
}

void PageLight::hideWidgets()
{
    foreach (auto widget, findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
    {
        if (widget == ui->labelType || widget == ui->cbType)
            continue;

        widget->hide();
    }
}
