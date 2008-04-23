/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ColumnApplet.h"

#include "ContextScene.h"
#include "debug.h"
#include "Svg.h"
#include "SvgTinter.h"
#include "TheInstances.h"

#include "plasma/layouts/layoutanimator.h"
#include "plasma/phase.h"
#include "plasma/theme.h"

#include <KAuthorized>
#include <KMenu>
#include <KTemporaryFile>
#include <KSvgRenderer>
#include <KGlobalSettings>

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Job.h>

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QThread>
#include <QTimeLine>



namespace Context
{

void SvgRenderJob::run()
{
    KSvgRenderer renderer( m_file );
    m_image = QImage(  QApplication::desktop()->screenGeometry( QApplication::desktop()->primaryScreen() ).size(), QImage::Format_ARGB32 );
    QPainter painter ( &m_image );
    renderer.render( &painter );
}

ColumnApplet::ColumnApplet( QObject *parent, const QVariantList &args )
    : Context::Containment( parent, args )
    , m_actions( 0 )
    , m_defaultColumnSize( 350 )
    , m_appletBrowserHasBeenKicked ( false )
{
    DEBUG_BLOCK

    setContainmentType( CustomContainment );

    m_columns = new ContextLayout( this );
    m_columns->setColumnWidth( m_defaultColumnSize );
    m_columns->setSpacing( 3 );
    m_columns->setMargin( Plasma::LeftMargin, 0 );
    m_columns->setMargin( Plasma::TopMargin, 0 );
    m_columns->setMargin( Plasma::RightMargin, 0 );
    m_columns->setMargin( Plasma::BottomMargin, 0 );

    //HACK alert!

    //m_job = new SvgRenderJob( file.fileName() );
    //the background is not really important for the use of Amarok,
    //and running this in a thread makes startup quite a bit faster
    //consider only starting the thread if QThread::idealThreadCount > 1
    //if the background *has* to be there in time most of the time
    //connect( m_job, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( jobDone() ) );
    //ThreadWeaver::Weaver::instance()->enqueue( m_job );

    //m_background = new Svg( m_tintedSvg.fileName(), this );
    m_logo = new Svg( "widgets/amarok-logo", this );
    m_width = 300; // TODO hardcoding for now, do we want this configurable?
    m_aspectRatio = (qreal)m_logo->size().height() / (qreal)m_logo->size().width();
    m_logo->resize( (int)m_width, (int)( m_width * m_aspectRatio ) );

    connect( this, SIGNAL( appletAdded( Plasma::Applet* ) ), this, SLOT( addApplet( Plasma::Applet* ) ) );


    m_appletBrowserAction = new QAction(i18n("Add applet"), this);
    connect(m_appletBrowserAction, SIGNAL(triggered(bool)), this, SLOT(launchAppletBrowser()));
    // set up default context menu actions
    m_actions = new QList<QAction*>();
    m_actions->append( m_appletBrowserAction );

    m_appletBrowser = new Plasma::AppletBrowser( this );
    m_appletBrowser->setApplication( "amarok" );
    m_appletBrowser->hide();

    //connect ( Plasma::Theme::self(), SIGNAL( changed() ), this, SLOT( paletteChange() ) );
    connect( KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged() ), this, SLOT( paletteChange() ) );
}

void ColumnApplet::saveToConfig( KConfig& conf )
{
//     debug() << "number of m_columns:" << m_columns->count();
    for( int i = 0; i < m_columns->count(); i++ )
    {
        Applet* applet = dynamic_cast< Applet* >( m_columns->itemAt( i ) );
//         debug() << "trying to save an applet";
        if( applet != 0 )
        {
            KConfigGroup cg( &conf, QString::number( applet->id() ) );
//             debug() << "saving applet" << applet->name();
            cg.writeEntry( "plugin", applet->pluginName() );
        }
    }
    conf.sync();
}

void ColumnApplet::loadConfig( KConfig& conf )
{
    foreach( const QString& group, conf.groupList() )
    {
        KConfigGroup cg( &conf, group );
        QString plugin = cg.readEntry( "plugin", QString() );
//         debug() << "loading applet:" << plugin
//             << QStringList() << group.toUInt();
        if( plugin != QString() )
            Plasma::Containment::addApplet( plugin );
    }
    recalculate();
}

QSizeF ColumnApplet::sizeHint() const
{
    return m_geometry.size();
}

QRectF ColumnApplet::boundingRect() const
{
    return m_geometry;
}
// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnApplet::updateSize() // SLOT
{
    m_geometry = scene()->sceneRect();
    m_columns->setGeometry( scene()->sceneRect() );
    setGeometry( scene()->sceneRect() );
}

void ColumnApplet::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& rect)
{
    Q_UNUSED( option );
    painter->save();
    //m_background->paint( painter, rect );
    /*if( !m_masterImage.isNull() )
    {
    QImage scaled = m_masterImage.scaled( rect.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation );
    painter->drawImage( rect, scaled );
    }
    else
    {
        painter->fillRect( rect, QApplication::palette().window() );
    }*/

    //m_renderer->render( painter, rect );

    painter->drawPixmap(0, 0, The::svgHandler()->renderSvg( "desktoptheme/default/widgets/amarok-wallpaper.svg", "context_background", rect.width(), rect.height() ) );

    painter->restore();

    QSize size = m_logo->size();
    QSize pos = rect.size() - size;
    qreal newHeight  = m_aspectRatio * m_width;
    m_logo->resize( QSize( (int)m_width, (int)newHeight ) );
    painter->save();
    m_logo->paint( painter, QRectF( pos.width() - 10.0, pos.height() - 5.0, size.width(), size.height() ) );
    painter->restore();
}

void ColumnApplet::appletRemoved( QObject* object ) // SLOT
{
    Q_UNUSED( object )
    recalculate();
}

/*
// parts of this code come from Qt 4.3, src/gui/graphicsview/qgraphicsitem.cpp
void ColumnApplet::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
    DEBUG_BLOCK
    if (event->button() == Qt::LeftButton && (flags() & ItemIsSelectable))
    {
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        if (!multiSelect) {
            if (!isSelected()) {
                if (scene())
                    scene()->clearSelection();
                setSelected(true);
            }
        }
    } else if (!(flags() & ItemIsMovable)) {
        event->ignore();
    }
}

// parts of this code come from Qt 4.3, src/gui/graphicsview/qgraphicsitem.cpp
void ColumnApplet::mouseMoveEvent( QGraphicsSceneMouseEvent * event )
{
//     debug() << "layout manager got a mouse event";
    if ( ( event->buttons() & Qt::LeftButton ) )
    {

        // Find the active view.
        QGraphicsView *view = 0;
        if (event->widget())
            view = qobject_cast<QGraphicsView *>(event->widget()->parentWidget());

        if ((flags() & ItemIsMovable) && (!parentItem() || !parentItem()->isSelected())) {
            QPointF diff;
            if (flags() & ItemIgnoresTransformations) {
                    // Root items that ignore transformations need to
                    // calculate their diff by mapping viewport coordinates to
                    // parent coordinates. Items whose ancestors ignore
                    // transformations can ignore this problem; their events
                    // are already mapped correctly.
                QTransform viewToParentTransform = (sceneTransform() * view->viewportTransform()).inverted();

                QTransform myTransform = transform().translate(pos().x(), pos().y());
                viewToParentTransform = myTransform * viewToParentTransform;

                diff = viewToParentTransform.map(QPointF(view->mapFromGlobal(event->screenPos())))
                    - viewToParentTransform.map(QPointF(view->mapFromGlobal(event->lastScreenPos())));
            } else
            {
                diff = mapToParent(event->pos()) - mapToParent(event->lastPos());
            }

            moveBy(diff.x(), diff.y());

            if (flags() & ItemIsSelectable)
                setSelected(true);
        }
    }
}

*/


Plasma::Applet* ColumnApplet::addApplet( Applet* applet )
{
//     debug() << "m_columns:" << m_columns;
     m_columns->addItem( applet );

    recalculate();
    return applet;
}

void ColumnApplet::recalculate()
{
//     debug() << "got child item that wants a recalculation";
    m_columns->invalidate();
}

QList<QAction*> ColumnApplet::contextActions()
{
    return *m_actions;
}

void ColumnApplet::launchAppletBrowser() // SLOT
{
    m_appletBrowser->show();
    if ( !m_appletBrowserHasBeenKicked ) {
        m_appletBrowser->resize( m_appletBrowser->size() + QSize( 1 , 0 ) );
        m_appletBrowserHasBeenKicked = true;
    }
}
/*
bool ColumnApplet::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    Applet *applet = qgraphicsitem_cast<Applet*>(watched);
    //QEvent::GraphicsSceneHoverEnter

    // Otherwise we're watching something we shouldn't be...
    Q_ASSERT(applet!=0);

    return false;
}*/

void ColumnApplet::destroyApplet()
{
    QAction *action = qobject_cast<QAction*>(sender());

    if (!action) {
        return;
    }

    Applet *applet = qobject_cast<Applet*>(action->data().value<QObject*>());
    Plasma::Phase::self()->animateItem(applet, Plasma::Phase::Disappear);
}

void ColumnApplet::appletDisappearComplete(QGraphicsItem *item, Plasma::Phase::Animation anim)
{
    if (anim == Plasma::Phase::Disappear) {
        if (item->parentItem() == this) {
            Applet *applet = qgraphicsitem_cast<Applet*>(item);

            if (applet) {
                applet->destroy();
            }
        }
    }
}

void ColumnApplet::jobDone()
{
    m_masterImage = m_job->m_image;
    m_job->deleteLater();
    m_job = 0;
    //we should request a redraw of  applet now, but I have now idea how to do that
}


void ColumnApplet::paletteChange()
{
    The::svgHandler()->reTint( "desktoptheme/default/widgets/amarok-wallpaper.svg" );
    update( boundingRect() );
}


} // Context namespace



#include "ColumnApplet.moc"
