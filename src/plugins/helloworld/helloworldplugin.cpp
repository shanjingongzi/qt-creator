// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helloworldplugin.h"

#include "helloworldtr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <QDebug>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

namespace HelloWorld::Internal {

/*!  A mode with a push button based on BaseMode.  */

class HelloMode : public Core::IMode
{
public:
    HelloMode()
    {
        setWidget(new QPushButton(Tr::tr("Hello World PushButton!")));
        setContext(Core::Context("HelloWorld.MainView"));
        setDisplayName(Tr::tr("Hello world!"));
        setIcon(QIcon());
        setPriority(0);
        setId("HelloWorld.HelloWorldMode");
    }
};


/*! Constructs the Hello World plugin. Normally plugins don't do anything in
    their constructor except for initializing their member variables. The
    actual work is done later, in the initialize() and extensionsInitialized()
    functions.
*/
HelloWorldPlugin::HelloWorldPlugin()
{
}

/*! Plugins are responsible for deleting objects they created on the heap, and
    to unregister objects from the plugin manager that they registered there.
*/
HelloWorldPlugin::~HelloWorldPlugin()
{
    delete m_helloMode;
}

/*! Initializes the plugin.
    Plugins want to register objects with the plugin manager here.

*/
void HelloWorldPlugin::initialize()
{
    // Create a unique context for our own view, that will be used for the
    // menu entry later.
    Core::Context context("HelloWorld.MainView");

    // Create our own menu to place in the Tools menu
    Utils::Id menuId = "HelloWorld.HelloWorldMenu";

    Core::ActionContainer *helloWorldMenu = Core::ActionManager::createMenu(menuId);
    QMenu *menu = helloWorldMenu->menu();
    menu->setTitle(Tr::tr("&Hello World"));
    menu->setEnabled(true);

    // Request the Tools menu and add the Hello World menu to it
    Core::ActionContainer *toolsMenu =
            Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(helloWorldMenu);

    // Create an action to be triggered by a menu entry.
    // The Action builder registers the action with the action manager
    // on its destruction.
    Core::ActionBuilder hello(this, "HelloWorld.HelloWorldAction");
    hello.setText(Tr::tr("Say \"&Hello World!\""));
    hello.setContext(context);
    hello.setOnTriggered(this, [this] { sayHelloWorld(); });

    // Add the Hello World action command to the menu
    hello.setContainer(menuId);

    // Add a mode with a push button based on BaseMode.
    m_helloMode = new HelloMode;
}

/*! Notification that all extensions that this plugin depends on have been
    initialized. The dependencies are defined in the plugins .json(.in) file.

    Normally this function is used for things that rely on other plugins to have
    added objects to the plugin manager, that implement interfaces that we're
    interested in. These objects can now be requested through the
    PluginManager.

    The HelloWorldPlugin doesn't need things from other plugins, so it does
    nothing here.
*/
void HelloWorldPlugin::extensionsInitialized()
{
}

void HelloWorldPlugin::sayHelloWorld()
{
    // When passing nullptr for the parent, the message box becomes an
    // application-global modal dialog box
    QMessageBox::information(nullptr,
                             Tr::tr("Hello World!"),
                             Tr::tr("Hello World! Beautiful day today, isn't it?"));
}

} // namespace HelloWorld::Internal
