#!/usr/bin/env python

############################################################################
# Python-Qt template script for amaroK
# (c) 2005 Mark Kretschmann <markey@web.de>
#
# Depends on: Python 2.2, PyQt
############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
############################################################################

import ConfigParser
import os
import Queue
import sys
import threading

from qt import *

# Replace with real name
debug_prefix = "[Test Script]"


class ConfigDialog( QDialog ):
""" Configuration widget """

    def __init__( self ):
        QDialog.__init__( self )
        self.setWFlags( Qt.WDestructiveClose )
        self.setCaption( "Test Script - amaroK" )

        self.adjustSize()

    def save( self ):
    """ Saves configuration to file """

        self.file = file( "testrc", 'w' )

        self.config = ConfigParser()
        self.config.add_section( "General" )
        self.config.set( "General", "foo", foovar )
        self.config.write( self.file )
        self.file.close()

        self.accept()


class Test( QApplication ):
""" The main application, also sets up the Qt event loop """

    def __init__( self, args ):
        QApplication.__init__( self, args )

        self.queue = Queue.Queue()
        self.startTimer( 100 )

        self.t = threading.Thread( target = self.readStdin )
        self.t.start()

        self.alarmTimer = QTimer()
        self.connect( self.alarmTimer, SIGNAL( "timeout()" ), self.wakeup )

        self.readSettings()

    def readSettings( self ):
    """ Reads settings from configuration file """

        try:
            foovar = config.get( "General", "foo" )

        except:
            debug( "No config file found, using defaults." )


############################################################################
# Stdin-Reader Thread
############################################################################

    def readStdin( self ):
        """ Reads incoming notifications from stdin """

        while True:
            line = sys.stdin.readline()

            if line:
                self.queue.put_nowait( line )
            else:
                break


############################################################################
# Notification Handling
############################################################################

    def timerEvent( self, event ):
    """ Polls the notification queue at regular interval """

        if not self.queue.empty():
            string = QString( self.queue.get_nowait() )
            debug( "Received notification: " + str( string ) )

            if string.contains( "configure" ):
                self.configure()

            if string.contains( "EngineStateChange: play" ):
                self.engineStatePlay()

            if string.contains( "EngineStateChange: idle" ):
                self.engineStateIdle()

            if string.contains( "EngineStateChange: pause" ):
                self.engineStatePause()

            if string.contains( "EngineStateChange: empty" ):
                self.engineStatePause()


# Notification callbacks. Implement these functions to react to specific notification
# events from amaroK:

    def configure( self ):
        debug( "Alarm Script: configuration" )

        self.dia = ConfigDialog()
        self.dia.show()
        self.connect( self.dia, SIGNAL( "destroyed()" ), self.readSettings )

    def engineStatePlay( self ):
        pass

    def engineStateIdle( self ):
        pass

    def engineStatePause( self ):
        pass

    def engineStateEmpty( self ):
        pass


############################################################################

def debug( message ):
""" Prints debug message to stdout """

    print debug_prefix + " " + message

def main( args ):
    app = Alarm( args )

    app.exec_loop()

if __name__ == "__main__":
    main( sys.argv )

