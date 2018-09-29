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

#include "pagerate.h"
#include "ui_pagesensitivity.h"
#include "ms794.h"

PageRate::PageRate(QWidget *parent)
    : MiceWidget(parent)
    , ui(new Ui::PageSensitivity)
{
    ui->setupUi(this);

    ui->labelReportRate->setMinimumWidth(fontMetrics().width(tr("Refresh rate")));
}

PageRate::~PageRate()
{
    delete ui;
}

bool PageRate::load(MS794 *mice)
{
    auto valueRate = mice->reportRate();
    if (valueRate < 0)
        return false;

    ui->sliderReportRate->setValue(valueRate);
    onReportRateChanged(valueRate);
    return true;
}

void PageRate::save(MS794 *mice)
{
    mice->setReportRate(ui->sliderReportRate->value());
}

void PageRate::onReportRateChanged(int value)
{
    // Make it zero-based
    --value;

    ui->labelReportRate->setText(tr("%1Hz").arg(125 * (1 << value)));
}
