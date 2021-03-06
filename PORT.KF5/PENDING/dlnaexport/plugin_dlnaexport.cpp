/* ============================================================
 *
 * This file is a part of kipi-plugins project
 * http://www.digikam.org
 *
 * Date        : 2012-05-28
 * Description : A KIPI plugin to export with DLNA Technology
 *
 * Copyright (C) 2012 by Smit Mehta <smit dot meh at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "plugin_dlnaexport.h"

extern "C"
{
#include <unistd.h>
}

// Qt includes

#include <QAction>
#include <QApplication>

// KDE includes

#include <klibloader.h>
#include <klocalizedstring.h>
#include <kshortcut.h>
#include <kactioncollection.h>
#include <kstandarddirs.h>
#include <kwindowsystem.h>

// Libkipi includes

#include <KIPI/Interface>

// Local includes

#include "kipiplugins_debug.h"
#include "wizard.h"

namespace KIPIDLNAExportPlugin
{

K_PLUGIN_FACTORY(DLNAExportFactory, registerPlugin<Plugin_DLNAExport>();)

class Plugin_DLNAExport::Private
{
public:

    Private()
    {
        actionExport = 0;
        dlgExport    = 0;
    }

    QAction * actionExport;
    Wizard*  dlgExport;
};

Plugin_DLNAExport::Plugin_DLNAExport(QObject* const parent, const QVariantList&)
    : Plugin(parent, "DLNAExport"),
      d(new Private)
{
    qCDebug(KIPIPLUGINS_LOG) << "Plugin_DLNAExport plugin loaded";

    setUiBaseName("kipiplugin_dlnaexportui.rc");
    setupXML();
}

Plugin_DLNAExport::~Plugin_DLNAExport()
{
    delete d;
}

void Plugin_DLNAExport::setup(QWidget* const widget)
{
    Plugin::setup(widget);

    if (!interface())
    {
        qCCritical(KIPIPLUGINS_LOG) << "KIPI interface is null!";
        return;
    }

    setupActions();
}

void Plugin_DLNAExport::setupActions()
{
    setDefaultCategory(ExportPlugin);

    d->actionExport = new QAction(this);
    d->actionExport->setText(i18n("Export via &DLNA"));
    d->actionExport->setIcon(QIcon::fromTheme("kipi-dlna"));

    connect(d->actionExport, SIGNAL(triggered(bool)),
            this, SLOT(slotExport()));

    addAction("dlnaexport", d->actionExport);
}

void Plugin_DLNAExport::slotExport()
{
    if (!d->dlgExport)
    {
        // We clean it up in the close button
        d->dlgExport = new Wizard(QApplication::activeWindow());
    }
    else
    {
        if (d->dlgExport->isMinimized())
        {
            KWindowSystem::unminimizeWindow(d->dlgExport->winId());
            KWindowSystem::activateWindow(d->dlgExport->winId());
        }
        else
        {
            delete d->dlgExport;
            d->dlgExport = new Wizard(QApplication::activeWindow());
        }
    }

    d->dlgExport->show();
}

}  // namespace KIPIDLNAExportPlugin

#include "plugin_dlnaexport.moc"
