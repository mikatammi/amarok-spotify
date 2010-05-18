/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "AvatarDownloader"

#include "AvatarDownloader.h"
#include "core/support/Debug.h"
#include "NetworkAccessManagerProxy.h"

#include <QNetworkRequest>
#include <QNetworkReply>

AvatarDownloader::AvatarDownloader()
{
    connect( The::networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
                                          SLOT(downloaded(QNetworkReply*)) );
}

AvatarDownloader::~AvatarDownloader()
{
}

void
AvatarDownloader::downloadAvatar( const QString& username, const KUrl& url )
{
    if( !url.isValid() )
        return;

    m_userAvatarUrls.insert( url, username );
    QNetworkRequest req( url );
    The::networkAccessManager()->get( req );
}

void
AvatarDownloader::downloaded( QNetworkReply *reply )
{
    const KUrl url = reply->request().url();
    if( !m_userAvatarUrls.contains( url ) )
        return;

    const QString username = m_userAvatarUrls.value( url );
    m_userAvatarUrls.remove( url );
    if( reply->error() == QNetworkReply::NoError )
    {
        QPixmap avatar;
        if( avatar.loadFromData( reply->readAll() ) )
        {
            reply->close();
            emit avatarDownloaded( username, avatar );
        }
    }
    else
        debug() << "Error:" << reply->errorString();
    reply->deleteLater();
}

#include "AvatarDownloader.moc"
