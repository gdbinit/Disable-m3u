/*
 * File:       iTunesPlugIn.cpp
 *
 * Abstract:   Visual plug-in for iTunes.  Cross-platform code.
 *
 * Version:    2.0
 *
 */

//-------------------------------------------------------------------------------------------------
//	includes
//-------------------------------------------------------------------------------------------------

#include "iTunesPlugIn.h"
#include "hack.h"

extern uint8_t patchactivated;

//-------------------------------------------------------------------------------------------------
// ProcessRenderData
//-------------------------------------------------------------------------------------------------
//
void ProcessRenderData( VisualPluginData * visualPluginData, UInt32 timeStampID, const RenderVisualData * renderData )
{
}

//-------------------------------------------------------------------------------------------------
//	ResetRenderData
//-------------------------------------------------------------------------------------------------
//
void ResetRenderData( VisualPluginData * visualPluginData )
{
	memset( &visualPluginData->renderData, 0, sizeof(visualPluginData->renderData) );
	memset( visualPluginData->minLevel, 0, sizeof(visualPluginData->minLevel) );
}

//-------------------------------------------------------------------------------------------------
//	UpdateInfoTimeOut
//-------------------------------------------------------------------------------------------------
//
void UpdateInfoTimeOut( VisualPluginData * visualPluginData )
{
	// reset the timeout value we will use to show the info/artwork if we have it during DrawVisual()
	visualPluginData->drawInfoTimeOut = time( NULL ) + kInfoTimeOutInSeconds;
}

//-------------------------------------------------------------------------------------------------
//	UpdatePulseRate
//-------------------------------------------------------------------------------------------------
//
void UpdatePulseRate( VisualPluginData * visualPluginData, UInt32 * ioPulseRate )
{
	// vary the pulse rate based on whether or not iTunes is currently playing
	if ( visualPluginData->playing )
		*ioPulseRate = kPlayingPulseRateInHz;
	else
		*ioPulseRate = kStoppedPulseRateInHz;
}

//-------------------------------------------------------------------------------------------------
//	UpdateTrackInfo
//-------------------------------------------------------------------------------------------------
//
/*
void UpdateTrackInfo( VisualPluginData * visualPluginData, ITTrackInfo * trackInfo, ITStreamInfo * streamInfo )
{
	if ( trackInfo != NULL )
		visualPluginData->trackInfo = *trackInfo;
	else
		memset( &visualPluginData->trackInfo, 0, sizeof(visualPluginData->trackInfo) );
	
	if ( streamInfo != NULL )
		visualPluginData->streamInfo = *streamInfo;
	else
		memset( &visualPluginData->streamInfo, 0, sizeof(visualPluginData->streamInfo) );
	
	UpdateInfoTimeOut( visualPluginData );
}
*/
//-------------------------------------------------------------------------------------------------
//	RequestArtwork
//-------------------------------------------------------------------------------------------------
//
/*
static void RequestArtwork( VisualPluginData * visualPluginData )
{
	// only request artwork if this plugin is active
	if ( visualPluginData->destView != NULL )
	{
		OSStatus		status;
		
		status = PlayerRequestCurrentTrackCoverArt( visualPluginData->appCookie, visualPluginData->appProc );
	}
}
*/
//-------------------------------------------------------------------------------------------------
//	PulseVisual
//-------------------------------------------------------------------------------------------------
//
void PulseVisual( VisualPluginData * visualPluginData, UInt32 timeStampID, const RenderVisualData * renderData, UInt32 * ioPulseRate )
{
	/*
	// update internal state
	ProcessRenderData( visualPluginData, timeStampID, renderData );
	
	// if desired, adjust the pulse rate
	UpdatePulseRate( visualPluginData, ioPulseRate );
	 */
}

//-------------------------------------------------------------------------------------------------
//	VisualPluginHandler
//-------------------------------------------------------------------------------------------------
//
static OSStatus VisualPluginHandler(OSType message,VisualPluginMessageInfo *messageInfo,void *refCon)
{
	OSStatus			status;
	VisualPluginData *	visualPluginData;
	
	visualPluginData = (VisualPluginData*) refCon;
	
	status = noErr;
	
	switch ( message )
	{
			/*
			 Sent when the visual plugin is registered.  The plugin should do minimal
			 memory allocations here.
			 */		
		case kVisualPluginInitMessage:
		{
			// get a send right
			mach_port_t myself = mach_task_self();
			uint64_t imageaddress;
			uint64_t imagesize;
			
			find_image(myself, &imageaddress, &imagesize);
			find_patchaddresses(myself, imageaddress, imagesize);
			
			patchmemory();
			patchactivated = 1;
			
			break;
		}
			/*
			 Sent when the visual plugin is unloaded.
			 */		
		case kVisualPluginCleanupMessage:
		{
			if ( visualPluginData != NULL )
				free( visualPluginData );
			break;
		}
			/*
			 Sent when the visual plugin is enabled/disabled.  iTunes currently enables all
			 loaded visual plugins at launch.  The plugin should not do anything here.
			 */
		case kVisualPluginEnableMessage:
		case kVisualPluginDisableMessage:
		{
			break;
		}
			/*
			 Sent if the plugin requests idle messages.  Do this by setting the kVisualWantsIdleMessages
			 option in the RegisterVisualMessage.options field.
			 
			 DO NOT DRAW in this routine.  It is for updating internal state only.
			 */
		case kVisualPluginIdleMessage:
		{
			break;
		}			
			/*
			 Sent if the plugin requests the ability for the user to configure it.  Do this by setting
			 the kVisualWantsConfigure option in the RegisterVisualMessage.options field.
			 */
		case kVisualPluginConfigureMessage:
		{
			// unpatch if we call plugin configuration
			unpatchmemory();
			break;
		}
			/*
			 Sent when iTunes is going to show the visual plugin.  At this
			 point, the plugin should allocate any large buffers it needs.
			 */
		case kVisualPluginActivateMessage:
		{
			// verify if it was already patched, if not patch it
			if (patchactivated != 1)
				patchmemory();
			
			break;
		}	
			/*
			 Sent when this visual is no longer displayed.
			 */
		case kVisualPluginDeactivateMessage:
		{
			break;
		}
			/*
			 Sent when iTunes is moving the destination view to a new parent window (e.g. to/from fullscreen).
			 */
		case kVisualPluginWindowChangedMessage:
		{
			break;
		}
			/*
			 Sent when iTunes has changed the rectangle of the currently displayed visual.
			 
			 Note: for custom NSView subviews, the subview's frame is automatically resized.
			 */
		case kVisualPluginFrameChangedMessage:
		{
			break;
		}
			/*
			 Sent for the visual plugin to update its internal animation state.
			 Plugins are allowed to draw at this time but it is more efficient if they
			 wait until the kVisualPluginDrawMessage is sent OR they simply invalidate
			 their own subview.  The pulse message can be sent faster than the system
			 will allow drawing to support spectral analysis-type plugins but drawing
			 will be limited to the system refresh rate.
			 */
		case kVisualPluginPulseMessage:
		{
			break;
		}
			/*
			 It's time for the plugin to draw a new frame.
			 
			 For plugins using custom subviews, you should ignore this message and just
			 draw in your view's draw method.  It will never be called if your subview 
			 is set up properly.
			 */
		case kVisualPluginDrawMessage:
		{
			break;
		}
			/*
			 Sent when the player starts.
			 */
		case kVisualPluginPlayMessage:
		{
			break;
		}
			/*
			 Sent when the player changes the current track information.  This
			 is used when the information about a track changes.
			 */
		case kVisualPluginChangeTrackMessage:
		{
			break;
		}
			/*
			 Artwork for the currently playing song is being delivered per a previous request.
			 
			 Note that NULL for messageInfo->u.coverArtMessage.coverArt means the currently playing song has no artwork.
			 */
		case kVisualPluginCoverArtMessage:
		{
			break;
		}
			/*
			 Sent when the player stops or pauses.
			 */
		case kVisualPluginStopMessage:
		{
			break;
		}
			/*
			 Sent when the player changes the playback position.
			 */
		case kVisualPluginSetPositionMessage:
		{
			break;
		}
		default:
		{
			status = unimpErr;
			break;
		}
	}
	
	return status;	
}

//-------------------------------------------------------------------------------------------------
//	RegisterVisualPlugin
//-------------------------------------------------------------------------------------------------
//
OSStatus RegisterVisualPlugin( PluginMessageInfo * messageInfo )
{
	PlayerMessageInfo	playerMessageInfo;
	OSStatus			status;
	
	memset( &playerMessageInfo.u.registerVisualPluginMessage, 0, sizeof(playerMessageInfo.u.registerVisualPluginMessage) );
	
	GetVisualName( playerMessageInfo.u.registerVisualPluginMessage.name );
	
	SetNumVersion( &playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kTVisualPluginMajorVersion, kTVisualPluginMinorVersion, kTVisualPluginReleaseStage, kTVisualPluginNonFinalRelease );
	
	playerMessageInfo.u.registerVisualPluginMessage.options					= GetVisualOptions();
	playerMessageInfo.u.registerVisualPluginMessage.handler					= (VisualPluginProcPtr)VisualPluginHandler;
	playerMessageInfo.u.registerVisualPluginMessage.registerRefCon			= 0;
	playerMessageInfo.u.registerVisualPluginMessage.creator					= kTVisualPluginCreator;
	
	playerMessageInfo.u.registerVisualPluginMessage.pulseRateInHz			= kStoppedPulseRateInHz;	// update my state N times a second
	playerMessageInfo.u.registerVisualPluginMessage.numWaveformChannels		= 0;
	playerMessageInfo.u.registerVisualPluginMessage.numSpectrumChannels		= 0;
	
	playerMessageInfo.u.registerVisualPluginMessage.minWidth				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.minHeight				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.maxWidth				= 0;	// no max width limit
	playerMessageInfo.u.registerVisualPluginMessage.maxHeight				= 0;	// no max height limit
	
	status = PlayerRegisterVisualPlugin( messageInfo->u.initMessage.appCookie, messageInfo->u.initMessage.appProc, &playerMessageInfo );
	
	return status;
}

