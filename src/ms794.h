/*
 *      Copyright 2017-2018 Pavel Bludov <pbludov@gmail.com>
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

#ifndef MS794_H
#define MS794_H

#include <QObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(UsbIo)

class MS794 : public QObject
{
    Q_PROPERTY(int lightType READ lightType WRITE setLightType)
    Q_PROPERTY(int lightValue READ lightValue WRITE setLightValue)
    Q_PROPERTY(int numProfiles READ numProfiles WRITE setNumProfiles)
    Q_PROPERTY(int profile READ profile WRITE setProfile)
    Q_PROPERTY(int reportRate READ reportRate WRITE setReportRate)
    Q_PROPERTY(bool unsavedChanges READ unsavedChanges)

    Q_OBJECT

    enum Page
    {
        PageLighting = 4, // 59 bytes
        PageButtons = 6, // 1145 bytes
        PageProfile = 8, // 9 bytes
    };

public:
    enum Constants
    {
        MaxMacroNum = 8,
        MaxMacroLength = 128,
        MaxReportRate = 4,
        MaxLightColor = 7,
        MaxLightType = 11,
        MaxProfile = 8,
        MaxProfileColor = 8,
        MaxDpi = 11,
    };

    enum Event
    {
        EventButton = 0x10,
        EventSequence = 0x20,
        EventTripleClick = 0x30,
        EventProfile = 0x40,
        EventDisabled = 0x50,
        EventKey = 0x60,
        EventMacro = 0x90,
        EventCustom = 0xFE,
    };

    enum ButtonIndex
    {
        ButtonLeft,
        ButtonRight,
        WheelClick,
        ButtonBack,
        ButtonForward,
        ButtonPlus,
        ButtonMinus,
    };

    enum MouseButton
    {
        MouseLeftButton = 0xF0,
        MouseRightButton,
        MouseMiddleButton,
        MouseBackButton,
        MouseForwardButton,
    };

    enum LightType
    {
        LightOff,
        LightColorfulStreaming,
        LightSteady,
        LightBreathing,
        LightTail,
        LightNeon,
        LightColorfulSteady,
        LightFliker,
        LightResponse,
        LightStreaming,
        LightWave,
        LightTrailing,
    };

    enum ProfileChange
    {
        NextProfile = 0x20,
        PreviousProfile = 0x40,
        // Same as next, but wraps at maximum
        CycleProfile = 0x00,
        LockToProfile = 0x80,
    };

    enum MacroRepeatMode
    {
        MacroRepeatCount = 1,
        MacroRepeatUntilNextKey = 2,
        MacroRepeatWhileHold = 4,
    };

    explicit MS794(QObject *parent = 0);
    ~MS794();

    int reportRate();
    void setReportRate(int value);

    int profile();
    void setProfile(int value);

    int numProfiles();
    void setNumProfiles(int value);

    int lightType();
    void setLightType(int value);

    int lightValue();
    void setLightValue(int value);

    bool unsavedChanges();
    bool save();

    int lightColor(int index);
    void setLightColor(int index, int value);

    int button(ButtonIndex btn);
    void setButton(ButtonIndex btn, int value);

    bool profileEnabled(int profile);
    void setProfileEnabled(int profile, bool value);

    int profileDpi(int profile);
    void setProfileDpi(int profile, int value);

    int profileColorIndex(int profile);
    void setProfileColorIndex(int profile, int value);

    QByteArray macro(int index);
    void setMacro(int index, const QByteArray &value);

    void blink(bool value);
    bool ping();
    bool backupConfig(class QIODevice *storage);
    bool restoreConfig(class QIODevice *storage);

signals:
    void connectChanged(bool connected);

private slots:
    void deviceArrival(const QString &path);
    void deviceRemove();

private:
    enum ButtonsPage
    {
        MacrosOffset = 1,
        ButtonsOffset = MacrosOffset + Constants::MaxMacroNum * Constants::MaxMacroLength,
    };

    enum ProfilePage
    {
        BlinkOffset = 1,
        ReportRateOffset = 2,
        ActiveProfileOffset = 5,
    };

    enum LightingPage
    {
        NumProfilesOffset = 2,
        DpiOffset = 5,
        LightTypeOffset = 21,
        LightValueOffset,
        LightColorOffset,
        ProfileColorOffset = 44,
    };

    char *readPage(Page page);
    bool writePage(const char *data, Page cmd);

    int readByte(Page page, int offset);
    void writeByte(Page page, int offset, int value);

    static int getPageSize(Page page);

    class QHIDDevice *device;
    class QHIDMonitor *monitor;

    std::map<Page, char *> cache;
    std::map<Page, bool> dirtyPages;
};

#endif // MS794_H
