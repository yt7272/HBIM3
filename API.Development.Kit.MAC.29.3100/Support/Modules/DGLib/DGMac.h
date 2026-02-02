// *********************************************************************************************************************
// File:			DGMac.h
//
// Description:		Macintosh only external functions
//
// Project:			GRAPHISOFT Dialog Manager (DGLib)
//
// Contact person:	HG, BM
//
// SG compatible
// *********************************************************************************************************************

#ifndef DGMAC_H
#define	DGMAC_H

#pragma once


// --- Includes --------------------------------------------------------------------------------------------------------

#include "DG.h"
#include "DGUtility.hpp"  // DG::Rect !!!

#include <ApplicationServices/ApplicationServices.h>

// --- Drawing Item callback interface ---------------------------------------------------------------------------------

class DG_DLL_EXPORT DrawingItemCallback
{
public:
	virtual ~DrawingItemCallback ();
	virtual void		Recompose (void*) = 0;
	virtual bool		IsInRecompose (void*) = 0;
};


// --- Exported Functions ----------------------------------------------------------------------------------------------

DG_DLL_EXPORT void CCALL		DGSetHideOnSuspendState (bool state);

DG_DLL_EXPORT bool CCALL		DGIsHideOnSuspendEnabled (void);

DG_DLL_EXPORT void CCALL		DGFlushDialogToScreen (short dialId);

DG_DLL_EXPORT void CCALL		DGSetExcludePaletteFromCountingWorkRect (short dialId, bool isExcluded);

DG_DLL_EXPORT void CCALL		DGUserItemSetHasDrawingItem (short dialId, short item);

DG_DLL_EXPORT OSStatus CCALL	DGCreateNewOverlayWindow (short dialId, short item, void** nsWindow);

DG_DLL_EXPORT OSStatus CCALL	DGDisposeOverlayWindow (short dialId, short item, void* nsWindow);

DG_DLL_EXPORT OSStatus CCALL	DGShowOverlayWindow (short dialId, short item, void* nsWindow, bool show);

DG_DLL_EXPORT OSStatus CCALL	DGGetWindowContext (short dialId, short item, void* nsWindow, CGContextRef* context);

DG_DLL_EXPORT OSStatus CCALL	DGReleaseWindowContext (short dialId, short item, void* nsWindow, CGContextRef context, bool anyDrawn = true);

DG_DLL_EXPORT void CCALL		DGSetWindowAlphaValue (short dialId, short item, void* nsWindow, double alphaValue);

DG_DLL_EXPORT bool CCALL		DGIsWindowVisible (short dialId, short item, void* nsWindow);

DG_DLL_EXPORT void CCALL		DGEnableScreenUpdates (void);

DG_DLL_EXPORT void CCALL		DGDisableScreenUpdates (void);

DG_DLL_EXPORT void CCALL		DGDrawingItemScroll (short dialId, short item, Int32 horizontalScroll, Int32 verticalScroll, const CGImageRef verticalImage, const CGImageRef horizontalImage);

DG_DLL_EXPORT bool CCALL		DGDrawingItemIsValid (short dialId, short item);

DG_DLL_EXPORT short CCALL		DGExtendedPrintDialog (GSResModule dialResModule, short resId, GSResModule dialIconResModule, const DGDialCallBack dCallBack, DGUserData userData);

DG_DLL_EXPORT void CCALL		DGSendDummyEvent (void);

DG_DLL_EXPORT void CCALL		DGGetContextMenu (char* menuItems);

DG_DLL_EXPORT void CCALL		DGSetDrawingItemCallbackInterface (short dialId, short item, DrawingItemCallback* drawingItemCallback);
#endif
