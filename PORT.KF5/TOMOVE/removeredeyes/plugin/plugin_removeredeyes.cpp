/* ============================================================
 *
 * This file is a part of kipi-plugins project
 * http://www.digikam.org
 *
 * Date        : 2008-06-08
 * Description : a kipi plugin to automatically detect
 *               and remove red eyes from images
 *
 * Copyright (C) 2008-2009 by Andi Clemens <andi dot clemens at googlemail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "plugin_removeredeyes.h"

// Qt includes

#include <QAction>
#include <QApplication>

// KDE includes

#include <kactioncollection.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <klocalizedstring.h>

// Libkipi includes

#include <KIPI/ImageCollection>
#include <KIPI/Interface>

// Local includes

#include "kipiplugins_debug.h"
#include "removeredeyeswindow.h"

namespace KIPIRemoveRedEyesPlugin
{

K_PLUGIN_FACTORY( RemoveRedEyesFactory, registerPlugin<Plugin_RemoveRedEyes>(); )

Plugin_RemoveRedEyes::Plugin_RemoveRedEyes(QObject* const parent, const QVariantList& /*args*/)
    : Plugin(parent, "RemoveRedEyes"),
      m_action(0)
{
    qCDebug(KIPIPLUGINS_LOG) << "Plugin_RemoveRedEyes plugin loaded";

    setUiBaseName("kipiplugin_removeredeyesui.rc");
    setupXML();
}

Plugin_RemoveRedEyes::~Plugin_RemoveRedEyes()
{
}

void Plugin_RemoveRedEyes::setup(QWidget* const widget)
{
    Plugin::setup(widget);

    setupActions();

    if (!interface())
    {
        qCCritical(KIPIPLUGINS_LOG) << "Kipi interface is null!";
        return;
    }

    ImageCollection selection = interface()->currentSelection();
    m_action->setEnabled(selection.isValid() && !selection.images().isEmpty() );

    connect(interface(), SIGNAL(selectionChanged(bool)),
            m_action, SLOT(setEnabled(bool)));
}

void Plugin_RemoveRedEyes::setupActions()
{
    setDefaultCategory(BatchPlugin);

    m_action = new QAction(this);
    m_action->setText(i18n("&Red-Eye Removal..."));
    m_action->setIcon(QIcon::fromTheme("draw-eraser"));

    connect(m_action, SIGNAL(triggered(bool)),
            this, SLOT(activate()));

    addAction("removeredeyes", m_action);
}

void Plugin_RemoveRedEyes::activate()
{
    RemoveRedEyesWindow* const window = new RemoveRedEyesWindow();
    window->show();
}

} // namespace KIPIRemoveRedEyesPlugin

#include "plugin_removeredeyes.moc"
