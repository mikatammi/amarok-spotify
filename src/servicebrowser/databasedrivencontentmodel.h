/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/


#ifndef AMAROKJAMENDOMODEL_H
#define AMAROKJAMENDOMODEL_H

#include "infoparserbase.h"
#include "simpleservicetypes.h"

#include "databasehandlerbase.h"

#include <servicemodelbase.h>
#include <QMap>
#include <QModelIndex>
#include <QVariant>
#include <kicon.h>

#include "databasedrivencontentitem.h"


class DatabaseDrivenContentModel : public ServiceModelBase
{
    Q_OBJECT

private:

   /* mutable QMap<int, DatabaseDrivenDatabaseEntry *> m_artistEntriesMap;
    mutable QMap<int, DatabaseDrivenDatabaseEntry *> m_albumEntriesMap;
    mutable QMap<int, DatabaseDrivenDatabaseEntry *> m_trackEntriesMap;
*/
    private:

    DatabaseDrivenContentItem *m_rootContentItem;
    QString m_genre;
    InfoParserBase * m_infoParser;

    KIcon m_artistIcon;
    KIcon m_albumIcon;
    KIcon m_trackIcon;

    DatabaseHandlerBase * m_dbHandler;
    

private slots:

    void infoParsed( const QString &infoHtml );

public:
    
    explicit DatabaseDrivenContentModel(QObject *parent = 0, const QString &genre = "All");
    
    ~DatabaseDrivenContentModel();

    QVariant data(const QModelIndex &index, int role) const;
    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    
    QModelIndex parent(const QModelIndex &index) const;
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    bool canFetchMore( const QModelIndex & parent ) const;

    void fetchMore ( const QModelIndex & parent );

    bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;

    void requestHtmlInfo ( const QModelIndex & index ) const;

    void setGenre( const QString &genre );

    void setDbHandler( DatabaseHandlerBase * dhHandler );

    void setInfoParser( InfoParserBase * infoParser );

//signals:
    //void infoChanged ( QString infoHtml );

 };

 #endif
