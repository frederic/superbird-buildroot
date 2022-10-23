#ifndef _Aml_MediaRenderDelegate_H
#define _Aml_MediaRenderDelegate_H
#include "PltUPnP.h"
#include "PltMediaRenderer.h"
#include "NptLogging.h"
#include <gst/gst.h>
#include <glib.h>
#ifndef NULL
#define NULL 0
#endif
NPT_SET_LOCAL_LOGGER( "platinum.android.jni" )
class Aml_MediaRenderDelegate;
class Aml_GstPlayer: public NPT_Thread
{
public:
    Aml_GstPlayer( Aml_MediaRenderDelegate *render ): playbin( NULL ), bus( NULL ), terminate( 0 ), delegate( render ) {};
    virtual void Run();
    void setDataSource( NPT_String uri );
    void start();
    void stop();
    void release();
    void pause();
    void seek( NPT_String &time );
    void setVolume( NPT_String &value );
    virtual ~Aml_GstPlayer() {}
private:
    NPT_String uri;
    GstElement *playbin;
    int terminate;
    GstBus *bus;
    Aml_MediaRenderDelegate *delegate;
};
class Aml_MediaRenderDelegate: public PLT_MediaRendererDelegate
{
public:
    Aml_MediaRenderDelegate(): gst_player( NULL ), service( NULL ) {};
    // ConnectionManager
    virtual NPT_Result OnGetCurrentConnectionInfo( PLT_ActionReference &action );

    // AVTransport
    virtual NPT_Result OnNext( PLT_ActionReference &action );
    virtual NPT_Result OnPause( PLT_ActionReference &action );
    virtual NPT_Result OnPlay( PLT_ActionReference &action );
    virtual NPT_Result OnPrevious( PLT_ActionReference &action );
    virtual NPT_Result OnSeek( PLT_ActionReference &action );
    virtual NPT_Result OnStop( PLT_ActionReference &action );
    virtual NPT_Result OnSetAVTransportURI( PLT_ActionReference &action );
    virtual NPT_Result OnSetPlayMode( PLT_ActionReference &action );

    // RenderingControl
    virtual NPT_Result OnSetVolume( PLT_ActionReference &action );
    virtual NPT_Result OnSetVolumeDB( PLT_ActionReference &action );
    virtual NPT_Result OnGetVolumeDBRange( PLT_ActionReference &action );
    virtual NPT_Result OnSetMute( PLT_ActionReference &action );
    virtual ~Aml_MediaRenderDelegate();
    virtual void setStopStatus();
private:
    Aml_GstPlayer *gst_player;
    PLT_Service *service;
};

#endif
