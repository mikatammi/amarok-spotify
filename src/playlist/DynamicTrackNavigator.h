/***************************************************************************
 * copyright            : (C) 2008 Daniel Jones <danielcjones@gmail.com> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#ifndef DYNAMICTRACKNAVIGATOR_H
#define DYNAMICTRACKNAVIGATOR_H

#include "DynamicPlaylist.h"
#include "PlaylistModel.h"
#include "PlaylistRowList.h"
#include "TrackNavigator.h"

class DynamicPlaylist;

namespace Playlist {

class Model;

    class DynamicTrackNavigator : public QObject, public TrackNavigator
    {
        Q_OBJECT

        public:
            DynamicTrackNavigator( Model* m, Meta::DynamicPlaylistPtr p ) ;
            int nextRow();
            
            void appendUpcoming();

        private slots:
            void activeRowChanged();
            void activeRowExplicitlyChanged();

        private:
            void setAsUpcoming( int row );
            void setAsPlayed( int row );

            void removePlayed();

            RowList m_upcomingRows;
            RowList m_playedRows;

            Meta::DynamicPlaylistPtr m_playlist;
    };
}


#endif

