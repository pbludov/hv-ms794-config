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

#include "ms794.h"
#include "qhiddevice.h"
#include "qhidmonitor.h"

#include <QRgb>

#define VENDOR  0x258A
#define PRODUCT 0x1007
#define KEYBOARD_USAGE_PAGE 7
#define KEYBOARD_USAGE      6

Q_LOGGING_CATEGORY(UsbIo, "usb")

#ifndef qCInfo
// QT 5.2 does not have qCInfo
#define qCInfo qCWarning
#endif

MS794::MS794(QObject *parent)
    : QObject(parent)
    , device(new QHIDDevice(VENDOR, PRODUCT, KEYBOARD_USAGE_PAGE, KEYBOARD_USAGE, this))
    , monitor(new QHIDMonitor(VENDOR, PRODUCT, this))
{
    connect(monitor, SIGNAL(deviceArrival(QString)), this, SLOT(deviceArrival(QString)));
    connect(monitor, SIGNAL(deviceRemove()), this, SLOT(deviceRemove()));
}

MS794::~MS794()
{
    foreach (auto page, cache)
    {
        delete[] page.second;
    }
}

void MS794::deviceArrival(const QString &path)
{
    qCInfo(UsbIo) << "Detected device arrival at" << path;
    auto connected = device->open(VENDOR, PRODUCT, KEYBOARD_USAGE_PAGE, KEYBOARD_USAGE) && ping();
    connectChanged(connected);
}

void MS794::deviceRemove()
{
    qCInfo(UsbIo) << "Detected device removal";
    connectChanged(false);
}

int MS794::getPageSize(Page page)
{
    switch (page)
    {
    case Page::PageLighting:
        return 59;
    case Page::PageButtons:
        return 1145;
    case Page::PageProfile:
        return 9;
    default:
        return 0;
    }
}

char *MS794::readPage(Page page)
{
    auto iter = cache.find(page);

    if (iter != cache.end())
        return iter->second;

    int pageSize = getPageSize(page);
    auto value = new char[pageSize];
    Q_CHECK_PTR(value);

    value[0] = (char)page;
    auto read = device->getFeatureReport(value, pageSize);

    if (read != pageSize || value[0] != (char)page)
    {
        qCWarning(UsbIo) << "readPage: invalid response:" << read;
        return nullptr;
    }
    qCDebug(UsbIo) << "readPage" << page << QByteArray(value, pageSize).toHex();

    dirtyPages[page] = false;
    return cache[page] = value;
}

bool MS794::writePage(const char *data, Page cmd)
{
    auto pageSize = getPageSize(cmd);
    int sent = device->sendFeatureReport(data, pageSize);
    if (sent != pageSize)
    {
        qCWarning(UsbIo) << "writePage: send failed: got" << sent << "expected" << pageSize;
        return false;
    }

    return true;
}

int MS794::button(ButtonIndex btn)
{
    auto bytes = readPage(PageButtons);

    if (!bytes)
        return -1;

    bytes += ButtonsOffset;
    auto btns = (const int *)bytes;
    return btns[btn];
}

void MS794::setButton(ButtonIndex btn, int value)
{
    auto bytes = readPage(PageButtons);

    if (bytes)
    {
        auto btns = (int *)(bytes + ButtonsOffset);

        if (btns[btn] != value)
        {
            dirtyPages[PageButtons] = true;
            btns[btn] = value;
        }
    }
}

QByteArray MS794::macro(int index)
{
    // Make it zero-based
    --index;

    auto offset = MacrosOffset + index * MaxMacroLength;
    auto page = readPage(PageButtons);
    return page ? QByteArray(page + offset, MaxMacroLength) : nullptr;
}

void MS794::setMacro(int index, const QByteArray &value)
{
    // Make it zero-based
    --index;

    auto offset = MacrosOffset + index * MaxMacroLength;
    auto page = readPage(PageButtons);
    auto length = qMin((int)MaxMacroLength, value.length());

    if (page && memcmp(page + offset, value.cbegin(), size_t(length)))
    {
        page += MacrosOffset + index * MaxMacroLength;
        memcpy(page, value.cbegin(), size_t(length));
        memset(page + length, 0, MaxMacroLength - size_t(length));
        dirtyPages[PageButtons] = true;
    }
}

int MS794::readByte(Page page, int offset)
{
    auto bytes = readPage(page);
    return bytes ? 0xFF & bytes[offset] : -1;
}

void MS794::writeByte(Page page, int offset, int value)
{
    auto bytes = readPage(page);
    if (bytes)
    {
        if ((0xFF & bytes[offset]) != (0xFF & value))
        {
            dirtyPages[page] = true;
            bytes[offset] = (char)value;
        }
    }
}

int MS794::lightColor(int index)
{
    Q_ASSERT(index >= 0 && index < MaxLightColor);

    auto bytes = readPage(PageLighting);
    if (!bytes)
        return -1;

    bytes += LightColorOffset + index * 3;

    return qRgba(bytes[0], bytes[1], bytes[2], 0);
}

void MS794::setLightColor(int index, int value)
{
    Q_ASSERT(index >= 0 && index < MaxLightColor);
    Q_ASSERT(value >= 0 && value <= 0xFFFFFF);

    auto bytes = readPage(PageLighting);
    if (bytes)
    {
        bytes += LightColorOffset + index * 3;

        if ((0xFF & bytes[0]) != qRed(value) || (0xFF & bytes[1]) != qGreen(value) || (0xFF & bytes[2]) != qBlue(value))
        {
            dirtyPages[PageLighting] = true;
            bytes[0] = (char)qRed(value);
            bytes[1] = (char)qGreen(value);
            bytes[2] = (char)qBlue(value);
        }
    }
}

int MS794::reportRate()
{
    return readByte(PageProfile, ReportRateOffset);
}

void MS794::setReportRate(int value)
{
    Q_ASSERT(value > 0 && value <= MaxReportRate);

    writeByte(PageProfile, ReportRateOffset, value);
}

int MS794::profile()
{
    return readByte(PageProfile, ActiveProfileOffset);
}

void MS794::setProfile(int value)
{
    Q_ASSERT(value > 0 && value <= MaxProfile);

    writeByte(PageProfile, ActiveProfileOffset, value);
}

int MS794::numProfiles()
{
    return readByte(PageLighting, NumProfilesOffset);
}

void MS794::setNumProfiles(int value)
{
    Q_ASSERT(value >= 0 && value <= MaxProfile);

    writeByte(PageLighting, NumProfilesOffset, value);
}

int MS794::lightType()
{
    return readByte(PageLighting, LightTypeOffset);
}

void MS794::setLightType(int value)
{
    Q_ASSERT(value >= 0 && (value >> 4) <= MaxLightType);

    writeByte(PageLighting, LightTypeOffset, value);
}

int MS794::lightValue()
{
    return readByte(PageLighting, LightValueOffset);
}

void MS794::setLightValue(int value)
{
    writeByte(PageLighting, LightValueOffset, value);
}

bool MS794::profileEnabled(int profile)
{
    Q_ASSERT(profile >= 0 && profile < MaxProfile);

    int byte = readByte(PageLighting, DpiOffset + profile);
    return 0 == (byte & 0x80);
}

void MS794::setProfileEnabled(int profile, bool value)
{
    Q_ASSERT(profile >= 0 && profile < MaxProfile);

    int byte = readByte(PageLighting, DpiOffset + profile);
    if (value)
        byte &= 0x7F;
    else
        byte |= 0x80;

    writeByte(PageLighting, DpiOffset + profile, byte);
}

int MS794::profileDpi(int profile)
{
    Q_ASSERT(profile >= 0 && profile < MaxProfile);

    return readByte(PageLighting, DpiOffset + profile) & 0x0F;
}

void MS794::setProfileDpi(int profile, int value)
{
    Q_ASSERT(profile >= 0 && profile < MaxProfile);

    int byte = readByte(PageLighting, DpiOffset + profile);
    if (byte & 0x80)
    {
        value |= 0x80;
    }
    writeByte(PageLighting, DpiOffset + profile, value);
}

int MS794::profileColorIndex(int profile)
{
    Q_ASSERT(profile >= 0 && profile < MaxProfile);
    return readByte(PageLighting, ProfileColorOffset + profile);
}

void MS794::setProfileColorIndex(int profile, int value)
{
    Q_ASSERT(profile >= 0 && profile < MaxProfile);
    Q_ASSERT(value >= 0 && value <= MaxProfileColor);

    writeByte(PageLighting, ProfileColorOffset + profile, value);
}

void MS794::blink(bool value)
{
    writeByte(PageProfile, BlinkOffset, value ? 2 : 1);
}

bool MS794::ping()
{
    return readPage(PageProfile) != nullptr;
}

bool MS794::unsavedChanges()
{
    return dirtyPages.cend() != std::find_if(dirtyPages.cbegin(), dirtyPages.cend(),
                                    [](const std::map<int, bool>::value_type &x) { return x.second; });
}

bool MS794::save()
{
    foreach (auto page, cache)
    {
        if (!dirtyPages[page.first])
            continue;

        if (writePage(page.second, page.first))
            dirtyPages[page.first] = false;
        else
            return false;
    }

    return true;
}

bool MS794::backupConfig(QIODevice *storage)
{
    auto cmds = {PageLighting, PageProfile, PageButtons};
    foreach (const auto& cmd, cmds)
    {
        auto page = readPage(cmd);
        if (!page)
            return false;
        storage->write(page, getPageSize(cmd));
    }

    return true;
}

bool MS794::restoreConfig(QIODevice *storage)
{
    int expected = 0;
    auto cmds = {PageLighting, PageProfile, PageButtons};
    foreach (const auto& cmd, cmds)
    {
        expected += getPageSize(cmd);
    }

    if (storage->size() != expected)
    {
        return false;
    }

    foreach (const auto& cmd, cmds)
    {
        auto page = storage->read(getPageSize(cmd));
        if (page.at(0) != cmd || !writePage(page.cbegin(), cmd))
            return false;
    }

    return true;
}
