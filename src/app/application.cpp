/*
 * Fooyin
 * Copyright 2022-2023, Luke Taylor <LukeT1@proton.me>
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

#include "application.h"

#include <core/actions/actionmanager.h>
#include <core/app/threadmanager.h>
#include <core/coresettings.h>
#include <core/database/database.h>
#include <core/engine/enginehandler.h>
#include <core/library/librarymanager.h>
#include <core/library/libraryscanner.h>
#include <core/library/musiclibrary.h>
#include <core/player/playercontroller.h>
#include <core/playlist/libraryplaylistmanager.h>
#include <core/playlist/playlisthandler.h>
#include <core/plugins/databaseplugin.h>
#include <core/plugins/pluginmanager.h>
#include <core/plugins/settingsplugin.h>
#include <core/plugins/threadplugin.h>
#include <core/plugins/widgetplugin.h>
#include <gui/controls/controlwidget.h>
#include <gui/editablelayout.h>
#include <gui/guisettings.h>
#include <gui/info/infowidget.h>
#include <gui/library/coverwidget.h>
#include <gui/library/statuswidget.h>
#include <gui/mainwindow.h>
#include <gui/playlist/playlistwidget.h>
#include <gui/settings/settingsdialog.h>
#include <gui/widgetfactory.h>
#include <gui/widgetprovider.h>
#include <gui/widgets/spacer.h>
#include <gui/widgets/splitterwidget.h>

#include <utils/paths.h>

struct Application::Private
{
    Core::ActionManager actionManager;
    Core::SettingsManager settingsManager;
    Core::Settings::CoreSettings coreSettings;
    Core::ThreadManager threadManager;
    Core::DB::Database database;
    std::unique_ptr<Core::Player::PlayerManager> playerManager;
    Core::Engine::EngineHandler engine;
    Core::Playlist::PlaylistHandler playlistHandler;
    std::unique_ptr<Core::Playlist::LibraryPlaylistInterface> playlistInterface;
    Core::Library::LibraryManager libraryManager;
    Core::Library::MusicLibrary library;

    Gui::Widgets::WidgetFactory widgetFactory;
    Gui::Widgets::WidgetProvider widgetProvider;
    Gui::Settings::GuiSettings guiSettings;
    Gui::Settings::SettingsDialog* settingsDialog;
    Gui::Widgets::EditableLayout* editableLayout;
    Gui::MainWindow* mainWindow;

    Plugins::PluginManager pluginManager;
    WidgetPluginContext widgetContext;
    ThreadPluginContext threadContext;
    DatabasePluginContext databaseContext;
    SettingsPluginContext settingsContext;

    explicit Private()
        : coreSettings{&settingsManager}
        , database{&settingsManager}
        , playerManager{std::make_unique<Core::Player::PlayerController>(&settingsManager)}
        , engine{playerManager.get()}
        , playlistHandler{playerManager.get()}
        , playlistInterface{std::make_unique<Core::Playlist::LibraryPlaylistManager>(&playlistHandler)}
        , libraryManager{&database}
        , library{playlistInterface.get(), &libraryManager, &threadManager, &database, &settingsManager}
        , widgetProvider{&widgetFactory}
        , guiSettings{&settingsManager}
        , settingsDialog{new Gui::Settings::SettingsDialog(&libraryManager, &settingsManager)}
        , editableLayout{new Gui::Widgets::EditableLayout(&settingsManager, &actionManager, &widgetFactory,
                                                          &widgetProvider)}
        , mainWindow{new Gui::MainWindow(&actionManager, &settingsManager, settingsDialog, editableLayout)}
        , widgetContext{&actionManager, playerManager.get(), &library, &widgetFactory}
        , threadContext{&threadManager}
        , databaseContext{&database}
        , settingsContext{&settingsManager, settingsDialog}
    {
        actionManager.setMainWindow(mainWindow);
        mainWindow->setAttribute(Qt::WA_DeleteOnClose);
        threadManager.moveToNewThread(&engine);

        setupConnections();
        registerWidgets();

        const QString pluginsPath = QCoreApplication::applicationDirPath() + "/../lib/fooyin/plugins";
        pluginManager.findPlugins(pluginsPath);
        pluginManager.loadPlugins();
        initialisePlugins();
    }

    void setupConnections() const
    {
        connect(&libraryManager, &Core::Library::LibraryManager::libraryAdded, &library,
                &Core::Library::MusicLibrary::reload);
        connect(&libraryManager, &Core::Library::LibraryManager::libraryRemoved, &library,
                &Core::Library::MusicLibrary::refresh);
    }

    void registerWidgets()
    {
        widgetFactory.registerClass<Gui::Widgets::ControlWidget>("Controls", [this]() {
            return new Gui::Widgets::ControlWidget(playerManager.get(), &settingsManager);
        });

        widgetFactory.registerClass<Gui::Widgets::InfoWidget>("Info", [this]() {
            return new Gui::Widgets::InfoWidget(playerManager.get(), &settingsManager);
        });

        widgetFactory.registerClass<Gui::Widgets::CoverWidget>("Artwork", [this]() {
            return new Gui::Widgets::CoverWidget(&library, playerManager.get());
        });

        widgetFactory.registerClass<Gui::Widgets::PlaylistWidget>("Playlist", [this]() {
            return new Gui::Widgets::PlaylistWidget(&libraryManager, &library, playerManager.get(), settingsDialog,
                                                    &settingsManager);
        });

        widgetFactory.registerClass<Gui::Widgets::Spacer>("Spacer", []() {
            return new Gui::Widgets::Spacer();
        });

        widgetFactory.registerClass<Gui::Widgets::VerticalSplitterWidget>(
            "Vertical Splitter",
            [this]() {
                return new Gui::Widgets::VerticalSplitterWidget(&actionManager, &widgetProvider, &settingsManager);
            },
            {"Splitter"});

        widgetFactory.registerClass<Gui::Widgets::HorizontalSplitterWidget>(
            "Horizontal Splitter",
            [this]() {
                return new Gui::Widgets::HorizontalSplitterWidget(&actionManager, &widgetProvider, &settingsManager);
            },
            {"Splitter"});

        widgetFactory.registerClass<Gui::Widgets::StatusWidget>("Status", [this]() {
            return new Gui::Widgets::StatusWidget(playerManager.get());
        });
    }

    void initialisePlugins()
    {
        pluginManager.initialisePlugins<Plugins::WidgetPlugin>(widgetContext);
        pluginManager.initialisePlugins<Plugins::SettingsPlugin>(settingsContext);
        pluginManager.initialisePlugins<Plugins::ThreadPlugin>(threadContext);
        pluginManager.initialisePlugins<Plugins::DatabasePlugin>(databaseContext);
        pluginManager.initialisePlugins();
    }
};

Application::Application(int& argc, char** argv, int flags)
    : QApplication{argc, argv, flags}
    , p{std::make_unique<Private>()}
{
    // Shutdown plugins on exit
    // Required to ensure plugins are unloaded before main event loop quits
    QObject::connect(this, &QCoreApplication::aboutToQuit, &p->threadManager, &Core::ThreadManager::close);
    QObject::connect(this, &QCoreApplication::aboutToQuit, &p->pluginManager, &Plugins::PluginManager::shutdown);

    startup();
}

void Application::startup()
{
    p->settingsManager.loadSettings();
    p->playerManager->restoreState();
    p->library.load();

    p->settingsDialog->setupUi();
    p->mainWindow->setupUi();
    p->mainWindow->show();
}

Application::~Application()
{
    shutdown();
};

void Application::shutdown()
{
    p->settingsManager.storeSettings();

    p->database.cleanup();
    p->database.closeDatabase();
}
