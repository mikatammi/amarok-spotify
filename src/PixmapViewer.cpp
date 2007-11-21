/***************************************************************************
 *   Copyright (C) 2005 Eyal Lotem <eyal.lotem@gmail.com>                  *
 *   Copyright (C) 2005 Alexandre Oliveira <aleprj@gmail.com>              *
 *   Copyright (C) 2007 Seb Ruiz <ruiz@kde.org>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/
#include "PixmapViewer.h"
#include "debug.h"

#include <KApplication>

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QLabel>
#include <QPixmap>


PixmapViewer::PixmapViewer( QWidget *widget, const QPixmap &pixmap )
    : QScrollArea( widget )
    , m_isDragging( false )
    , m_pixmap( pixmap )
{
    resize( m_pixmap.width(), m_pixmap.height() );

    QLabel *imageLabel = new QLabel();
    imageLabel->setBackgroundRole( QPalette::Base );
    imageLabel->setScaledContents( true );
    imageLabel->setPixmap( pixmap );
    
    setBackgroundRole( QPalette::Dark );
    setWidget( imageLabel );
}

void PixmapViewer::mousePressEvent(QMouseEvent *event)
{
    debug() << "mouse pressed!";
    if( Qt::LeftButton == event->button() )
    {
        m_currentPos = event->globalPos();
        m_isDragging = true;
    }
}

void PixmapViewer::mouseReleaseEvent(QMouseEvent *event)
{
    debug() << "mouse released!";
    if( Qt::LeftButton == event->button() )
    {
        m_currentPos = event->globalPos();
        m_isDragging = false;
    }
}

void PixmapViewer::mouseMoveEvent(QMouseEvent *event)
{
    if( m_isDragging )
    {
        debug() << "dragging";
        QPoint delta = m_currentPos - event->globalPos();
        scroll( delta.x(), delta.y() );
        m_currentPos = event->globalPos();
    }
}

QSize PixmapViewer::maximalSize()
{
    debug() << "Desktop  size: " << KApplication::desktop()->size();
    debug() << "Pixmap   size: " << m_pixmap.size();
    debug() << "Widget   size: " << size();
    debug() << "Viewport size: " << viewport()->size();
    //return m_pixmap.size().boundedTo( KApplication::desktop()->size() ) + viewport()->size();
    return size().boundedTo( KApplication::desktop()->size() );
}

#include "PixmapViewer.moc"
