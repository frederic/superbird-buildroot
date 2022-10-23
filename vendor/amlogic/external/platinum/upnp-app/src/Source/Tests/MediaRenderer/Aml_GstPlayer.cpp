#include "Aml_MediaRenderDelegate.h"
#include "PltUPnP.h"
#include "unistd.h"
#include <gst/gst.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>
#include <gst/pbutils/pbutils.h>
#include <gst/tag/tag.h>
#include <gst/math-compat.h>
#include <gst/audio/streamvolume.h>

#define VOLUME_STEPS 20

void Aml_GstPlayer::Run()
{
    GstMessage *msg = NULL;
    GError *err;
    gchar *debug_info;
    while ( this->playbin ) {
        bus = gst_element_get_bus ( GST_ELEMENT( playbin ) );
        msg = gst_bus_timed_pop_filtered ( bus, 1100 * GST_MSECOND,
                                           GST_MESSAGE_EOS );
        if ( msg ) {
            gst_message_unref( msg );
            gst_element_set_state ( playbin, GST_STATE_NULL );
            gst_object_unref ( playbin );
            playbin = NULL;
            msg = NULL;
            if ( this->delegate )
                this->delegate->setStopStatus();
        } else {
            GstFormat fmt = GST_FORMAT_TIME;
            gint64 current = -1;
            gint64 duration = -1;
            if ( !gst_element_query_position ( playbin, fmt, &duration ) ) {
                g_printerr ( "Could not query current position.\n" );
            }
            if ( !GST_CLOCK_TIME_IS_VALID ( duration ) ) {
                if ( !gst_element_query_duration ( playbin, fmt, &duration ) ) {
                    g_printerr ( "Could not query current duration.\n" );
                }
            }
            //g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",  GST_TIME_ARGS (current), GST_TIME_ARGS (duration));
        }
    }
}

void Aml_GstPlayer::setDataSource( NPT_String name )
{
    uri = name;
    gst_init( NULL, NULL );
    uri = NPT_String( "playbin uri=" ) + name;
    playbin = gst_parse_launch( uri.GetChars(), NULL );
    terminate = 1;
    if ( !playbin ) {
        NPT_LOG_WARNING( "'playbin' gstreamer plugin missing" );
        return;
    }
    NPT_LOG_WARNING( "end setDataSource" );
}
void Aml_GstPlayer::start()
{
    if ( playbin ) {
        gst_element_set_state ( playbin, GST_STATE_PLAYING );
    }
}
void Aml_GstPlayer::stop()
{
    if ( playbin ) {
        gst_element_set_state ( playbin, GST_STATE_PAUSED );
    }
}

void Aml_GstPlayer::setVolume( NPT_String &value )
{
    int vol;
    value.ToInteger(vol);
    if ( playbin ) {
        //printf("setVolume:%d\n",vol);
        gst_set_relative_volume(playbin,vol);
    }
}
void Aml_GstPlayer::pause()
{
    if ( playbin ) {
        gst_element_set_state ( playbin, GST_STATE_PAUSED );
    }
}
void Aml_GstPlayer::seek( NPT_String &value )
{
    int h = 0;
    int m = 0;
    int s = 0;
    int i = 0;
    guint time = 0;
    if ( playbin ) {
        NPT_UInt64 time = -1;
        NPT_Array<NPT_String> arr = value.SplitAny( ":" );
        if ( arr.GetItemCount() == 3 ) {
            arr[0].ToInteger( h );
            arr[1].ToInteger( m );
            arr[2].ToInteger( s );
        }
        g_printerr ( "seek to %d:%d:%d\n", h, m, s );
        time = ( ( h * 60 ) + m ) * 60 + s;
        g_printerr ( "seek to %u%d\n", time, arr.GetItemCount() );
        gst_element_seek_simple ( playbin, GST_FORMAT_TIME,
           GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT ), time * GST_SECOND );
    }
}
void Aml_GstPlayer::release()
{
    if ( playbin ) {
        gst_element_set_state ( playbin, GST_STATE_NULL );
        gst_object_unref ( playbin );
        playbin = NULL;
    }
}
