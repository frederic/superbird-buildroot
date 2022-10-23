#include "Aml_MediaRenderDelegate.h"

NPT_Result  Aml_MediaRenderDelegate::OnGetCurrentConnectionInfo( PLT_ActionReference &action )
{
    if ( !service )
        service = action->GetActionDesc().GetService();
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnNext( PLT_ActionReference &action )
{
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnPause( PLT_ActionReference &action )
{
    if ( gst_player ) {
        gst_player->pause();
    }
    service = action->GetActionDesc().GetService();
    if ( service ) {
        service->SetStateVariable( "TransportState", "PAUSED_PLAYBACK" );
        service->SetStateVariable( "TransportStatus", "OK" );
        service->SetStateVariable( "TransportPlaySpeed", "1" );
    }
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnPlay( PLT_ActionReference &action )
{
    if ( gst_player ) {
        gst_player->start();
    }
    PLT_Service *service = action->GetActionDesc().GetService();
    if ( service ) {
        service->SetStateVariable( "TransportState", "PLAYING" );
        service->SetStateVariable( "TransportStatus", "OK" );
        service->SetStateVariable( "TransportPlaySpeed", "1" );
    }
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnPrevious( PLT_ActionReference &action )
{
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnSeek( PLT_ActionReference &action )
{
    NPT_String value;
    action->GetArgumentValue( "Target", value );
    value = value.SubString( 0, 8 );
    NPT_LOG_WARNING( "onSeek:::::" + value );
    if ( !service ) {
        service = action->GetActionDesc().GetService();
    }
    if ( gst_player ) {
        gst_player->seek( value );
    }
    return NPT_SUCCESS;
}
void Aml_MediaRenderDelegate::setStopStatus()
{
    if ( service ) {
        service->SetStateVariable( "TransportState", "STOPPED" );
        service->SetStateVariable( "TransportStatus", "OK" );
        service->SetStateVariable( "TransportPlaySpeed", "1" );
    }
}
NPT_Result Aml_MediaRenderDelegate::OnStop( PLT_ActionReference &action )
{
    if ( gst_player ) {
        gst_player->stop();
        gst_player->Interrupt();
        gst_player->release();
        gst_player = NULL;
    }
    service = action->GetActionDesc().GetService();
    if ( service ) {
        service->SetStateVariable( "TransportState", "STOPPED" );
        service->SetStateVariable( "TransportStatus", "OK" );
        service->SetStateVariable( "TransportPlaySpeed", "1" );
    }
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnSetAVTransportURI( PLT_ActionReference &action )
{
    NPT_String value;
    if ( gst_player ) {
        gst_player->stop();
        gst_player->Interrupt();
        gst_player->release();
        gst_player = NULL;
    }
    gst_player = new Aml_GstPlayer( this );
    PLT_Service *service = action->GetActionDesc().GetService();
    service->SetStateVariable( "TransportState", "STOPPED" );
    service->SetStateVariable( "TransportStatus", "OK" );
    service->SetStateVariable( "TransportPlaySpeed", "1" );
    action->GetArgumentValue( "CurrentURI", value );
    if ( gst_player ) {
        gst_player->setDataSource( value );
        gst_player->Start();
    }
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnSetPlayMode( PLT_ActionReference &action )
{
    return NPT_SUCCESS;
}
Aml_MediaRenderDelegate::~Aml_MediaRenderDelegate()
{
    if ( service )
        service = NULL;
    if ( gst_player ) {
        gst_player->stop();
        gst_player->Interrupt();
        gst_player->release();
        gst_player = NULL;
    }
}
// RenderingControl
NPT_Result Aml_MediaRenderDelegate::OnSetVolume( PLT_ActionReference &action )
{
    NPT_String channel;
    NPT_String value;

    action->GetArgumentValue( "Channel", channel );
    action->GetArgumentValue( "DesiredVolume" ,value);

    if ( !service ) {
        service = action->GetActionDesc().GetService();
    }
    if ( gst_player ) {
        gst_player->setVolume( value );
    }

    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnSetVolumeDB( PLT_ActionReference &action )
{
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnGetVolumeDBRange( PLT_ActionReference &action )
{
    return NPT_SUCCESS;
}
NPT_Result Aml_MediaRenderDelegate::OnSetMute( PLT_ActionReference &action )
{
    return NPT_SUCCESS;
}
