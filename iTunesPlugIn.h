/*
 * File:       iTunesPlugIn.h
 *
 * Abstract:   Visual plug-in for iTunes
 *
 * Version:    2.0
 *
 */

#ifndef ITUNESPLUGIN_H
#define ITUNESPLUGIN_H

#include "iTunesVisualAPI.h"
#include <time.h>

//-------------------------------------------------------------------------------------------------
//	build flags
//-------------------------------------------------------------------------------------------------
#define USE_SUBVIEW						(TARGET_OS_MAC && 1)		// use a custom NSView subview on the Mac

//-------------------------------------------------------------------------------------------------
//	typedefs, structs, enums, etc.
//-------------------------------------------------------------------------------------------------

#define	kTVisualPluginCreator			'hook'

#define	kTVisualPluginMajorVersion		1
#define	kTVisualPluginMinorVersion		0
#define	kTVisualPluginReleaseStage		developStage
#define	kTVisualPluginNonFinalRelease	0

struct VisualPluginData;

#if TARGET_OS_MAC
#import <Cocoa/Cocoa.h>

// "namespace" our ObjC classname to avoid load conflicts between multiple visualizer plugins
#define VisualView		ComAppleExample_VisualView
#define GLVisualView	ComAppleExample_GLVisualView

@class VisualView;
@class GLVisualView;

#endif

#define kInfoTimeOutInSeconds		10							// draw info/artwork for N seconds when it changes or playback starts
#define kPlayingPulseRateInHz		1							// when iTunes is playing, draw N times a second
#define kStoppedPulseRateInHz		1							// when iTunes is not playing, draw N times a second

struct VisualPluginData
{
	void *				appCookie;
	ITAppProcPtr		appProc;
	
#if TARGET_OS_MAC
	NSView*				destView;
	NSRect				destRect;
#if USE_SUBVIEW
	VisualView*			subview;								// custom subview
#endif
	NSImage *			currentArtwork;
#else
	HWND				destView;
	RECT				destRect;
	Gdiplus::Bitmap* 	currentArtwork;
	long int			lastDrawTime;
#endif
	OptionBits			destOptions;
	
	RenderVisualData	renderData;
	UInt32				renderTimeStampID;
	
	ITTrackInfo			trackInfo;
	ITStreamInfo		streamInfo;
	
	// Plugin-specific data
	
	Boolean				playing;								// is iTunes currently playing audio?
	Boolean				padding[3];
	
	time_t				drawInfoTimeOut;						// when should we stop showing info/artwork?
	
	UInt8				minLevel[kVisualMaxDataChannels];		// 0-128
	UInt8				maxLevel[kVisualMaxDataChannels];		// 0-128
};
typedef struct VisualPluginData VisualPluginData;

void		GetVisualName( ITUniStr255 name );
OptionBits	GetVisualOptions( void );
OSStatus	RegisterVisualPlugin( PluginMessageInfo * messageInfo );

OSStatus	ActivateVisual( VisualPluginData * visualPluginData, VISUAL_PLATFORM_VIEW destView, OptionBits options );
OSStatus	MoveVisual( VisualPluginData * visualPluginData, OptionBits newOptions );
OSStatus	DeactivateVisual( VisualPluginData * visualPluginData );
OSStatus	ResizeVisual( VisualPluginData * visualPluginData );

void		ProcessRenderData( VisualPluginData * visualPluginData, UInt32 timeStampID, const RenderVisualData * renderData );
void		ResetRenderData( VisualPluginData * visualPluginData );
void		UpdateInfoTimeOut( VisualPluginData * visualPluginData );
void		UpdateTrackInfo( VisualPluginData * visualPluginData, ITTrackInfo * trackInfo, ITStreamInfo * streamInfo );
void		UpdateArtwork( VisualPluginData * visualPluginData, VISUAL_PLATFORM_DATA coverArt, UInt32 coverArtSize, UInt32 coverArtFormat );
void		UpdatePulseRate( VisualPluginData * visualPluginData, UInt32 * ioPulseRate );

void		DrawVisual( VisualPluginData * visualPluginData );
void		PulseVisual( VisualPluginData * visualPluginData, UInt32 timeStampID, const RenderVisualData * renderData, UInt32 * ioPulseRate );
void		InvalidateVisual( VisualPluginData * visualPluginData );

OSStatus	ConfigureVisual( VisualPluginData * visualPluginData );

#endif
