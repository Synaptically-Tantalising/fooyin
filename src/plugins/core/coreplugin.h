/*
 * Fooyin
 * Copyright 2022, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <pluginsystem/plugin.h>

class Application;

class CorePlugin : public QObject,
                   public PluginSystem::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.fooyin.plugin" FILE "metadata.json")
    Q_INTERFACES(PluginSystem::Plugin)

public:
    CorePlugin();
    ~CorePlugin() override;

    void initialise() override;
    void finalise() override;
    void shutdown() override;

private:
    Application* m_app;
};
