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
#include "ms794.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QThread>

inline QString tr(const char *str)
{
    return QCoreApplication::translate("main", str);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(PRODUCT_NAME);
    QCoreApplication::setApplicationVersion(PRODUCT_VERSION);
    app.setWindowIcon(QIcon(":/app/icon"));

    QCommandLineParser parser;
    parser.setApplicationDescription(tr("HV-MS794 configuration application"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption profileOption(QStringList() << "p" << "profile", tr("Get the active profile."));
    parser.addOption(profileOption);
    QCommandLineOption setProfileOption(QStringList() << "P" << "set-profile", tr("Select the active <profile>."), tr("profile"));
    parser.addOption(setProfileOption);
    QCommandLineOption reportRateOption(QStringList() << "r" << "rate", tr("Get the active report rate."));
    parser.addOption(reportRateOption);
    QCommandLineOption setReportRateOption(QStringList() << "R" << "set-rate", tr("Select the active report <rate>."), tr("rate"));
    parser.addOption(setReportRateOption);
    QCommandLineOption backupOption(QStringList() << "backup", tr("Backup NAND data to a <file>."), tr("file"));
    parser.addOption(backupOption);
    QCommandLineOption restoreOption(QStringList() << "restore", tr("Restore NAND data from a <file>."), tr("file"));
    parser.addOption(restoreOption);
    QCommandLineOption verboseOption(QStringList() << "verbose", tr("Verbose output."));
    parser.addOption(verboseOption);

    // Process the actual command line arguments given by the user.
    parser.process(app);

    if (!parser.isSet(verboseOption))
    {
        QLoggingCategory::setFilterRules("*.debug=false");
    }

    auto optionsNames = parser.optionNames();
    optionsNames.removeAll("verbose");

    if (optionsNames.isEmpty())
    {
        MainWindow w;
        w.show();
        return app.exec();
    }

    MS794 mice;

    if (!mice.ping())
    {
        qWarning() << "The device was not found.";
        return 1;
    }

    if (parser.isSet(backupOption))
    {
        qWarning() << "Reading config from the device...";

        QFile file(parser.value(backupOption));

        if (!file.open(QFile::WriteOnly))
        {
            qWarning() << "Failed to open" << file.fileName() << "for writing.";
            return 2;
        }

        if (!mice.backupConfig(&file))
        {
            qWarning() << "Failed to read the config.";
            return 3;
        }

        qWarning() << "The config has been successfully read from the device and written to" << file.fileName();
        return 0;
    }

    if (parser.isSet(restoreOption))
    {
        qWarning() << "Writing config to the device...";

        QFile file(parser.value(restoreOption));

        if (!file.open(QFile::ReadOnly))
        {
            qWarning() << "Failed to open" << file.fileName() << "for reading.";
            return 2;
        }

        if (!mice.restoreConfig(&file))
        {
            qWarning() << "Failed to write the config.";
            return 3;
        }

        qWarning() << "The config has been successfully read from " << file.fileName() << " and written to the device";
        return 0;
    }

    if (parser.isSet(profileOption))
    {
        qWarning() << mice.profile();
        return 0;
    }

    if (parser.isSet(setProfileOption))
    {
        mice.setProfile(parser.value(setProfileOption).toInt());
        mice.save();
        return 0;
    }

    if (parser.isSet(reportRateOption))
    {
        qWarning() << mice.reportRate();
        return 0;
    }

    if (parser.isSet(setReportRateOption))
    {
        mice.setReportRate(parser.value(setReportRateOption).toInt());
        mice.save();
        return 0;
    }

    // Should never happen.
    qCritical() << "Unhandled options: " << parser.optionNames();
    return 0;
}
