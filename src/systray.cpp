//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "amarok.h"
#include "enginecontroller.h"
#include "systray.h"

#include <qevent.h>
#include <qimage.h>
#include <qtimer.h>
#include <kaction.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kiconeffect.h>
#include <kstandarddirs.h>

namespace amaroK
{
    static QPixmap *
    loadOverlay( const char * iconName )
    {
        QPixmap icon( locate( "data", QString( "amarok/images/b_%1.png" ).arg( iconName ) ), "PNG" );
        return new QPixmap( icon.convertToImage().smoothScale( 12, 12 ) );
    }
}


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget )
  : KSystemTray( playerWidget ),
  trackLength( 0 ), trackPercent( -1 ), drawnPercent( -1 ),
  baseIcon( 0 ), grayedIcon( 0 ), alternateIcon( 0 ),
  blinkTimerID( 0 )
{
    KActionCollection* const ac = amaroK::actionCollection();

    setAcceptDrops( true );

    ac->action( "prev"  )->plug( contextMenu() );
    ac->action( "play"  )->plug( contextMenu() );
    ac->action( "pause" )->plug( contextMenu() );
    ac->action( "stop"  )->plug( contextMenu() );
    ac->action( "next"  )->plug( contextMenu() );

    QPopupMenu &p = *contextMenu();
    QStringList shortcuts; shortcuts << "" << "Z" << "X" << "C" << "V" << "B";
    const QString body = "|&%1| %2";

    for( uint index = 1; index < 6; ++index )
    {
        int id = p.idAt( index );
        p.changeItem( id, body.arg( *shortcuts.at( index ), p.text( id ) ) );
    }

    //seems to be necessary
    KAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL( activated() ), kapp, SLOT( quit() ) );

    playOverlay =  amaroK::loadOverlay( "play" );
    pauseOverlay = amaroK::loadOverlay( "pause" );
    stopOverlay =  amaroK::loadOverlay( "stop" );
    overlay = 0;
    overlayVisible = false;
    paintIcon();

    // attach to get notified about engine events
    EngineController::instance()->attach( this );
}

amaroK::TrayIcon::~TrayIcon( )
{
    EngineController::instance()->detach( this );
    killTimer( blinkTimerID );
    delete baseIcon;
    delete grayedIcon;
    delete alternateIcon;
    delete playOverlay;
    delete pauseOverlay;
    delete stopOverlay;
}

bool
amaroK::TrayIcon::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::Drop:
    case QEvent::Wheel:
    case QEvent::DragEnter:
        return amaroK::genericEventHandler( this, e );

    case QEvent::Timer:
        if( static_cast<QTimerEvent*>(e)->timerId() != blinkTimerID )
            return KSystemTray::event( e );

        // if we're playing, blink icon
        if ( overlay == playOverlay )
        {
            overlayVisible = !overlayVisible;
            paintIcon( trackPercent, true );
        }

        // if we're stopped return to default state after the first tick
        else if ( overlay == stopOverlay )
        {
            killTimer( blinkTimerID );
            blinkTimerID = 0;
            overlayVisible = false;
            paintIcon( 100, true );
        }

        return true;

    case QEvent::MouseButtonPress:
        if( static_cast<QMouseEvent*>(e)->button() == Qt::MidButton )
        {
            EngineController::instance()->playPause();

            return true;
        }

        //else FALL THROUGH

    default:
        return KSystemTray::event( e );
    }
}

void
amaroK::TrayIcon::engineStateChanged( Engine::State state )
{
    // stop timer
    if ( blinkTimerID )
    {
        killTimer( blinkTimerID );
        blinkTimerID = 0;
    }
    // draw overlay
    overlayVisible = true;

    // draw the right overlay for each state
    switch( state )
    {
    case Engine::Paused:
        overlay = pauseOverlay;
        paintIcon( trackPercent, true );
        break;

    case Engine::Playing:
        overlay = playOverlay;
        blinkTimerID = startTimer( 1500 );  // start 'blink' timer
        break;

    default: // idle/stopped case
        overlay = stopOverlay;
        blinkTimerID = startTimer( 2500 );  // start 'hide' timer
        paintIcon( 100, true );
    }
}

void
amaroK::TrayIcon::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    trackLength = bundle.length() * 1000;
}

void
amaroK::TrayIcon::engineTrackPositionChanged( long position )
{
    trackPercent = trackLength ? (100 * position) / trackLength : 100;
    paintIcon( trackPercent );
}

void
amaroK::TrayIcon::paletteChange( const QPalette & op )
{
    if ( palette().active().highlight() == op.active().highlight() || !alternateIcon )
        return;

    delete alternateIcon;
    alternateIcon = 0;
    paintIcon( trackPercent, true );
}

void
amaroK::TrayIcon::paintIcon( int percent, bool force )
{
    // skip redrawing the same pixmap
    if ( percent == drawnPercent && !force )
        return;
    drawnPercent = percent;

    // load the base trayIcon
    if ( !baseIcon )
        baseIcon = new QPixmap( KSystemTray::loadIcon("amarok") );
    if ( percent > 99 )
        return blendOverlay( baseIcon );
        
    // make up the grayed icon
    if ( !grayedIcon )
    {
        QImage tmpTrayIcon = baseIcon->convertToImage();
        KIconEffect::semiTransparent( tmpTrayIcon );
        grayedIcon = new QPixmap( tmpTrayIcon );
    }

    if ( percent < 1 )
        return blendOverlay( grayedIcon );

    // make up the alternate icon (use hilight color but more saturated)
    if ( !alternateIcon )
    {
        QImage tmpTrayIcon = baseIcon->convertToImage();
        // eros: this looks cool with dark red blue or green but sucks with
        // other colors (such as kde default's pale pink..). maybe the effect
        // or the blended color has to be changed..
        QColor saturatedColor = palette().active().highlight();
        int hue, sat, value;
        saturatedColor.getHsv( &hue, &sat, &value );
        saturatedColor.setHsv( hue, (sat + 510) / 3, value );
        KIconEffect::colorize( tmpTrayIcon, saturatedColor, 0.9 );
        alternateIcon = new QPixmap( tmpTrayIcon );
    }

    // mix [ grayed <-> colored ] icons
    QPixmap tmpTrayPixmap( *grayedIcon );
    int height = grayedIcon->height(),
        sourceH = 1 + (height * percent) / 100, // 1 .. height
        sourceY = height - sourceH;             // height-1 .. 0
    copyBlt( &tmpTrayPixmap, 0,sourceY, alternateIcon, 0,sourceY,
            grayedIcon->width(), sourceH );
    blendOverlay( &tmpTrayPixmap );
}

void
amaroK::TrayIcon::blendOverlay( QPixmap * sourcePixmap )
{
    if ( !overlayVisible || !overlay || overlay->isNull() )
        return setPixmap( *sourcePixmap ); // @since 3.2
    
    // here comes the tricky part.. no kdefx functions are helping here.. :-(
    // we have to blend pixmaps with different sizes (blending will be done in
    // the bottom-left corner of source pixmap with a smaller overlay pixmap)
    int opW = overlay->width(),
        opH = overlay->height(),
        opX = 0,
        opY = sourcePixmap->height() - opH;

    // get the rectangle where blending will take place 
    QPixmap sourceCropped( opW, opH, sourcePixmap->depth() );
    copyBlt( &sourceCropped, 0,0, sourcePixmap, opX,opY, opW,opH );

    // blend the overlay image over the cropped rectangle
    QImage blendedImage = sourceCropped.convertToImage();
    QImage overlayImage = overlay->convertToImage();
    KIconEffect::overlay( blendedImage, overlayImage );
    sourceCropped.convertFromImage( blendedImage );
    
    // put back the blended rectangle to the original image
    QPixmap sourcePixmapCopy = *sourcePixmap;
    copyBlt( &sourcePixmapCopy, opX,opY, &sourceCropped, 0,0, opW,opH );

    setPixmap( sourcePixmapCopy ); // @since 3.2
}
