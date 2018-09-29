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

#ifndef PROFILEEDIT_H
#define PROFILEEDIT_H

#include "micewidget.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QSlider)

class ProfileEdit : public MiceWidget
{
    Q_PROPERTY(int index READ index)
    Q_PROPERTY(int enabled READ enabled)
    Q_PROPERTY(bool active READ active WRITE setActive)

    Q_OBJECT
public:
    explicit ProfileEdit(int indexValue, QWidget *parent = 0);

    bool load(class MS794 *mice);
    void save(class MS794 *mice);

    bool enabled() const;

    bool active() const;
    void setActive(bool value);

    int index() const;

private slots:
    void onDpiChanged(int value);
    void onEnableProfile(bool enabled);
    void onSelectProfile(bool);

private:
    int indexValue;
    QCheckBox *checkEnabled;
    QLabel *labelDpi;
    QComboBox *cbColor;
    QRadioButton *btnActive;
    QSlider *sliderDpi;
};

#endif // PROFILEEDIT_H
