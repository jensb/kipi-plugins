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

#include "imgurtalkerauth.h"

// Qt includes

#include <QImageReader>

// Local includes

#include "kipiplugins_debug.h"
#include "kpversion.h"
#include "plugin_imgur.h"

namespace KIPIImgurPlugin
{

class ImgurTalkerAuth::Private
{
public:

    Private()
    {
        userAgent                            = QLatin1String("KIPI-Plugins-ImgurExport") + QLatin1String("/") + kipipluginsVersion();
        const char _imgurApiConsumerKey[]    = _IMGUR_API_CONSUMER_KEY;
        consumerKey                          = QByteArray( _imgurApiConsumerKey );
        const char _imgurApiConsumerSecret[] = _IMGUR_API_CONSUMER_SECRET;
        consumerSecret                       = QByteArray( _imgurApiConsumerSecret );
        continueUpload                       = true;
        OAuthService                         = 0;
        OAuthRequest                         = 0;
    }

    QString         userAgent;

    KQOAuthManager* OAuthService;
    KQOAuthRequest* OAuthRequest;

    QByteArray      consumerKey;
    QByteArray      consumerSecret;
    QByteArray      oauthToken;
    QByteArray      oauthTokenSecret;

    bool            continueUpload;
};

ImgurTalkerAuth::ImgurTalkerAuth (Interface* const interface, QWidget* const parent)
    : ImgurTalker(interface, parent),
      d(new Private)
{
    d->OAuthRequest = new KQOAuthRequest();
    d->OAuthService = new KQOAuthManager(this);
}

ImgurTalkerAuth::~ImgurTalkerAuth()
{
    delete d;
}

const char* ImgurTalkerAuth::getAuthError(KQOAuthManager::KQOAuthError error)
{
    switch (error)
    {
        case KQOAuthManager::NetworkError:               // Network error: timeout, cannot connect.
            return I18N_NOOP("Network error: timeout, cannot connect.");
            break;
        case KQOAuthManager::RequestEndpointError:       // Request endpoint is not valid.
            return I18N_NOOP("Request endpoint is not valid.");
            break;
        case KQOAuthManager::RequestValidationError:     // Request is not valid: some parameter missing?
            return I18N_NOOP("Request is not valid: some parameter missing?");
            break;
        case KQOAuthManager::RequestUnauthorized:        // Authorization error: trying to access a resource without tokens.
            return I18N_NOOP("Authorization error: trying to access a resource without tokens.");
            break;
        case KQOAuthManager::RequestError:               // The given request to KQOAuthManager is invalid: NULL?,
            return I18N_NOOP("The given request is invalid.");
            break;
        case KQOAuthManager::ManagerError:                // Manager error, cannot use for sending requests.
            return I18N_NOOP("Manager error, cannot use for sending requests.");
            break;
        case KQOAuthManager::NoError:                    // No error
        default:
            return I18N_NOOP("No error");
            break;
    }
}

void ImgurTalkerAuth::cancel()
{
    if (d->OAuthRequest)
    {
        d->OAuthRequest->clearRequest();
    }

    emit signalBusy(false);
}

void ImgurTalkerAuth::imageUpload (const QUrl& filePath)
{
    qCDebug(KIPIPLUGINS_LOG) << "Authenticated" << (d->OAuthService->isAuthorized() ? "Yes" : "No");
    setCurrentUrl(filePath);

    qCDebug(KIPIPLUGINS_LOG) << "Authenticated upload of" << currentUrl();

    m_state = IE_ADDPHOTO;

    emit signalUploadStart(filePath);
    emit signalBusy(true);

    QByteArray contentType = QImageReader::imageFormat(filePath.toLocalFile());

    QFile imageFile(filePath.toLocalFile());

    if (!imageFile.open(QIODevice::ReadOnly))
    {
        return;
    }

    QByteArray imageData   = imageFile.readAll();

    d->OAuthRequest->setEnableDebugOutput (false);
    d->OAuthRequest->initRequest(KQOAuthRequest::AuthorizedRequest, QUrl(ImgurConnection::APIuploadURL()));
    d->OAuthRequest->setConsumerKey(d->consumerKey.data() );
    d->OAuthRequest->setConsumerSecretKey(d->consumerSecret.data() );
    d->OAuthRequest->setToken( d->oauthToken.data() );
    d->OAuthRequest->setTokenSecret( d->oauthTokenSecret.data() );

    KQOAuthParameters params;
    params.insert(QLatin1String("name"),           filePath.fileName());
    params.insert(QLatin1String("title"),          filePath.fileName());
    params.insert(QLatin1String("content-type"),   contentType.data());
    params.insert(QLatin1String("content-length"), QString::fromUtf8("Content-Length: %1").arg(imageData.toBase64().length()));
    params.insert(QLatin1String("UserAgent"),      d->userAgent);
    params.insert(QLatin1String("image"),          imageData.toBase64());

    d->OAuthRequest->setAdditionalParameters(params);
    d->OAuthService->executeRequest(d->OAuthRequest);
/*
   connect(d->OAuthService, SIGNAL(requestReady(QByteArray)),
           this, SLOT(slotRequestReady(QByteArray)));

   connect(d->OAuthService, SIGNAL(authorizedRequestDone()),
           this, SLOT(slotAuthorizedRequestDone()));
*/
}

void ImgurTalkerAuth::slotOAuthLogin()
{
    m_state = IE_LOGIN;

    d->OAuthRequest->initRequest(KQOAuthRequest::TemporaryCredentials, QUrl(ImgurConnection::OAuthTokenEndPoint()));
    d->OAuthRequest->setConsumerKey(d->consumerKey.data());
    d->OAuthRequest->setConsumerSecretKey(d->consumerSecret.data());

    d->OAuthRequest->setEnableDebugOutput(true);

    connect(d->OAuthService, SIGNAL(temporaryTokenReceived(QString,QString)),
            this, SLOT(slotTemporaryTokenReceived(QString,QString)));

    connect(d->OAuthService, SIGNAL(authorizationReceived(QString,QString)),
            this, SLOT(slotAuthorizationReceived(QString,QString)));

    connect(d->OAuthService, SIGNAL(accessTokenReceived(QString,QString)),
            this, SLOT(slotAccessTokenReceived(QString,QString)));

    connect(d->OAuthService, SIGNAL(requestReady(QByteArray)),
            this, SLOT(slotRequestReady(QByteArray)));

    d->OAuthService->setHandleUserAuthorization(true);
    d->OAuthService->executeRequest(d->OAuthRequest);

    emit signalBusy(true);
}

void ImgurTalkerAuth::slotTemporaryTokenReceived(const QString& token, const QString& tokenSecret)
{
    qCDebug(KIPIPLUGINS_LOG) << "Temporary token received: " << token << tokenSecret;

    if( d->OAuthService->lastError() == KQOAuthManager::NoError)
    {
        qCDebug(KIPIPLUGINS_LOG) << "Asking for user's permission to access protected resources. Opening URL: " << ImgurConnection::OAuthAuthorizationEndPoint();
        d->OAuthService->getUserAuthorization(QUrl(ImgurConnection::OAuthAuthorizationEndPoint()));
    }

    if (d->OAuthService->lastError() != KQOAuthManager::NoError)
    {
        //emit signalAuthenticated(false, getAuthError(d->OAuthService->lastError()));
        //emit signalBusy(false);
        qCDebug(KIPIPLUGINS_LOG) << "Error :" << getAuthError(d->OAuthService->lastError());
    }
}

void ImgurTalkerAuth::slotAuthorizationReceived(const QString& token, const QString& verifier)
{
    qCDebug(KIPIPLUGINS_LOG) << "User authorization received: " << token << verifier;

    if (d->OAuthService->lastError() == KQOAuthManager::NoError)
    {
        d->OAuthService->getUserAccessTokens(QUrl(ImgurConnection::OAuthAccessEndPoint()));
    }

    if( d->OAuthService->lastError() != KQOAuthManager::NoError)
    {
        emit signalAuthenticated(false, i18n(getAuthError(d->OAuthService->lastError())));
        emit signalBusy(false);

        d->OAuthRequest->clearRequest();
        qCDebug(KIPIPLUGINS_LOG) << "Auth error :" << getAuthError(d->OAuthService->lastError());
    }
}

void ImgurTalkerAuth::slotAccessTokenReceived(const QString& token, const QString& tokenSecret)
{
    qCDebug(KIPIPLUGINS_LOG) << "Access token received: " << token << tokenSecret;

    d->oauthToken       = token.toLatin1();
    d->oauthTokenSecret = tokenSecret.toLatin1();

    emit signalAuthenticated(true, i18n("OK"));
    emit signalBusy(false);

    qCDebug(KIPIPLUGINS_LOG) << "Access tokens now stored";
}

void ImgurTalkerAuth::slotAuthorizedRequestDone()
{
    qCDebug(KIPIPLUGINS_LOG) << "Request received from Imgur!";
}

void ImgurTalkerAuth::slotRequestReady(const QByteArray& response)
{
    //qCDebug(KIPIPLUGINS_LOG) << "Authorized: " << d->OAuthService->isAuthorized();
    //qCDebug(KIPIPLUGINS_LOG) << "Verified: " << d->OAuthService->isAuthorized();
    //qCDebug(KIPIPLUGINS_LOG) << "End point: " << d->OAuthRequest->requestEndpoint();
    //qCDebug(KIPIPLUGINS_LOG) << "Response from the service: " << response;

    if (d->OAuthService->isAuthorized() && d->OAuthService->isVerified() &&
        d->OAuthRequest->requestEndpoint() == QUrl(ImgurConnection::APIuploadURL())
       )
    {
        ImgurTalker::parseResponse (response);
    }

    return;
}

void ImgurTalkerAuth::slotContinueUpload(bool yes)
{
    d->continueUpload = yes;

    if (yes && !m_queue->isEmpty())
    {
        if (!d->OAuthService->isAuthorized())
        {
            // not authenticated, we upload anonymously
            ImgurTalker::imageUpload (m_queue->first());
        }
        else
        {
            // the top of the queue was already removed - first() is a new image
            imageUpload (m_queue->first());
        }
    }

    return;
}

} // namespace KIPIImgurPlugin
