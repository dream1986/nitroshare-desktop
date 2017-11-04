/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Nathan Osman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostInfo>
#include <QUuid>

#include <nitroshare/application.h>

#include "application_p.h"

const QString PluginDir = "plugin-dir";
const QString PluginBlacklist = "plugin-blacklist";

const QString Application::DeviceCategoryName = "device";
const QString Application::DeviceUuidSettingName = "DeviceUuid";
const QString Application::DeviceNameSettingName = "DeviceName";

ApplicationPrivate::ApplicationPrivate(Application *application)
    : QObject(application),
      q(application),
      deviceCategory({
          { Category::NameKey, Application::DeviceCategoryName },
          { Category::TitleKey, tr("Device") }
      }),
      deviceUuid({
          { Setting::TypeKey, Setting::String },
          { Setting::NameKey, Application::DeviceUuidSettingName },
          { Setting::TitleKey, tr("Device UUID") },
          { Setting::IsHiddenKey, true },
          { Setting::DefaultValueKey, QUuid::createUuid().toString() }
      }),
      deviceName({
          { Setting::TypeKey, Setting::String },
          { Setting::NameKey, Application::DeviceNameSettingName },
          { Setting::TitleKey, tr("Device Name") },
          { Setting::CategoryKey, Application::DeviceCategoryName },
          { Setting::DefaultValueKey, QHostInfo::localHostName() }
      }),
      pluginModel(application),
      settingsRegistry(&settings),
      transferModel(application),
      uiEnabled(false)
{
    settingsRegistry.addCategory(&deviceCategory);
    settingsRegistry.addSetting(&deviceUuid);
    settingsRegistry.addSetting(&deviceName);
}

ApplicationPrivate::~ApplicationPrivate()
{
    settingsRegistry.removeSetting(&deviceUuid);
    settingsRegistry.removeSetting(&deviceName);
    settingsRegistry.removeCategory(&deviceCategory);
}

Application::Application(QObject *parent)
    : QObject(parent),
      d(new ApplicationPrivate(this))
{
}

Application::~Application()
{
    // Unload all plugins before the remaining members are destroyed
    d->pluginModel.unloadAll();
}

void Application::addCliOptions(QCommandLineParser *parser)
{
    parser->addOption(QCommandLineOption(PluginDir, tr("load plugins in directory"), tr("directory"), NITROSHARE_PLUGIN_PATH));
    parser->addOption(QCommandLineOption(PluginBlacklist, tr("do not load plugin"), tr("plugin")));
}

void Application::processCliOptions(QCommandLineParser *parser)
{
    // Blacklist the plugins that were specified
    pluginModel()->addToBlacklist(parser->values(PluginBlacklist));

    // Load all plugins in the directories
    pluginModel()->loadPluginsFromDirectories(parser->values(PluginDir));
}

QString Application::deviceUuid() const
{
    return d->settingsRegistry.value(DeviceUuidSettingName).toString();
}

QString Application::deviceName() const
{
    return d->settingsRegistry.value(DeviceNameSettingName).toString();
}

QString Application::version() const
{
    return NITROSHARE_VERSION;
}

ActionRegistry *Application::actionRegistry() const
{
    return &d->actionRegistry;
}

DeviceModel *Application::deviceModel() const
{
    return &d->deviceModel;
}

HandlerRegistry *Application::handlerRegistry() const
{
    return &d->handlerRegistry;
}

Logger *Application::logger() const
{
    return &d->logger;
}

PluginModel *Application::pluginModel() const
{
    return &d->pluginModel;
}

SettingsRegistry *Application::settingsRegistry() const
{
    return &d->settingsRegistry;
}

TransferModel *Application::transferModel() const
{
    return &d->transferModel;
}

bool Application::isUiEnabled() const
{
    return d->uiEnabled;
}

void Application::setUiEnabled(bool uiEnabled)
{
    d->uiEnabled = uiEnabled;
}

Application::OperatingSystem Application::operatingSystem() const
{
#if defined(Q_OS_WIN32)
    return Windows;
#elif defined(Q_OS_MACX)
    return MacOS;
#elif defined(Q_OS_LINUX)
    return Linux;
#else
    return UnknownOperatingSystem;
#endif
}

Application::Architecture Application::architecture() const
{
    // Note: Qt doesn't provide the definitions we need to accurately
    // determine the CPU architecture - in order to do that, we'll need
    // to check a few preprocessor definitions ourselves

#if defined(__i386__) || defined(_M_IX86)
    return X86;
#elif defined(__x86_64__) || defined(_M_X64)
    return X86_64;
#else
    return UnknownArchitecture;
#endif
}

void Application::quit()
{
    QCoreApplication::quit();
}
