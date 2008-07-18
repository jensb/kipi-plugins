/* ============================================================
 * File  : gallerympform.cpp
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2004-12-02
 * Description :
 *
 * Copyright 2004 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 *
 * Modified by : Andrea Diamantini <adjam7@gmail.com>
 * Date        : 2008-07-11
 * Copyright 2008 by Andrea Diamantini <adjam7@gmail.com>
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

#include <KApplication>
#include <KDebug>
#include <KMimeType>
#include <KUrl>

#include <QFile>
#include <QFileInfo>
#include <q3textstream.h>
//Added by qt3to4:
#include <Q3CString>

#include <cstring>
#include <cstdio>

#include "gallerympform.h"
#include "gallerytalker.h"

namespace KIPIGalleryExportPlugin
{

GalleryMPForm::GalleryMPForm()
{
    m_boundary  = "----------";
// FIXME     m_boundary += KApplication::randomString( 42 + 13 ).ascii();

    if (GalleryTalker::isGallery2())
    {
      addPairRaw("g2_controller", "remote:GalleryRemote");
      QString auth_token = GalleryTalker::getAuthToken();
      if (!auth_token.isEmpty())
        addPairRaw("g2_authToken", auth_token);
    }
}

GalleryMPForm::~GalleryMPForm()
{
}

void GalleryMPForm::reset()
{
    m_buffer.resize(0);
}

void GalleryMPForm::finish()
{
    Q3CString str;
    str += "--";
    str += m_boundary;
    str += "--";
    str += "\r\n";

    Q3TextStream ts(m_buffer, QIODevice::Append|QIODevice::WriteOnly);
    ts.setEncoding(Q3TextStream::UnicodeUTF8);
    ts << str << '\0';
}

bool GalleryMPForm::addPair(const QString& name, const QString& value)
{
    if (GalleryTalker::isGallery2())
      return addPairRaw(QString("g2_form[%1]").arg(name), value);

    return addPairRaw(name, value);
}

bool GalleryMPForm::addPairRaw(const QString& name, const QString& value)
{
    Q3CString str;

    str += "--";
    str += m_boundary;
    str += "\r\n";
    str += "Content-Disposition: form-data; name=\"";
    str += name.ascii();
    str += "\"";
    str += "\r\n\r\n";
    str += value.ascii();
    str += "\r\n";

    //uint oldSize = m_buffer.size();
    //m_buffer.resize(oldSize + str.size());
    //memcpy(m_buffer.data() + oldSize, str.data(), str.size());

    Q3TextStream ts(m_buffer, QIODevice::Append|QIODevice::WriteOnly);
    ts.setEncoding(Q3TextStream::UnicodeUTF8);
    ts << str;

    return true;
}

bool GalleryMPForm::addFile(const QString& path, const QString& displayFilename)
{
    QString filename = "userfile_name";
    if (GalleryTalker::isGallery2())
        filename = "g2_userfile_name";

    if (!addPairRaw(filename, displayFilename))
    {
        return false;
    }

    KMimeType::Ptr ptr = KMimeType::findByUrl(path);
    QString mime = ptr->name();
    if (mime.isEmpty())
    {
        // if we ourselves can't determine the mime of the local file,
        // very unlikely the remote gallery will be able to identify it
        return false;
    }

    QFile imageFile(path);
    if ( !imageFile.open( QIODevice::ReadOnly ) )
        return false;
    QByteArray imageData = imageFile.readAll();
    imageFile.close();

    Q3CString str;

    str += "--";
    str += m_boundary;
    str += "\r\n";
    str += "Content-Disposition: form-data; name=\"";
    if (GalleryTalker::isGallery2())
      str += "g2_userfile";
    else
      str += "userfile";
    str += "\"; ";
    str += "filename=\"";
    str += QFile::encodeName(KUrl(path).fileName());
    str += "\"";
    str += "\r\n";
    str += "Content-Type: ";
    str +=  mime.ascii();
    str += "\r\n\r\n";

    Q3TextStream ts(m_buffer, QIODevice::Append|QIODevice::WriteOnly);
    ts.setEncoding(Q3TextStream::UnicodeUTF8);
    ts << str;

    int oldSize = m_buffer.size();
    m_buffer.resize(oldSize + imageData.size() + 2);
    memcpy(m_buffer.data()+oldSize, imageData.data(), imageData.size());
    m_buffer[m_buffer.size()-2] = '\r';
    m_buffer[m_buffer.size()-1] = '\n';

    return true;
}

QString GalleryMPForm::contentType() const
{
    return QString("Content-Type: multipart/form-data; boundary=" + m_boundary);
}

QString GalleryMPForm::boundary() const
{
    return m_boundary;
}

QByteArray GalleryMPForm::formData() const
{
    return m_buffer;
}

}
