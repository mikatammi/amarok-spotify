/***************************************************************************
  begin                : Fre Nov 15 2002
  copyright            : (C) Mark Kretschmann <markey@web.de>
                       : (C) Max Howell <max.howell@methylblue.com>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTWINDOW_H
#define AMAROK_PLAYLISTWINDOW_H

//#include "browserbar.h"
#include "amarok_export.h"

#include <khbox.h>          //baseclass for DynamicBox
#include <qwidget.h>        //baseclass
//Added by qt3to4:
#include <QShowEvent>
#include <QEvent>
#include <QLabel>
#include <QCloseEvent>
#include <QPaintEvent>
#include <kxmlguiclient.h>  //baseclass (for XMLGUI)
#include <kmainwindow.h>
// #include <phonon/phononnamespace.h>
class KLineEdit;
class CollectionBrowser;
class ContextBrowser;
class MediaBrowser;
class QMenuBar;
class SideBar;
class KMenu;
class KToolBar;
class QLabel;
class QTimer;
class KLineEdit;

// namespace Phonon {
//     class VideoWidget;
// }
/**
  * @class PlaylistWindow
  * @short The PlaylistWindow widget class.
  *
  * This is the main window widget (the Playlist not Player).
  */
class /*AMAROK_EXPORT*/ PlaylistWindow : public KMainWindow//public QWidget, public KXMLGUIClient
{
        Q_OBJECT

    public:
        PlaylistWindow();
       ~PlaylistWindow();

        void init();
        void applySettings();

        void createGUI(); //should be private but App::slowConfigToolbars requires it
        void recreateGUI();

        //allows us to switch browsers from within other browsers etc
        void showBrowser( const QString& name );
        void addBrowser( const QString &name, QWidget *widget, const QString &text, const QString &icon );

        //takes into account minimized, multiple desktops, etc.
        bool isReallyShown() const;

        virtual bool eventFilter( QObject*, QEvent* );

        //instance is declared in KXMLGUI
        static PlaylistWindow *self() { return s_instance; }

        void activate();

        void showVideo( bool );
//         Phonon::VideoWidget *videoWidget() { return m_videoWidget; }

        SideBar *sideBar() const { return m_browsers; };

    public slots:
        void showHide();
        void mbAvailabilityChanged( bool isAvailable );

    private slots:
        void savePlaylist() const;
        void slotBurnPlaylist() const;
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void slotAddStream();
        void playLastfmPersonal();
        void addLastfmPersonal();
        void playLastfmNeighbor();
        void addLastfmNeighbor();
        void playLastfmCustom();
        void addLastfmCustom();
        void playLastfmGlobaltag( int );
        void addLastfmGlobaltag( int );
        void playAudioCD();
        void showQueueManager();
        void showScriptSelector();
        void showStatistics();
        void slotMenuActivated( int );
        void actionsMenuAboutToShow();
        void toolsMenuAboutToShow();
        void slotToggleMenu();
        void slotToggleFocus();
        void slotEditFilter();
        void slotSetFilter( const QString &filter );

    protected:
        virtual void closeEvent( QCloseEvent* );
        virtual void showEvent( QShowEvent* );
        virtual QSize sizeHint() const;

    private:
        enum MenuId { ID_SHOW_TOOLBAR = 2000, ID_SHOW_PLAYERWINDOW };

        QMenuBar      *m_menubar;
        KMenu         *m_toolsMenu;
        KMenu         *m_settingsMenu;
        SideBar       *m_browsers;
        QStringList    m_browserNames;
        KMenu         *m_searchMenu;

        KLineEdit     *m_searchLine;
        KToolBar      *m_toolbar;
        QTimer        *m_timer;  //search filter timer
        QStringList    m_lastfmTags;
        MediaBrowser  *m_currMediaBrowser;

        void    createActions();
        void    createMenus();
        int m_lastBrowser;
        int m_searchField;
//         Phonon::VideoWidget *m_videoWidget;
        ContextBrowser *cb;

        static PlaylistWindow *s_instance;
};

class DynamicTitle : public QWidget
{
    Q_OBJECT
    public:
        DynamicTitle(QWidget* parent);
        void setTitle(const QString& newTitle);

    protected:
        virtual void paintEvent(QPaintEvent* e);

    private:
        static const int s_curveWidth = 5;
        static const int s_imageSize = 16;
        QString m_title;
        QFont m_font;
};

class DynamicBar : public KHBox
{
    Q_OBJECT
    public:
        DynamicBar(QWidget* parent);
        void init();

    public slots:
        void slotNewDynamicMode(const DynamicMode* mode);
        void changeTitle(const QString& title);

    private:
        DynamicTitle* m_titleWidget;
};


#endif //AMAROK_PLAYLISTWINDOW_H
