/***************************************************************************
 * copyright            : (C) 2006 Andy Kelk <andy@mopoke.co.uk>           *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /**
  *  Based on njb mediadevice with some code hints from the libmtp
  *  example tools
  */

 /**
  *  MTP media device
  *  @author Andy Kelk <andy@mopoke.co.uk>
  *  @see http://libmtp.sourceforge.net/
  */

#define DEBUG_PREFIX "MtpMediaDevice"

#include <config.h>
#include "mtpmediadevice.h"

AMAROK_EXPORT_PLUGIN( MtpMediaDevice )

// Amarok
#include <debug.h>
#include <metabundle.h>
#include <statusbar/statusbar.h>
#include <statusbar/popupMessage.h>
#include <collectionbrowser.h>

// KDE
#include <kapplication.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>
#include <ktoolbarbutton.h>
#include <ktempdir.h>

// Qt
#include <qdir.h>
#include <qlistview.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qregexp.h>
#include <qmap.h>


/**
 * MtpMediaDevice Class
 */

MtpMediaDevice::MtpMediaDevice() : MediaDevice()
{
    m_name = "MTP Device";
    m_device = 0;
    m_folders = 0;
    m_playlistItem = 0;
    setDisconnected();
    m_hasMountPoint = false;
    m_syncStats = false;
    m_transcode = false;
    m_transcodeAlways = false;
    m_transcodeRemove = false;
    m_configure = false;
    m_customButton = true;
    m_transfer = true;

    KToolBarButton *customButton = MediaBrowser::instance()->getToolBar()->getButton( MediaBrowser::CUSTOM );
    customButton->setText( i18n("Special device functions") );
    QToolTip::remove( customButton );
    QToolTip::add( customButton, i18n( "Special functions of your device" ) );

    mtpFileTypes[LIBMTP_FILETYPE_WAV] = "wav";
    mtpFileTypes[LIBMTP_FILETYPE_MP3] = "mp3";
    mtpFileTypes[LIBMTP_FILETYPE_WMA] = "wma";
    mtpFileTypes[LIBMTP_FILETYPE_OGG] = "ogg";
    mtpFileTypes[LIBMTP_FILETYPE_AUDIBLE] = "aa"; // audible
    mtpFileTypes[LIBMTP_FILETYPE_MP4] = "mp4";
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_AUDIO] = "undef-audio";
    mtpFileTypes[LIBMTP_FILETYPE_WMV] = "wmv";
    mtpFileTypes[LIBMTP_FILETYPE_AVI] = "avi";
    mtpFileTypes[LIBMTP_FILETYPE_MPEG] = "mpg";
    mtpFileTypes[LIBMTP_FILETYPE_ASF] = "asf";
    mtpFileTypes[LIBMTP_FILETYPE_QT] = "mov";
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_VIDEO] = "undef-video";
    mtpFileTypes[LIBMTP_FILETYPE_JPEG] = "jpg";
    mtpFileTypes[LIBMTP_FILETYPE_JFIF] = "jpg";
    mtpFileTypes[LIBMTP_FILETYPE_TIFF] = "tiff";
    mtpFileTypes[LIBMTP_FILETYPE_BMP] = "bmp";
    mtpFileTypes[LIBMTP_FILETYPE_GIF] = "gif";
    mtpFileTypes[LIBMTP_FILETYPE_PICT] = "pict";
    mtpFileTypes[LIBMTP_FILETYPE_PNG] = "png";
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR1] = "vcs"; // vcal1
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR2] = "vcs"; // vcal2
    mtpFileTypes[LIBMTP_FILETYPE_VCARD2] = "vcf"; // vcard2
    mtpFileTypes[LIBMTP_FILETYPE_VCARD3] = "vcf"; // vcard3
    mtpFileTypes[LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT] = "wim"; // windows image format
    mtpFileTypes[LIBMTP_FILETYPE_WINEXEC] = "exe";
    mtpFileTypes[LIBMTP_FILETYPE_TEXT] = "txt";
    mtpFileTypes[LIBMTP_FILETYPE_HTML] = "html";
    mtpFileTypes[LIBMTP_FILETYPE_UNKNOWN] = "unknown";
}

void
MtpMediaDevice::init( MediaBrowser *parent )
{
    MediaDevice::init( parent );
}

bool
MtpMediaDevice::isConnected()
{
    return !( m_device == 0 );
}

/**
 * File types that we support
 */
QStringList
MtpMediaDevice::supportedFiletypes()
{
    return m_supportedFiles;
}


int
MtpMediaDevice::progressCallback( uint64_t const sent, uint64_t const total, void const * const data )
{
    Q_UNUSED( sent );
    Q_UNUSED( total );

    kapp->processEvents( 100 );

    MtpMediaDevice *dev = (MtpMediaDevice*)(data);

    if( dev->isCanceled() )
    {
        debug() << "Canceling transfer operation" << endl;
        dev->setCanceled( true );
        return 1;
    }

    return 0;
}

/**
 * Copy a track to the device
 */
MediaItem
*MtpMediaDevice::copyTrackToDevice( const MetaBundle &bundle )
{
    DEBUG_BLOCK

    QString genericError = i18n( "Could not send track" );

    if( m_fileNameToItem[ bundle.filename() ] != 0 )
    {
        // track already exists. don't do anything (for now).
        debug() << "Track already exists on device." << endl;
        Amarok::StatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "Track already exists on device" ),
            KDE::StatusBar::Error
        );
        return 0;
    }

    LIBMTP_track_t *trackmeta = LIBMTP_new_track_t();
    trackmeta->item_id = 0;
    debug() << "filetype : " << bundle.fileType() << endl;
    if( bundle.fileType() == MetaBundle::mp3 )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_MP3;
    }
    else if( bundle.fileType() == MetaBundle::ogg )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_OGG;
    }
    else if( bundle.fileType() == MetaBundle::wma )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_WMA;
    }
    else if( bundle.fileType() == MetaBundle::mp4 )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_MP4;
    }
    else
    {
        // Couldn't recognise an Amarok filetype.
        // fallback to checking the extension (e.g. .wma, .ogg, etc)
        debug() << "No filetype found by Amarok filetype" << endl;

        const QString extension = bundle.url().path().section( ".", -1 ).lower();

        int libmtp_type = m_supportedFiles.findIndex( extension );
        if( libmtp_type >= 0 )
        {
            int keyIndex = mtpFileTypes.values().findIndex( extension );
            libmtp_type = mtpFileTypes.keys()[keyIndex];
            trackmeta->filetype = (LIBMTP_filetype_t) libmtp_type;
            debug() << "set filetype to " << libmtp_type << " based on extension of ." << extension << endl;
        }
        else
        {
            debug() << "We don't support the extension ." << extension << endl;
            Amarok::StatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Cannot determine a valid file type" ),
                KDE::StatusBar::Error
            );
            return 0;
        }
    }

    if( bundle.title().isEmpty() )
    {
        trackmeta->title = qstrdup( "<Unknown title>" );
    }
    else
    {
        trackmeta->title = qstrdup( bundle.title().utf8() );
    }

    if( bundle.album().isEmpty() )
    {
        trackmeta->album = qstrdup( "<Unknown album>" );
    }
    else
    {
        trackmeta->album = qstrdup( bundle.album().string().utf8() );
    }

    if( bundle.artist().isEmpty() )
    {
        trackmeta->artist = qstrdup( "<Unknown artist>" );
    }
    else
    {
        trackmeta->artist = qstrdup( bundle.artist().string().utf8() );
    }

    if( bundle.genre().isEmpty() )
    {
        trackmeta->genre = qstrdup( "<Unknown genre>" );
    }
    else
    {
        trackmeta->genre = qstrdup( bundle.genre().string().utf8() );
    }

    if( bundle.year() > 0 )
    {
        QString date;
        QTextOStream( &date ) << bundle.year() << "0101T0000.0";
        trackmeta->date = qstrdup( date.utf8() );
    }
    else
    {
        trackmeta->date = qstrdup( "00010101T0000.0" );
    }

    if( bundle.track() > 0 )
    {
        trackmeta->tracknumber = bundle.track();
    }
    if( bundle.length() > 0 )
    {
        // Multiply by 1000 since this is in milliseconds
        trackmeta->duration = bundle.length() * 1000;
    }
    if( !bundle.filename().isEmpty() )
    {
        trackmeta->filename = qstrdup( bundle.filename().utf8() );
    }
    trackmeta->filesize = bundle.filesize();

    // Decide which folder to send it to:
    // If the device gave us a parent_folder setting, we use it
    uint32_t parent_id = 0;
    if( m_default_parent_folder )
    {
        parent_id = m_default_parent_folder;
        debug() << "Using default music folder : " << parent_id << endl;
    }
    // Otherwise look for a folder called "Music"
    else if( m_folders != 0 )
    {
        parent_id = folderNameToID( "Music", m_folders );
        if( !parent_id )
        {
            debug() << "Parent folder could not be found. Going to use top level." << endl;
        }
    }
    // Give up and don't set a parent folder, let the device deal with it
    else
    {
        debug() << "No folders found. Going to use top level." << endl;
    }

    // try and create the requested folder structure
    if( !m_folderStructure.isEmpty() )
    {
        parent_id = checkFolderStructure( parent_id, trackmeta, bundle );
        if( parent_id == 0 )
        {
            debug() << "Couldn't create new parent (" << m_folderStructure << ")" << endl;
            Amarok::StatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Cannot create parent folder. Check your structure." ),
                KDE::StatusBar::Error
            );
            return 0;
        }
    }
    debug() << "Parent id : " << parent_id << endl;

    m_critical_mutex.lock();
    debug() << "Sending track... " << bundle.url().path().utf8() << endl;
    int ret = LIBMTP_Send_Track_From_File(
        m_device, bundle.url().path().utf8(), trackmeta,
        progressCallback, this, parent_id  // callbacks only in libmtp >= 0.0.15
    );
    m_critical_mutex.unlock();

    if( ret < 0 )
    {
        debug() << "Could not write file " << ret << endl;
        Amarok::StatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "File write failed" ),
            KDE::StatusBar::Error
        );
        return 0;
    }

    MetaBundle temp( bundle );
    MtpTrack *taggedTrack = new MtpTrack( trackmeta );
    taggedTrack->setBundle( temp );

    LIBMTP_destroy_track_t( trackmeta );

    kapp->processEvents( 100 );

    // cache the track
    return addTrackToView( taggedTrack );
}

/**
 * Check (and create) the folder structure to put a
 * track into. Return the (possibly new) parent folder ID
 */
uint32_t
MtpMediaDevice::checkFolderStructure( uint32_t parent_id, const LIBMTP_track_t *trackmeta, const MetaBundle &bundle )
{
    m_critical_mutex.lock();
    QStringList folders = QStringList::split( "/", m_folderStructure ); // use slash as a dir separator
    QString completePath;
    for( QStringList::Iterator it = folders.begin(); it != folders.end(); ++it )
    {
        if( (*it).isEmpty() )
            continue;
        // substitute %a , %b , %g
        QString artist = trackmeta->artist;
        if( bundle.compilation() == MetaBundle::CompilationYes )
            artist = i18n( "Various Artists" );
        (*it).replace( QRegExp( "%a" ), artist )
            .replace( QRegExp( "%b" ), trackmeta->album )
            .replace( QRegExp( "%g" ), trackmeta->genre );
        // check if it exists
        uint32_t check_folder = subfolderNameToID( (*it).utf8(), m_folders, parent_id );
        // create if not exists
        if( check_folder == 0 )
        {
            check_folder = createFolder( (*it).utf8() , parent_id );
            if( check_folder == 0 )
                return 0;
        }
        completePath += (*it).utf8() + '/';
        // set new parent
        parent_id = check_folder;
    }
    m_critical_mutex.unlock();
    debug() << "Folder path : " << completePath << endl;
    // return parent
    return parent_id;
}

/**
 * Create a new mtp folder
 */
uint32_t
MtpMediaDevice::createFolder( const char *name, uint32_t parent_id )
{
    debug() << "Creating new folder '" << name << "' as a child of "<< parent_id << endl;
    char *name_copy = qstrdup( name );
    uint32_t new_folder_id = LIBMTP_Create_Folder( m_device, name_copy, parent_id );
    delete(name_copy);
    debug() << "New folder ID: " << new_folder_id << endl;
    if( new_folder_id == 0 )
    {
        debug() << "Attempt to create folder '" << name << "' failed." << endl;
        return 0;
    }
    updateFolders();

    return new_folder_id;
}

/**
 * Recursively search the folder list for a matching one under the specified
 * parent ID and return the child's ID
 */
uint32_t
MtpMediaDevice::subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id )
{
    uint32_t i;

    if( folderlist == 0 )
        return 0;

    if( !strcasecmp( name, folderlist->name ) && folderlist->parent_id == parent_id )
        return folderlist->folder_id;

    if( ( i = ( subfolderNameToID( name, folderlist->child, parent_id ) ) ) )
        return i;
    if( ( i = ( subfolderNameToID( name, folderlist->sibling, parent_id ) ) ) )
        return i;

    return 0;
}

/**
 * Recursively search the folder list for a matching one
 * and return its ID
 */
uint32_t
MtpMediaDevice::folderNameToID( char *name, LIBMTP_folder_t *folderlist )
{
    uint32_t i;

    if( folderlist == 0 )
        return 0;

    if( !strcasecmp( name, folderlist->name ) )
        return folderlist->folder_id;

    if( ( i = ( folderNameToID( name, folderlist->child ) ) ) )
        return i;
    if( ( i = ( folderNameToID( name, folderlist->sibling ) ) ) )
        return i;

    return 0;
}

/**
 * Get a list of selected items, download them to a temporary location and
 * organize.
 */
int
MtpMediaDevice::downloadSelectedItemsToCollection()
{
    QPtrList<MediaItem> items;
    m_view->getSelectedLeaves( 0, &items );

    KTempDir tempdir( QString::null );
    tempdir.setAutoDelete( true );
    KURL::List urls;
    QString genericError = i18n( "Could not copy track from device." );

    int total,progress;
    total = items.count();
    progress = 0;

    if( total == 0 )
        return 0;

    setProgress( progress, total );
    for( MtpMediaItem *it = dynamic_cast<MtpMediaItem*>(items.first()); it && !(m_canceled); it = dynamic_cast<MtpMediaItem*>(items.next()) )
    {
        if( it->type() == MediaItem::TRACK )
        {
            QString filename = tempdir.name() + it->bundle()->filename();
            int ret = LIBMTP_Get_Track_To_File(
                    m_device, it->track()->id(), filename.utf8(),
                    progressCallback, this // callbacks only in libmtp >= 0.0.15
                  );
            if( ret != 0 )
            {
                debug() << "Get Track failed: " << ret << endl;
                Amarok::StatusBar::instance()->shortLongMessage(
                    genericError,
                    i18n( "Could not copy track from device." ),
                    KDE::StatusBar::Error
                );
            }
            else
            {
                urls << filename;
                progress++;
                setProgress( progress );
            }
        }
        else
        {
            total--;
            setProgress( progress, total );
        }
    }
    hideProgress();
    CollectionView::instance()->organizeFiles( urls, i18n( "Move Files To Collection" ), false );
    return 0;
}

/**
 * Write any pending changes to the device, such as database changes
 */
void
MtpMediaDevice::synchronizeDevice()
{
    return;
}

/**
 * Find an existing track
 */
MediaItem
*MtpMediaDevice::trackExists( const MetaBundle &bundle )
{
    MediaItem *artist = dynamic_cast<MediaItem *>( m_view->findItem( bundle.artist(), 0 ) );
    if( artist )
    {
        MediaItem *album = dynamic_cast<MediaItem *>( artist->findItem( bundle.album() ) );
        if( album )
        {
            return dynamic_cast<MediaItem *>( album->findItem( bundle.title() ) );
        }
    }
    return 0;
}

/**
 * Create a new playlist
 */
MtpMediaItem
*MtpMediaDevice::newPlaylist( const QString &name, MediaItem *parent, QPtrList<MediaItem> items )
{
    DEBUG_BLOCK
    MtpMediaItem *item = new MtpMediaItem( parent, this );
    item->setType( MediaItem::PLAYLIST );
    item->setText( 0, name );
    item->setPlaylist( new MtpPlaylist() );

    addToPlaylist( item, 0, items );

    if( ! isTransferring() )
        m_view->rename( item, 0 );

    return item;
}

/**
 * Add an item to a playlist
 */
void
MtpMediaDevice::addToPlaylist( MediaItem *mlist, MediaItem *after, QPtrList<MediaItem> items )
{
    DEBUG_BLOCK
    MtpMediaItem *list = dynamic_cast<MtpMediaItem *>( mlist );
    if( !list )
        return;

    int order;
    MtpMediaItem *it;
    if( after )
    {
        order = after->m_order + 1;
        it = dynamic_cast<MtpMediaItem*>(after->nextSibling());
    }
    else
    {
        order = 0;
        it = dynamic_cast<MtpMediaItem*>( list->firstChild() );
 }

    for(  ; it; it = dynamic_cast<MtpMediaItem *>( it->nextSibling() ) )
    {
        it->m_order += items.count();
    }

    for( MtpMediaItem *it = dynamic_cast<MtpMediaItem *>(items.first() );
            it;
            it = dynamic_cast<MtpMediaItem *>( items.next() ) )
    {
        if( !it->track() )
            continue;

        MtpMediaItem *add;
        if( it->parent() == list )
        {
            add = it;
            if( after )
            {
                it->moveItem(after);
            }
            else
            {
                list->takeItem(it);
                list->insertItem(it);
            }
        }
        else
        {
            if( after )
            {
                add = new MtpMediaItem( list, after );
            }
            else
            {
                add = new MtpMediaItem( list, this );
            }
        }
        after = add;

        add->setType( MediaItem::PLAYLISTITEM );
        add->setTrack( it->track() );
        add->setBundle( new MetaBundle( *(it->bundle()) ) );
        add->m_device = this;
        add->setText( 0, it->bundle()->artist() + " - " + it->bundle()->title() );
        add->m_order = order;
        order++;
    }

    // make numbering consecutive
    int i = 0;
    for( MtpMediaItem *it = dynamic_cast<MtpMediaItem *>( list->firstChild() );
            it;
            it = dynamic_cast<MtpMediaItem *>( it->nextSibling() ) )
    {
        it->m_order = i;
        i++;
    }

    playlistFromItem( list );
}

/**
 * When a playlist has been renamed, we must save it
 */
void
MtpMediaDevice::playlistRenamed( QListViewItem *item, const QString &, int ) // SLOT
{
    DEBUG_BLOCK
    MtpMediaItem *playlist = static_cast<MtpMediaItem*>( item );
    if( playlist->type() == MediaItem::PLAYLIST )
        playlistFromItem( playlist );
}

/**
 * Save a playlist
 */
void
MtpMediaDevice::playlistFromItem( MtpMediaItem *item )
{
    if( item->childCount() == 0 )
        return;
    m_critical_mutex.lock();
    LIBMTP_playlist_t *metadata = LIBMTP_new_playlist_t();
    metadata->name = qstrdup( item->text( 0 ).utf8() );
    uint32_t *tracks = ( uint32_t* )malloc( sizeof( uint32_t ) * item->childCount() );
    uint32_t i = 0;
    for( MtpMediaItem *it = dynamic_cast<MtpMediaItem *>(item->firstChild());
            it;
            it = dynamic_cast<MtpMediaItem *>(it->nextSibling()) )
    {
        tracks[i] = it->track()->id();
        i++;
    }
    metadata->tracks = tracks;
    metadata->no_tracks = i;

    QString genericError = i18n( "Could not save playlist." );

    if( item->playlist()->id() == 0 )
    {
        debug() << "creating new playlist : " << metadata->name << endl;
        int ret = LIBMTP_Create_New_Playlist( m_device, metadata, 0 );
        if( ret == 0 )
        {
            item->playlist()->setId( metadata->playlist_id );
            debug() << "playlist saved : " << metadata->playlist_id << endl;
        }
        else
        {
            Amarok::StatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Could not create new playlist on device." ),
                KDE::StatusBar::Error
            );
        }
    }
    else
    {
        metadata->playlist_id = item->playlist()->id();
        debug() << "updating playlist : " << metadata->name << endl;
        int ret = LIBMTP_Update_Playlist( m_device, metadata );
        if( ret != 0 )
        {
            Amarok::StatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Could not update playlist on device." ),
                KDE::StatusBar::Error
            );
        }
    }
    m_critical_mutex.unlock();
}

/**
 * Recursively remove MediaItem from the device and media view
 */
int
MtpMediaDevice::deleteItemFromDevice(MediaItem* item, int flags )
{

    int result = 0;
    if( isCanceled() )
    {
        return -1;
    }
    MediaItem *next = 0;

    switch( item->type() )
    {
        case MediaItem::PLAYLIST:
        case MediaItem::TRACK:
            if( isCanceled() )
                break;
            if( item )
            {
                int res = deleteObject( dynamic_cast<MtpMediaItem *> ( item ) );
                if( res >=0 && result >= 0 )
                    result += res;
                else
                    result = -1;
            }
            break;
        case MediaItem::PLAYLISTITEM:
            if( isCanceled() )
                break;
            if( item )
            {
                MtpMediaItem *parent = dynamic_cast<MtpMediaItem *> ( item->parent() );
                if( parent->type() == MediaItem::PLAYLIST ) {
                    delete( item );
                    playlistFromItem( parent );
                }
            }
            break;
        case MediaItem::ALBUM:
        case MediaItem::ARTIST:
            // Recurse through the lists
            next = 0;

            if( isCanceled() )
                break;

            for( MediaItem *it = dynamic_cast<MediaItem *>( item->firstChild() ); it ; it = next )
            {
                next = dynamic_cast<MediaItem *>( it->nextSibling() );
                int res = deleteItemFromDevice( it, flags );
                if( res >= 0 && result >= 0 )
                    result += res;
                else
                    result = -1;

            }
            if( item )
                delete dynamic_cast<MediaItem *>( item );
            break;
        default:
            result = 0;
    }
    return result;
}

/**
 * Actually delete a track or playlist
 */
int
MtpMediaDevice::deleteObject( MtpMediaItem *deleteItem )
{
    DEBUG_BLOCK

    u_int32_t object_id;
    if( deleteItem->type() == MediaItem::PLAYLIST )
        object_id = deleteItem->playlist()->id();
    else
        object_id = deleteItem->track()->id();

    QString genericError = i18n( "Could not delete item" );

    debug() << "delete this id : " << object_id << endl;

    m_critical_mutex.lock();
    int status = LIBMTP_Delete_Object( m_device, object_id );
    m_critical_mutex.unlock();

    if( status != 0 )
    {
        debug() << "delete object failed" << endl;
        Amarok::StatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "Delete failed" ),
            KDE::StatusBar::Error
        );
        return -1;
    }
    debug() << "object deleted" << endl;

    // remove from the media view
    delete deleteItem;
    kapp->processEvents( 100 );

    return 1;
}

/**
 * Update local cache of mtp folders
 */
void
MtpMediaDevice::updateFolders( void )
{
    LIBMTP_destroy_folder_t( m_folders );
    m_folders = 0;
    m_folders = LIBMTP_Get_Folder_List( m_device );
}

/**
 * Set cancellation of an operation
 */
void
MtpMediaDevice::cancelTransfer()
{
    m_canceled = true;
}

/**
 * Connect to device, and populate m_view with MediaItems
 */
bool
MtpMediaDevice::openDevice( bool silent )
{
    DEBUG_BLOCK

    Q_UNUSED( silent );

    if( m_device != 0 )
        return true;


    QString genericError = i18n( "Could not connect to MTP Device" );

    m_critical_mutex.lock();
	LIBMTP_Init();
	m_device = LIBMTP_Get_First_Device();
    m_critical_mutex.unlock();
	if( m_device == 0 ) {
        debug() << "No devices." << endl;
        Amarok::StatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "MTP device could not be opened" ),
            KDE::StatusBar::Error
        );
        setDisconnected();
        return false;
	}

    connect(
        m_view, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
        this,   SLOT( playlistRenamed( QListViewItem*, const QString&, int ) )
    );

    QString modelname = QString( LIBMTP_Get_Modelname( m_device ) );
    QString ownername = QString( LIBMTP_Get_Friendlyname( m_device ) );
    m_name = modelname;
    if(! ownername.isEmpty() )
    {
        m_name += " (" + ownername + ')';
    }

    m_default_parent_folder = m_device->default_music_folder;
    debug() << "setting default parent : " << m_default_parent_folder << endl;

    MtpMediaDevice::readMtpMusic();

    m_critical_mutex.lock();
    m_folders = LIBMTP_Get_Folder_List( m_device );
    uint16_t *filetypes;
    uint16_t filetypes_len;
    int ret = LIBMTP_Get_Supported_Filetypes( m_device, &filetypes, &filetypes_len );
    if( ret == 0 )
    {
        uint16_t i;
        for( i = 0; i < filetypes_len; i++ )
        {
            m_supportedFiles << mtpFileTypes[ filetypes[ i ] ];
        }
    }
    free( filetypes );
    m_critical_mutex.unlock();

    return true;
}

/**
 * Start the view (add default folders such as for playlists)
 */
void
MtpMediaDevice::initView()
{
    if( ! isConnected() )
        return;
    m_playlistItem = new MtpMediaItem( m_view, this );
    m_playlistItem->setText( 0, i18n("Playlists") );
    m_playlistItem->setType( MediaItem::PLAYLISTSROOT );
    m_playlistItem->m_order = -1;
}

/**
 * Wrap up any loose ends and close the device
 */
bool
MtpMediaDevice::closeDevice()
{
    DEBUG_BLOCK

    // clear folder structure
    if( m_folders != 0 )
    {
        m_critical_mutex.lock();
        LIBMTP_destroy_folder_t( m_folders );
        m_critical_mutex.unlock();
        m_folders = 0;
        debug() << "Folders destroyed" << endl;
    }

    // release device
    if( m_device != 0 )
    {
        m_critical_mutex.lock();
        LIBMTP_Release_Device( m_device );
        m_critical_mutex.unlock();
        setDisconnected();
        debug() << "Device released" << endl;
    }

    // clean up the view
    clearItems();

    return true;
}

/**
 * Get the capacity and freespace available on the device, in KB
 */
bool
MtpMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !isConnected() )
        return false;

    uint64_t totalbytes;
    uint64_t freebytes;
    char *storage_description;
    char *volume_label;

    m_critical_mutex.lock();
    int ret = LIBMTP_Get_Storageinfo( m_device, &totalbytes, &freebytes, &storage_description, &volume_label );
    m_critical_mutex.unlock();
    if( ret == 0 )
    {
        *total = totalbytes;
        *available = freebytes;
        return true;
    }
    else
    {
        debug() << "couldn't get storage details" << endl;
        return false;
    }
}

/**
 * Get custom information about the device via MTP
 */
void
MtpMediaDevice::customClicked()
{
    QString Information;
    if( isConnected() )
    {
        QString batteryLevel;
        QString secureTime;
        QString storageInformation;
        QString supportedFiles;

        uint8_t maxbattlevel;
        uint8_t currbattlevel;
        uint64_t totalbytes;
        uint64_t freebytes;
        char *storage_description;
        char *volume_label;
        char *sectime;


        m_critical_mutex.lock();
        LIBMTP_Get_Storageinfo( m_device, &totalbytes, &freebytes, &storage_description, &volume_label );
        LIBMTP_Get_Batterylevel( m_device, &maxbattlevel, &currbattlevel );
        LIBMTP_Get_Secure_Time( m_device, &sectime );
        m_critical_mutex.unlock();

        batteryLevel = i18n("Battery level: ")
            + QString::number( (int) ( (float) currbattlevel / (float) maxbattlevel * 100.0 ) )
            + '%';
        secureTime = i18n("Secure time: ") + sectime;
        storageInformation = i18n("Volume label: ")
            + volume_label + '\n'
            + i18n("Storage description: ") + storage_description;
        supportedFiles = i18n("Supported file types: ")
            + m_supportedFiles.join( ", " );

        Information = ( i18n( "Player Information for " )
                        + m_name + '\n' + batteryLevel
                        + '\n' + secureTime + '\n'
                        + storageInformation + '\n' + supportedFiles );
        free(storage_description);
        free(volume_label);
        free(sectime);
    }
    else
    {
        Information = i18n( "Player not connected" );
    }

    KMessageBox::information( 0, Information, i18n( "Device information" ) );
}

/**
 * Current device
 */
LIBMTP_mtpdevice_t
*MtpMediaDevice::current_device()
{
    return m_device;
}

/**
 * We use a 0 device to show a disconnected device.
 * This sets the device to that.
 */
void
MtpMediaDevice::setDisconnected()
{
    m_device = 0;
}

/**
 * Handle clicking of the right mouse button
 */
void
MtpMediaDevice::rmbPressed( QListViewItem *qitem, const QPoint &point, int )
{

    enum Actions {RENAME, DOWNLOAD, DELETE, MAKE_PLAYLIST};

    MtpMediaItem *item = static_cast<MtpMediaItem *>( qitem );
    if( item )
    {
        KPopupMenu menu( m_view );
        switch( item->type() )
        {
        case MediaItem::ARTIST:
        case MediaItem::ALBUM:
        case MediaItem::TRACK:
            menu.insertItem( SmallIconSet( Amarok::icon( "collection" ) ), i18n("&Copy Files to Collection..."), DOWNLOAD );
            menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n( "Make Media Device Playlist" ), MAKE_PLAYLIST );
            break;
        case MediaItem::PLAYLIST:
            menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "Rename" ), RENAME );
            break;
        default:
            break;
        }

        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "Delete from device" ), DELETE );

        int id =  menu.exec( point );
        switch( id )
        {
        case MAKE_PLAYLIST:
            {
                QPtrList<MediaItem> items;
                m_view->getSelectedLeaves( 0, &items );
                QString name = i18n( "New Playlist" );
                newPlaylist( name, m_playlistItem, items );
            }
            break;
        case DELETE:
            MediaDevice::deleteFromDevice();
            break;
        case RENAME:
            if( item->type() == MediaItem::PLAYLIST && ! isTransferring() )
            {
                m_view->rename( item, 0 );
            }
            break;
        case DOWNLOAD:
            downloadSelectedItemsToCollection();
            break;
        }
    }
    return;
}

/**
 * Add gui elements to the device configuration
 */
void
MtpMediaDevice::addConfigElements( QWidget *parent )
{

    m_folderLabel = new QLabel( parent );
    m_folderLabel->setText( i18n( "Folder structure:" ) );

    m_folderStructureBox = new QLineEdit( parent );
    m_folderStructureBox->setText( m_folderStructure );
    QToolTip::add( m_folderStructureBox,
        i18n( "Files copied to the device will be placed in this folder." ) + '\n'
        + i18n( "/ is used as folder separator." ) + '\n'
        + i18n( "%a will be replaced with the artist name, ")
        + i18n( "%b with the album name," ) + '\n'
        + i18n( "%g with the genre.") + '\n'
        + i18n( "An empty path means the files will placed unsorted in the default music folder." ) );
}

/**
 * Remove gui elements from the device configuration
 */
void
MtpMediaDevice::removeConfigElements( QWidget *parent)
{
    Q_UNUSED(parent)

    delete m_folderStructureBox;
    m_folderStructureBox = 0;

    delete m_folderLabel;
    m_folderLabel = 0;
}

/**
 * Save changed config after dialog commit
 */
void
MtpMediaDevice::applyConfig()
{
    m_folderStructure = m_folderStructureBox->text();
    setConfigString( "FolderStructure", m_folderStructure );
}

/**
 * Load config from the amarokrc file
 */
void
MtpMediaDevice::loadConfig()
{
    m_folderStructure = configString( "FolderStructure","%a - %b" );
}

/**
 * Add a track to the current list view
 */
MtpMediaItem
*MtpMediaDevice::addTrackToView( MtpTrack *track, MtpMediaItem *item )
{
    QString artistName = track->bundle()->artist();

    MtpMediaItem *artist = dynamic_cast<MtpMediaItem *>( m_view->findItem( artistName, 0 ) );
    if( !artist )
    {
        artist = new MtpMediaItem(m_view);
        artist->m_device = this;
        artist->setText( 0, artistName );
        artist->setType( MediaItem::ARTIST );
    }

    QString albumName = track->bundle()->album();
    MtpMediaItem *album = dynamic_cast<MtpMediaItem *>( artist->findItem( albumName ) );
    if( !album )
    {
        album = new MtpMediaItem( artist );
        album->setText( 0, albumName );
        album->setType( MediaItem::ALBUM );
        album->m_device = this;
    }

    if( item )
        album->insertItem( item );
    else
    {
        item = new MtpMediaItem( album );
        item->m_device = this;
        QString titleName = track->bundle()->title();
        item->setTrack( track );
        item->m_order = track->bundle()->track();
        item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->setBundle( track->bundle() );
        item->track()->setId( track->id() );
        m_fileNameToItem[ track->bundle()->filename() ] = item;
        m_idToTrack[track->id()] = track;
    }
    return item;
}

/**
 * Get tracks and add them to the listview
 */
int
MtpMediaDevice::readMtpMusic()
{

    DEBUG_BLOCK

    clearItems();

    m_critical_mutex.lock();

    QString genericError = i18n( "Could not get music from MTP Device" );

    int total = 100;
    int progress = 0;
    setProgress( progress, total ); // we don't know how many tracks. fake progress bar.

    kapp->processEvents( 100 );

    LIBMTP_track_t *tracks = LIBMTP_Get_Tracklisting( m_device );

    debug() << "Got tracks from device" << endl;

    if( tracks == 0 )
    {
        debug()<< "Error reading tracks. 0 returned." << endl;
        Amarok::StatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "Could not read MTP Device tracks" ),
            KDE::StatusBar::Error
        );
        hideProgress();
        return -1;
    }
    else
    {
        LIBMTP_track_t *tmp = tracks;
        total = 0;
        // spin through once to determine size of the list
        while( tracks != 0 )
        {
            tracks = tracks->next;
            total++;
        }
        setProgress( progress, total );
        tracks = tmp;
        // now process the tracks
        while( tracks != 0 )
        {
            MtpTrack *mtp_track = new MtpTrack( tracks );
            mtp_track->readMetaData( tracks );
            addTrackToView( mtp_track );
            tmp = tracks;
            tracks = tracks->next;
            LIBMTP_destroy_track_t( tmp );
            progress++;
            setProgress( progress );
            if( progress % 50 == 0 )
                kapp->processEvents( 100 );
        }
    }
    setProgress( total );
    hideProgress();

    m_critical_mutex.unlock();

    readPlaylists();

    return 0;
}

/**
 * Populate playlists
 */
void
MtpMediaDevice::readPlaylists()
{
    m_critical_mutex.lock();
    LIBMTP_playlist_t *playlists = LIBMTP_Get_Playlist_List( m_device );

    if( playlists != 0 )
    {
        LIBMTP_playlist_t *tmp;
        while( playlists != 0 )
        {
            MtpMediaItem *playlist = new MtpMediaItem( m_playlistItem, this );
            playlist->setText( 0, QString::fromUtf8( playlists->name ) );
            playlist->setType( MediaItem::PLAYLIST );
            playlist->setPlaylist( new MtpPlaylist() );
            playlist->playlist()->setId( playlists->playlist_id );
            uint32_t i;
            for( i = 0; i < playlists->no_tracks; i++ )
            {
                MtpTrack *track = m_idToTrack[ playlists->tracks[i] ];
                if( track == 0 ) // skip invalid playlist entries
                    continue;
                MtpMediaItem *item = new MtpMediaItem( playlist );
                item->setText( 0, track->bundle()->artist() + " - " + track->bundle()->title() );
                item->setType( MediaItem::PLAYLISTITEM );
                item->setBundle( track->bundle() );
                item->setTrack( track );
                item->m_order = i;
                item->m_device = this;
            }
            tmp = playlists;
            playlists = playlists->next;
            LIBMTP_destroy_playlist_t( tmp );
        }
        kapp->processEvents( 100 );
    }
    m_critical_mutex.unlock();
}

/**
 * Clear the current listview
 */
void
MtpMediaDevice::clearItems()
{
    m_view->clear();
    initView();
}

/**
 * MtpTrack Class
 */
MtpTrack::MtpTrack( LIBMTP_track_t *track )
{
    m_id = track->item_id;
}

/**
 * Read track properties from the device and set it on the track
 */
void
MtpTrack::readMetaData( LIBMTP_track_t *track )
{
    MetaBundle *bundle = new MetaBundle();

    if( track->genre != 0 )
        bundle->setGenre( AtomicString( QString::fromUtf8( track->genre ) ) );
    if( track->artist != 0 )
        bundle->setArtist( AtomicString( QString::fromUtf8( track->artist ) ) );
    if( track->album != 0 )
        bundle->setAlbum( AtomicString( QString::fromUtf8( track->album ) ) );
    if( track->title != 0 )
        bundle->setTitle( AtomicString( QString::fromUtf8( track->title ) ) );
    if( track->filename != 0 )
        bundle->setPath( AtomicString( QString::fromUtf8( track->filename ) ) );

    // translate codecs to file types
    if( track->filetype == LIBMTP_FILETYPE_MP3 )
        bundle->setFileType( MetaBundle::mp3 );
    else if( track->filetype == LIBMTP_FILETYPE_WMA )
        bundle->setFileType( MetaBundle::wma );
    else if( track->filetype == LIBMTP_FILETYPE_OGG )
        bundle->setFileType( MetaBundle::ogg );
    else
        bundle->setFileType( MetaBundle::other );

    if( track->date != 0 )
        bundle->setYear( QString( QString::fromUtf8( track->date ) ).mid( 0, 4 ).toUInt() );
    if( track->tracknumber > 0 )
        bundle->setTrack( track->tracknumber );
    if( track->duration > 0 )
        bundle->setLength( track->duration / 1000 ); // Divide by 1000 since this is in milliseconds

    this->setBundle( *bundle );
}

/**
 * Set this track's metabundle
 */
void
MtpTrack::setBundle( MetaBundle &bundle )
{
    m_bundle = bundle;
}
