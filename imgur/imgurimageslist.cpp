/* ============================================================
 *
 * This file is a part of kipi-plugins project
 * http://www.kipi-plugins.org
 *
 * Date        : 2010-02-04
 * Description : a tool to export images to imgur.com
 *
 * Copyright (C) 2010-2012 by Marius Orcsik <marius at habarnam dot ro>
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

#include "imgurimageslist.h"

// Qt includes

#include <QLabel>
#include <QPointer>
#include <QDesktopServices>

// KDE includes

#include <klocalizedstring.h>

// Local includes

#include "kipiplugins_debug.h"

namespace KIPIImgurPlugin
{

ImgurImagesList::ImgurImagesList(QWidget* const parent)
    : KPImagesList(parent)
{
    setControlButtonsPlacement(KPImagesList::ControlButtonsBelow);
    setAllowDuplicate(false);
    setAllowRAW(false);

    listView()->setColumnLabel(KPImagesListView::Thumbnail, i18n("Thumbnail"));

    //listView()->setColumnLabel(KPImagesListView::Filename, i18n("File name"));

    listView()->setColumnLabel(static_cast<KIPIPlugins::KPImagesListView::ColumnType>(ImgurImagesList::Title),
                               i18n("Submission title"));

    listView()->setColumnLabel(static_cast<KIPIPlugins::KPImagesListView::ColumnType>(ImgurImagesList::Description),
                               i18n("Submission description"));

    listView()->setColumn(static_cast<KIPIPlugins::KPImagesListView::ColumnType>(ImgurImagesList::URL),
                          i18n("Imgur URL"), true);

    listView()->setColumn(static_cast<KIPIPlugins::KPImagesListView::ColumnType>(ImgurImagesList::DeleteURL),
                          i18n("Imgur Delete URL"), true);

    connect(listView(), SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotDoubleClick(QTreeWidgetItem*,int)));
}

ImgurImagesList::~ImgurImagesList()
{
}

void ImgurImagesList::updateItemWidgets()
{
    qCDebug(KIPIPLUGINS_LOG) << "update";
}

void ImgurImagesList::slotAddImages(const QList<QUrl>& list)
{
    /* Replaces the KPImagesList::slotAddImages method, so that
     * ImgurImageListViewItems can be added instead of ImagesListViewItems */

    // Figure out which of the supplied URL's should actually be added and which
    // of them already exist.
    bool found;

    for (QList<QUrl>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        QUrl imageUrl = *it;
        found         = false;

        if (iface())
        {
            for (int i = 0; i < listView()->topLevelItemCount(); ++i)
            {
                ImgurImageListViewItem* const currItem = dynamic_cast<ImgurImageListViewItem*>(listView()->topLevelItem(i));

                if (currItem && currItem->url() == imageUrl)
                {
                    found = true;
                    break;
                }
            }

            QPointer<MetadataProcessor> meta = iface()->createMetadataProcessor();

            if (!found && meta && meta->load(imageUrl))
            {
                ImgurImageListViewItem* const currItem = new ImgurImageListViewItem(listView(), imageUrl);

                const QString sUrl       = meta->getXmpTagString(QLatin1String("Xmp.kipi.ImgurId"));
                const QString sDeleteUrl = meta->getXmpTagString(QLatin1String("Xmp.kipi.ImgurDeleteHash"));

                if (!sUrl.isEmpty())
                {
                    currItem->setUrl(QLatin1String(ImgurConnection::pageURL(sUrl).toEncoded()));
                }

                if (!sDeleteUrl.isEmpty())
                {
                    currItem->setDeleteUrl(QLatin1String(ImgurConnection::deleteURL(sDeleteUrl).toEncoded()));
                }
            }
        }
    }

    // Duplicate the signalImageListChanged of the ImageWindow, to enable the
    // upload button again.
    emit signalImageListChanged();
    emit signalAddItems(list);
}

void ImgurImagesList::slotUploadError(const QUrl& /*localFile*/, const ImgurError& /*error*/)
{
    // TODO
}

void ImgurImagesList::slotUploadSuccess(const QUrl& localFile, const ImgurSuccess& success)
{
    for (int i = 0; i < listView()->topLevelItemCount(); ++i)
    {
        ImgurImageListViewItem* const currItem = dynamic_cast<ImgurImageListViewItem*>(listView()->topLevelItem(i));

        if (currItem && currItem->url() == localFile)
        {
            if (!success.image.id.isEmpty())
            {
                const QString sUrl = QLatin1String(ImgurConnection::pageURL(success.image.id).toEncoded());
                currItem->setUrl(sUrl);
            }

            if (!success.image.deletehash.isEmpty())
            {
                const QString sDeleteUrl = QLatin1String(ImgurConnection::deleteURL(success.image.deletehash).toEncoded());
                currItem->setDeleteUrl(sDeleteUrl);
            }

            break;
        }
    }
}

void ImgurImagesList::slotDoubleClick(QTreeWidgetItem* element, int i)
{
    if (i == 3 || i == 4)
    {
        const QUrl url = QUrl (element->text(i));
        // need to check for delete url click - and ask user if he wants to remove the tags also
        QDesktopServices::openUrl(url);
    }
}

// ------------------------------------------------------------------------------------------------

ImgurImageListViewItem::ImgurImageListViewItem(KPImagesListView* const view, const QUrl& url)
    : KPImagesListViewItem(view, url)
{
    const QColor blue = QColor (50, 50, 255);

    setTextColor(3, blue);
    setTextColor(4, blue);
}

ImgurImageListViewItem::~ImgurImageListViewItem()
{
}

void ImgurImageListViewItem::setTitle(const QString& str)
{
    setText(ImgurImagesList::Title, str);
    m_Title = str;
}

QString ImgurImageListViewItem::Title() const
{
    return m_Title;
}

void ImgurImageListViewItem::setDescription(const QString& str)
{
    setText(ImgurImagesList::Description, str);
    m_Description = str;
}

QString ImgurImageListViewItem::Description() const
{
    return m_Description;
}

void ImgurImageListViewItem::setUrl(const QString& str)
{
    setText(ImgurImagesList::URL, str);
    m_Url = str;
}

QString ImgurImageListViewItem::Url() const
{
    return m_Url;
}

void ImgurImageListViewItem::setDeleteUrl(const QString& str)
{
    setText(ImgurImagesList::DeleteURL, str);
    m_deleteUrl = str;
}

QString ImgurImageListViewItem::deleteUrl() const
{
    return m_deleteUrl;
}

} // namespace KIPIImgurPlugin
