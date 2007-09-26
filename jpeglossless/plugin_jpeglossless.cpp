/* ============================================================
 *
 * This file is a part of kipi-plugins project
 * http://www.kipi-plugins.org
 *
 * Date        : 2003-09-26
 * Description : loss less images transformations plugin.
 *
 * Copyright (C) 2003-2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2004-2007 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 * Copyright (C) 2006-2007 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

// C++ includes.

#include <iostream>

// Qt includes.

#include <kurl.h>

// KDE includes.

#include <kshortcut.h>
#include <klocale.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kstandardguiitem.h>

// LibKipi includes.

#include <libkipi/batchprogressdialog.h>
#include <libkipi/interface.h>

// Local includes.

#include "actionthread.h"
#include "plugin_jpeglossless.h"
#include "plugin_jpeglossless.moc"

K_PLUGIN_FACTORY( JPEGLosslessFactory, registerPlugin<Plugin_JPEGLossless>(); )
K_EXPORT_PLUGIN ( JPEGLosslessFactory("kipiplugin_jpeglossless") )

Plugin_JPEGLossless::Plugin_JPEGLossless(QObject *parent, const QVariantList &)
                   : KIPI::Plugin( JPEGLosslessFactory::componentData(), parent, "JPEGLossless")
{
    m_total                    = 0;
    m_current                  = 0;
    m_action_Convert2GrayScale = 0;
    m_action_AutoExif          = 0;
    m_action_RotateImage       = 0;
    m_action_FlipImage         = 0;
    m_progressDlg              = 0;
    m_thread                   = 0;
    m_failed                   = false;

    kDebug( 51001 ) << "Plugin_JPEGLossless plugin loaded";
}

void Plugin_JPEGLossless::setup( QWidget* widget )
{
    KIPI::Plugin::setup( widget );

    m_action_AutoExif = new KAction(i18n("Auto Rotate/Flip Using Exif Information"), actionCollection());
    m_action_AutoExif->setObjectName("rotate_exif");
    connect(m_action_AutoExif, SIGNAL(triggered(bool)), 
            this, SLOT(slotRotateExif()));
    addAction(m_action_AutoExif);

    m_action_RotateImage = new KActionMenu(KIcon("rotate"), i18n("Rotate"), actionCollection());
    m_action_RotateImage->setObjectName("jpeglossless_rotate");
    addAction(m_action_RotateImage);

    KAction *left = new KAction(i18n("Left"), actionCollection());
    left->setObjectName("rotate_ccw");
    left->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Left);
    connect(left, SIGNAL(triggered(bool)), 
            this, SLOT(slotRotateLeft()));
    m_action_RotateImage->addAction(left);

    KAction *right = new KAction(i18n("Right"), actionCollection());
    right->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Right);
    right->setObjectName("rotate_cw");
    connect(right, SIGNAL(triggered(bool)), 
            this, SLOT(slotRotateRight()));
    m_action_RotateImage->addAction(right);

    m_action_FlipImage = new KActionMenu(KIcon("flip"), i18n("Flip"), actionCollection());
    addAction(m_action_FlipImage);

    KAction *hori = new KAction(i18n("Horizontally"), actionCollection());
    hori->setShortcut(Qt::CTRL+Qt::Key_Asterisk);
    hori->setObjectName("flip_horizontal");
    connect(hori, SIGNAL(triggered(bool)), 
            this, SLOT(slotFlipHorizontally()));
    m_action_FlipImage->addAction(hori);

    KAction *verti = new KAction(i18n("Vertically"), actionCollection());
    verti->setShortcut(Qt::CTRL+Qt::Key_Slash);
    verti->setObjectName("flip_vertical");
    connect(verti, SIGNAL(triggered(bool)), 
            this, SLOT(slotFlipVertically()));
    m_action_FlipImage->addAction(verti);

    m_action_Convert2GrayScale = new KAction(KIcon("grayscaleconvert"), i18n("Convert to Black && White"), actionCollection());
    m_action_Convert2GrayScale->setObjectName("jpeglossless_convert2grayscale");
    connect(m_action_Convert2GrayScale, SIGNAL(triggered(bool)), 
            this, SLOT(slotConvert2GrayScale()));
    addAction(m_action_Convert2GrayScale);

    KIPI::Interface* interface = dynamic_cast<KIPI::Interface*>( parent() );
    if ( !interface ) 
    {
        kError( 51000 ) << "Kipi interface is null!" << endl;
        return;
    }

    m_action_AutoExif->setEnabled( false );
    m_action_RotateImage->setEnabled( false );
    m_action_FlipImage->setEnabled( false );
    m_action_Convert2GrayScale->setEnabled( false );

    m_thread = new KIPIJPEGLossLessPlugin::ActionThread(interface, this);

    connect( m_thread, SIGNAL(starting(const QString &, int)),
             this, SLOT(slotStarting(const QString &, int)));

    connect( m_thread, SIGNAL(finished(const QString &, int)),
             this, SLOT(slotFinished(const QString &, int)));

    connect( m_thread, SIGNAL(failed(const QString &, int, const QString &)),
             this, SLOT(slotFailed(const QString &, int, const QString &)));

    connect( interface, SIGNAL( selectionChanged( bool ) ),
             m_action_AutoExif, SLOT( setEnabled( bool ) ) );

    connect( interface, SIGNAL( selectionChanged( bool ) ),
             m_action_RotateImage, SLOT( setEnabled( bool ) ) );

    connect( interface, SIGNAL( selectionChanged( bool ) ),
             m_action_FlipImage, SLOT( setEnabled( bool ) ) );

    connect( interface, SIGNAL( selectionChanged( bool ) ),
             m_action_Convert2GrayScale, SLOT( setEnabled( bool ) ) );
}

Plugin_JPEGLossless::~Plugin_JPEGLossless()
{
    delete m_progressDlg;
}

void Plugin_JPEGLossless::slotFlipHorizontally()
{
    flip(KIPIJPEGLossLessPlugin::FlipHorizontal, i18n("horizontally"));
}

void Plugin_JPEGLossless::slotFlipVertically()
{
    flip(KIPIJPEGLossLessPlugin::FlipVertical, i18n("vertically"));
}

void Plugin_JPEGLossless::flip(KIPIJPEGLossLessPlugin::FlipAction action, const QString &title)
{
    KUrl::List items = images();
    if (items.count() <= 0) return;

    m_thread->flip(items, action);

    m_total   = items.count();
    m_current = 0;
    m_failed  = false;

    if (m_progressDlg) 
    {
        delete m_progressDlg;
        m_progressDlg = 0;
    }

    m_progressDlg = new KIPI::BatchProgressDialog(kapp->activeWindow(), 
                        i18n("Flip images %1", title));

    connect(m_progressDlg, SIGNAL(cancelClicked()),
            this, SLOT(slotCancel()));

    m_progressDlg->show();

    if (!m_thread->isRunning())
        m_thread->start();
}

void Plugin_JPEGLossless::slotRotateRight()
{
    rotate(KIPIJPEGLossLessPlugin::Rot90, i18n("right (clockwise)"));
}

void Plugin_JPEGLossless::slotRotateLeft()
{
    rotate(KIPIJPEGLossLessPlugin::Rot270, i18n("left (counterclockwise)"));
}

void Plugin_JPEGLossless::slotRotateExif()
{
    rotate(KIPIJPEGLossLessPlugin::Rot0, i18n("using Exif orientation tag"));
}

void Plugin_JPEGLossless::rotate(KIPIJPEGLossLessPlugin::RotateAction action, const QString &title)
{
    KUrl::List items = images();
    if (items.count() <= 0) return;

    m_thread->rotate(items, action);

    m_total   = items.count();
    m_current = 0;
    m_failed  = false;

    if (m_progressDlg) 
    {
        delete m_progressDlg;
        m_progressDlg = 0;
    }

    m_progressDlg = new KIPI::BatchProgressDialog(kapp->activeWindow(), 
                        i18n("Rotate images %1", title));

    connect(m_progressDlg, SIGNAL(cancelClicked()),
            this, SLOT(slotCancel()));

    m_progressDlg->show();

    if (!m_thread->isRunning())
        m_thread->start();
}

void Plugin_JPEGLossless::slotConvert2GrayScale()
{
    KUrl::List items = images();
    if (items.count() <= 0 ||
        KMessageBox::No==KMessageBox::warningYesNo(kapp->activeWindow(),
                     i18n("<p>Are you sure you wish to convert the selected image(s) to "
                         "black and white? This operation <b>cannot</b> be undone.</p>")))
        return;

    QString from(sender()->objectName());

    m_total   = items.count();
    m_current = 0;
    m_failed  = false;

    if (m_progressDlg) 
    {
        delete m_progressDlg;
        m_progressDlg = 0;
    }

    m_progressDlg = new KIPI::BatchProgressDialog(kapp->activeWindow(), 
                        i18n("Convert images to black & white"));

    connect(m_progressDlg, SIGNAL(cancelClicked()),
            this, SLOT(slotCancel()));

    m_progressDlg->show();

    m_thread->convert2grayscale(items);
    if (!m_thread->isRunning())
        m_thread->start();
}

void Plugin_JPEGLossless::slotCancel()
{
    m_thread->cancel();

    KIPI::Interface* interface = dynamic_cast<KIPI::Interface*>( parent() );

    if ( !interface ) 
    {
        kError( 51000 ) << "Kipi interface is null!" << endl;
        return;
    }

    interface->refreshImages( m_images );
}

void Plugin_JPEGLossless::slotStarting(const QString &filePath, int action)
{
    QString text;

    switch ((KIPIJPEGLossLessPlugin::Action)action)
    {
        case(KIPIJPEGLossLessPlugin::Rotate):
        {
            text = i18n("Rotating Image \"%1\"", filePath.section('/', -1));
            break;
        }
        case(KIPIJPEGLossLessPlugin::Flip):
        {
            text = i18n("Flipping Image \"%1\"", filePath.section('/', -1));
            break;
        }
        case(KIPIJPEGLossLessPlugin::GrayScale):
        {
            text = i18n("Converting to Black & White \"%1\"", filePath.section('/', -1));
            break;
        }
        default:
        {
            kWarning( 51000 ) << "KIPIJPEGLossLessPlugin: Unknown event";
        }
    }

    m_progressDlg->addedAction(text, KIPI::StartingMessage);
}

void Plugin_JPEGLossless::slotFinished(const QString &filePath, int action)
{
    Q_UNUSED(filePath);

    QString text;

    switch ((KIPIJPEGLossLessPlugin::Action)action)
    {
        case(KIPIJPEGLossLessPlugin::Rotate):
        {
            text = i18n("Rotate image complete");
            break;
        }
        case(KIPIJPEGLossLessPlugin::Flip):
        {
            text = i18n("Flip image complete");
            break;
        }
        case(KIPIJPEGLossLessPlugin::GrayScale):
        {
            text = i18n("Convert to Black & White complete");
            break;
        }
        default:
        {
            kWarning( 51000 ) << "KIPIJPEGLossLessPlugin: Unknown event";
        }
    }

    m_progressDlg->addedAction(text, KIPI::SuccessMessage);

    oneTaskCompleted();
}

void Plugin_JPEGLossless::slotFailed(const QString &filePath, int action, const QString &errString)
{
    Q_UNUSED(filePath);

    m_failed = true;
    QString text;

    switch ((KIPIJPEGLossLessPlugin::Action)action)
    {
        case(KIPIJPEGLossLessPlugin::Rotate):
        {
            text = i18n("Failed to Rotate image");
            break;
        }
        case(KIPIJPEGLossLessPlugin::Flip):
        {
            text = i18n("Failed to Flip image");
            break;
        }
        case(KIPIJPEGLossLessPlugin::GrayScale):
        {
            text = i18n("Failed to convert image to Black & White");
            break;
        }
        default:
        {
            kWarning( 51000 ) << "KIPIJPEGLossLessPlugin: Unknown event";
        }
    }

    m_progressDlg->addedAction(text, KIPI::WarningMessage);

    if (!errString.isEmpty())
        m_progressDlg->addedAction(errString, KIPI::WarningMessage);

    oneTaskCompleted();
}

void Plugin_JPEGLossless::oneTaskCompleted()
{
    m_current++;
    m_progressDlg->setProgress(m_current, m_total);

    if (m_current >= m_total)
    {
        m_current = 0;

        if (m_failed)
        {
            m_progressDlg->setButtonGuiItem(KDialog::Cancel, KStandardGuiItem::close());

            disconnect(m_progressDlg, SIGNAL(cancelClicked()),
                       this, SLOT(slotCancel()));
        }
        else 
        {
            slotCancel();
            m_progressDlg->close();
            m_progressDlg = 0;
        }

        KIPI::Interface* interface = dynamic_cast<KIPI::Interface*>( parent() );

        if ( !interface ) 
        {
           kError( 51000 ) << "Kipi interface is null!" << endl;
           return;
        }

        interface->refreshImages( m_images );
    }
}

KIPI::Category Plugin_JPEGLossless::category( KAction* action ) const
{
    if (action == m_action_AutoExif)
        return KIPI::IMAGESPLUGIN;
    else if ( action == m_action_RotateImage )
       return KIPI::IMAGESPLUGIN;
    else if ( action == m_action_FlipImage )
       return KIPI::IMAGESPLUGIN;
    else if ( action == m_action_Convert2GrayScale )
       return KIPI::IMAGESPLUGIN;

    kWarning( 51000 ) << "Unrecognized action for plugin category identification";
    return KIPI::IMAGESPLUGIN; // no warning from compiler, please
}

KUrl::List Plugin_JPEGLossless::images()
{
    KIPI::Interface* interface = dynamic_cast<KIPI::Interface*>( parent() );

    if ( !interface ) 
    {
        kError( 51000 ) << "Kipi interface is null!" << endl;
        return KUrl::List();
    }

    KIPI::ImageCollection images = interface->currentSelection();
    if ( !images.isValid() )
        return KUrl::List();

    // We don't want the set of images to change before we are done
    // and tells the host app to refresh the images.
    m_images = images.images();
    return images.images();
}
