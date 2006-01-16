/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2002-2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * Maintainer:        Robert Gogolok <gogo@graphics.cs.uni-sb.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include "nmm_engine.h"

#include "nmm_kdeconfig.h"
#include "nmm_configdialog.h"
#include "debug.h"
#include "plugin/plugin.h"

#include <nmm/base/graph/GraphBuilder2.hpp>
#include <nmm/base/registry/NodeDescription.hpp>
#include <nmm/base/ProxyApplication.hpp>
#include <nmm/interfaces/base/sync/ISynchronizedSink.hpp>
#include <nmm/interfaces/file/ISeekable.hpp>
#include <nmm/interfaces/file/ITrack.hpp>
#include <nmm/interfaces/file/IBufferSize.hpp>
#include <nmm/interfaces/general/progress/IProgressListener.hpp>
#include <nmm/interfaces/general/progress/IProgress.hpp>
#include <nmm/interfaces/general/ITrackDuration.hpp>
#include <nmm/interfaces/device/audio/IAudioDevice.hpp>
#include <nmm/base/ProxyObject.hpp>

#include <qapplication.h>
#include <qtimer.h>

#include <kfileitem.h>
#include <kmimetype.h>
#include <iostream>
#include <kurl.h>

NmmEngine* NmmEngine::s_instance;

AMAROK_EXPORT_PLUGIN( NmmEngine )

NmmEngine::NmmEngine()
  : Engine::Base(),
    __position(0),
    __track_length(0),
    __state(Engine::Empty),
    __app(NULL),
    __endTrack_listener(this, &NmmEngine::endTrack),
    __syncReset_listener(this, &NmmEngine::syncReset),
    __setProgress_listener(this, &NmmEngine::setProgress),
    __trackDuration_listener(this, &NmmEngine::trackDuration),
    __composite(NULL),
    __playback(NULL),
    __display(NULL),
    __av_sync(NULL),
    __synchronizer(NULL),
    __with_video(false),
    __seeking(false)
{
  addPluginProperty( "HasConfigure", "true" );
}

bool NmmEngine::init()
{
  DEBUG_BLOCK
  
  s_instance = this;
    
  // disable debug and warning streams
  NamedObject::getGlobalInstance().setDebugStream(NULL, NamedObject::ALL_LEVELS);
  NamedObject::getGlobalInstance().setWarningStream(NULL, NamedObject::ALL_LEVELS);

  // create new NMM application object
  __app = ProxyApplication::getApplication(0, 0);

  return true;
}

NmmEngine::~NmmEngine()
{
  // stop all nodes
  stop();

  // delete application object
  if (__app)
    delete __app;
}

Engine::State NmmEngine::state() const
{
  return __state;
}

amaroK::PluginConfig* NmmEngine::configure() const
{
    NmmConfigDialog* dialog = new NmmConfigDialog();

    // connect...
    
    return dialog;
}

bool NmmEngine::load(const KURL& url, bool stream)
{
  DEBUG_BLOCK

  bool ret = true;

  // play only local files
  if (!url.isLocalFile()) return false;

  Engine::Base::load(url, stream);

  cleanup();

  // TODO: in general look what to cleanup if expection is thrown...
  // make the GraphBuilder construct an appropriate graph for the given URL
  try {
    QString host;

    // these nodes will be used for audio and video playback
    NodeDescription playback_nd("PlaybackNode");
    if( NmmKDEConfig::audioOutputPlugin() == "ALSAPlaybackNode" )
        playback_nd = NodeDescription("ALSAPlaybackNode");
    if (!(host=getAudioSinkHost()).isEmpty())
        playback_nd.setLocation(host.ascii());

    NodeDescription display_nd("XDisplayNode");
    if (!(host=getVideoSinkHost()).isEmpty())
        display_nd.setLocation(host.ascii());

    GraphBuilder2 gb;

    // convert the URL to a valid NMM url
    if(!gb.setURL("file://" + string(url.path().ascii()))) {
      throw Exception("Invalid URL given");
    }

    ClientRegistry& registry = __app->getRegistry();
    {
      RegistryLock lock(registry);
      // TODO: release __playback and __display when exception thrown?

      // get a playback node interface from the registry
      list<Response> playback_response = registry.initRequest(playback_nd);
      if (playback_response.empty())
        throw Exception("PlaybackNode is not available on host " + playback_nd.getLocation() );
      __playback = registry.requestNode(playback_response.front());

      // get a display node interface from the registry
      list<Response> display_response = registry.initRequest(display_nd);
      if (display_response.empty())
        throw Exception("Display node is not available on host" + display_nd.getLocation() );
      __display = registry.requestNode(display_response.front());

    }

    __av_sync = new MultiAudioVideoSynchronizer();
    __synchronizer = __av_sync->getCheckedInterface<IMultiAudioVideoSynchronizer>();

    // initialize the GraphBuilder
    gb.setMultiAudioVideoSynchronizer(__synchronizer);
    gb.setAudioSink(__playback);
    gb.setVideoSink(__display);
    gb.setDemuxAudioJackTag("audio");
    gb.setDemuxVideoJackTag("video");

    // create the graph represented by a composite node
    __composite = gb.createGraph(*__app);

    // if the display node is connected we know we will play a video
    __with_video = __display->isInputConnected();
    debug() << " __with_video " << __with_video << endl;

    // set volume for playback node
    setVolume( m_volume );

    // register the needed event listeners at the display node if video enabled
    if(__with_video) {
        __display->getParentObject()->registerEventListener(ISyncReset::syncReset_event, &__syncReset_listener);
        __display->getParentObject()->registerEventListener(IProgressListener::setProgress_event, &__setProgress_listener);
        __display->getParentObject()->registerEventListener(ITrack::endTrack_event, &__endTrack_listener);

    }
    else { // in other case at the playback node
        __playback->getParentObject()->registerEventListener(ISyncReset::syncReset_event, &__syncReset_listener);
        __playback->getParentObject()->registerEventListener(IProgressListener::setProgress_event, &__setProgress_listener);
        __playback->getParentObject()->registerEventListener(ITrack::endTrack_event, &__endTrack_listener);


        (__playback->getParentObject()->getCheckedInterface<ISynchronizedSink>())->setSynchronized(false);
    }

    __playback->getParentObject()->registerEventListener(ITrackDuration::trackDuration_event, &__trackDuration_listener);
    __display->getParentObject()->registerEventListener(ITrackDuration::trackDuration_event, &__trackDuration_listener);

    // Tell the node that implements the IProgress interface to send progress events frequently.
    IProgress_var progress(__composite->getInterface<IProgress>());
    if (progress.get()) {
      progress->sendProgressInformation(true);
      progress->setProgressInterval(1);
    }

    // minimize the buffer size to increase the frequency of progress events
    IBufferSize_var buffer_size(__composite->getInterface<IBufferSize>());
    if (buffer_size.get()) {
      buffer_size->setBufferSize(1000);
    }

    // we don't know the track length yet - we have to wait for the trackDuration event
    __track_length = 0;

    // finally start the graph
    if(__playback->isActivated())
      __playback->reachStarted();
    if(__display->isActivated())
      __display->reachStarted();

    __composite->reachStarted();
 
    __seeking = false;
    __state = Engine::Playing;
  }
  catch (const Exception& e) {
    cerr << e << endl;
    QString status = e.getComment().c_str() ;
    emit statusText( QString("NMM engine: ") + status );
    ret = false;
  }
  catch(...) {
    emit statusText( "NMM engine: Something went wrong..." );
    ret = false;
  }

  return ret;
}

bool NmmEngine::play(uint)
{
  DEBUG_BLOCK

  if (!__composite)
    return false;

  // wake up if paused
  ISynchronizedSink_var sync_sink(__playback->getParentObject()->getCheckedInterface<ISynchronizedSink>());
  if (sync_sink.get()) {
    sync_sink->getController()->wakeup();
  }

  __state = Engine::Playing;
  emit stateChanged(Engine::Playing);

  return true;
}

void NmmEngine::cleanup()
{
  DEBUG_BLOCK

  if (!__composite) {
    return;
  }

  // remove all event listeners
  if(__with_video) {
    __display->getParentObject()->removeEventListener(&__setProgress_listener);
    __display->getParentObject()->removeEventListener(&__endTrack_listener);
    __display->getParentObject()->removeEventListener(&__syncReset_listener);
  } 
  else {
    __playback->getParentObject()->removeEventListener(&__setProgress_listener);
    __playback->getParentObject()->removeEventListener(&__endTrack_listener);
    __playback->getParentObject()->removeEventListener(&__syncReset_listener);
  }
  __playback->getParentObject()->removeEventListener(&__trackDuration_listener);
  __display->getParentObject()->removeEventListener(&__trackDuration_listener);

  // stop the graph
  __composite->reachActivated();

  if(__playback->isStarted()) {
    __playback->reachActivated();
  }

  if(__display->isStarted()) {
    __display->reachActivated();
  }

  __composite->flush();
  __composite->reachConstructed();

  __playback->flush();
  __playback->reachConstructed();
  
  //__display->flush();
  //__display->reachConstructed();
 
  // release the playback and video node
  ClientRegistry& registry = __app->getRegistry();
  {
    RegistryLock lock(registry);
    registry.releaseNode(*__playback);
    registry.releaseNode(*__display);
  }

  delete __composite;
  __composite = 0;
  __playback = 0;
  __display = 0;
  __with_video = false;

  delete __synchronizer; 
  __synchronizer = 0;
  delete __av_sync; 
  __av_sync = 0;

  __position = 0;
  __state = Engine::Idle;
}

void NmmEngine::stop()
{
  DEBUG_BLOCK

  cleanup();

  __state = Engine::Empty;
  emit stateChanged(Engine::Empty);

}

void NmmEngine::pause()
{
  if (!__composite)
    return;

  // pause or play...
  if (__state == Engine::Playing) {
    __synchronizer->pause();
    __state = Engine::Paused;
    emit stateChanged(Engine::Paused);
  }
  else {
    __synchronizer->wakeup();
    play();
  }
}

void NmmEngine::seek(uint ms)
{
  if (!__track_length)
    return;

  __seeking = true;
  __position = ms;

  ISeekable_var seek(__composite->getCheckedInterface<ISeekable>());
  if (seek.get())
    seek->seekPercentTo(Rational(ms, __track_length));
}

void NmmEngine::endOfStreamReached()
{
    DEBUG_BLOCK
    emit trackEnded();
}

uint NmmEngine::position() const
{
  return __position;
}

uint NmmEngine::length() const
{
  return __track_length;
}

bool NmmEngine::canDecode(const KURL& url) const
{
    static QStringList types;

    if (url.protocol() == "http" ) return false; 

    // the following MIME types can be decoded
    types += QString("audio/x-mp3");
    types += QString("audio/x-wav");
    types += QString("audio/ac3");
    types += QString("audio/vorbis");
    types += QString("video/mpeg");
    types += QString("video/x-msvideo");
    types += QString("video/x-ogm");

    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url, false ); //false = determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();

    return types.contains(mimetype->name());
}


void NmmEngine::setVolumeSW(uint percent)
{
    if( __playback )
    {
        IAudioDevice_var audio(__playback->getParentObject()->getCheckedInterface<IAudioDevice>());
        audio->setVolume( percent );
    }
}

// minimum of two positive numbers a and b
// negativ numbers are ignored (if both are negative return 0)
int minpos(int a, int b)
{
    return std::max(std::min(a,b), 0);
}

QString NmmEngine::getAudioSinkHost()
{
    if (NmmKDEConfig::audioLocation() == NmmKDEConfig::EnumAudioLocation::AudioSinkEnvVariable)
    {
        QString host(getenv("SOUND"));
        if (!host.isEmpty())
            return host;

        host = getenv("DISPLAY");
        return host.left( minpos( host.find('.'), host.find(':') ) );
    }
    else if (NmmKDEConfig::audioLocation() == NmmKDEConfig::EnumAudioLocation::AudioSinkHostName)
    {
        return NmmKDEConfig::audioHost()[0]; // TODO: for now we return the first host in the list.
    }
    return QString();
}

QString NmmEngine::getVideoSinkHost()
{
    if( NmmKDEConfig::videoLocation() == NmmKDEConfig::EnumVideoLocation::VideoSinkEnvVariable )
    {
        QString host(getenv("DISPLAY"));
        return host.left( minpos( host.find('.'), host.find(':') ) );
    }
    else if( NmmKDEConfig::videoLocation() == NmmKDEConfig::EnumVideoLocation::VideoSinkHostName )
    {
        return NmmKDEConfig::videoHost()[0]; // TODO: for now we return the first host in the list.
    }
    return QString();
}



Result NmmEngine::setProgress(u_int64_t& numerator, u_int64_t& denominator)
{
  // compute the track position in milliseconds
  u_int64_t position = numerator * __track_length / denominator;

  if (__seeking)
    return SUCCESS;
  
  __position = position;

  return SUCCESS;
}

Result NmmEngine::endTrack()
{
  __state = Engine::Idle;
  __position = 0;

  // cleanup after this method returned
  QTimer::singleShot( 0, instance(), SLOT( endOfStreamReached() ) );

  return SUCCESS;
}

Result NmmEngine::syncReset()
{
  __seeking = false;
  return SUCCESS;
}

Result NmmEngine::trackDuration(Interval& duration)
{
  // we got the duration of the track, so let's convert it to milliseconds
  __track_length = duration.sec * 1000 + duration.nsec / 1000;
  kdDebug() << "NmmEngine::trackDuration " << __track_length << endl;
  return SUCCESS;
}

#include "nmm_engine.moc"
