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

#ifndef IMGURTALKERAUTH_H
#define IMGURTALKERAUTH_H

// Qt includes

#include <QWidget>
#include <QObject>
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QUrl>

// QtKOAuth includes

// Pragma directives to reduce warnings from QtKOAuth header files.
#if not defined(__APPLE__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
#endif

#include <QtKOAuth>

// Restore warnings
#if not defined(__APPLE__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__APPLE__) && defined(__clang__)
#pragma clang diagnostic pop
#endif

// Libkipi includes

#include <KIPI/Interface>

// Local includes

#include "imgurtalker_common.h"
#include "imgurtalker.h"

using namespace KIPI;

namespace KIPIImgurPlugin
{

class ImgurTalkerAuth : public ImgurTalker
{
    Q_OBJECT

public:

    ImgurTalkerAuth(Interface* const iface, QWidget* const parent = 0);
    ~ImgurTalkerAuth();

    void imageUpload(const QUrl& filePath);
    void cancel();

Q_SIGNALS:

    void signalAuthenticated(bool authenticated, const QString& message = QString());

private:

    bool               parseLoginResponse(const QByteArray& data);
    static const char* getAuthError(KQOAuthManager::KQOAuthError error);

private Q_SLOTS:

    void slotOAuthLogin();
    void slotTemporaryTokenReceived(const QString& temporaryToken, const QString& temporaryTokenSecret);
    void slotAuthorizationReceived(const QString& token, const QString& verifier);
    void slotAccessTokenReceived(const QString& token, const QString& tokenSecret);
    void slotAuthorizedRequestDone();
    void slotRequestReady(const QByteArray&);
    void slotContinueUpload(bool yes);

private:

    class Private;
    Private* const d;
};

} // namespace KIPIImgurPlugin

#endif // IMGURTALKERAUTH_H
