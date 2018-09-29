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

#ifndef BUTTONEDIT_H
#define BUTTONEDIT_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

class ButtonEdit : public QWidget
{
    Q_PROPERTY(quint32 value READ value WRITE setValue)
    Q_PROPERTY(int index READ index)

    Q_OBJECT

public:
    explicit ButtonEdit(QString labelText, QWidget *parent = 0);

    quint32 value() const;
    void setValue(quint32 value);

    int index() const;

public slots:
    void onModeChanged(int idx);

private:
    quint32 extractValue(quint32 mode) const;
    void hideWidgets();

    int buttonIndex;

    QLabel *label;
    QComboBox *cbMode;

    // Key, Sequence
    class QxtCheckComboBox *cbModifiers;
    class EnumEdit *editScans[2];

    // Mouse
    class MouseButtonBox *cbButton;
    QSpinBox *spinCount;
    QSpinBox *spinDelay;

    // Profile
    QComboBox *cbProfile;

    // Macro
    QLabel *labelRepeat;
    QSpinBox *spinMacroIndex;
    QComboBox *cbRepeatMode;

    // Expert mode
    QLineEdit *editCustom;
};

#endif // BUTTONEDIT_H
