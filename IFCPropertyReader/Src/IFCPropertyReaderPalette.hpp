// *****************************************************************************
// Header file for IFCPropertyReaderPalette class
// *****************************************************************************

#ifndef IFCPROPERTYREADERPALETTE_HPP
#define IFCPROPERTYREADERPALETTE_HPP

// ---------------------------------- Includes ---------------------------------

#include "APIEnvir.h"
#include "ACAPinc.h"		// also includes APIdefs.h
#include "DGModule.hpp"
#include "DGBrowser.hpp"

#define IFCPropertyReaderPaletteResId 32500
#define IFCPropertyReaderPaletteMenuResId 32500
#define IFCPropertyReaderPaletteMenuItemIndex 1

// --- Class declaration: IFCPropertyReaderPalette ------------------------------------------

class IFCPropertyReaderPalette final : public DG::Palette,
							 public DG::PanelObserver
{
public:
	enum SelectionModification { RemoveFromSelection, AddToSelection };

	struct ElementInfo {
		GS::UniString	guidStr;
		GS::UniString	typeName;
		GS::UniString	elemID;
	};

protected:
	enum {
		BrowserId = 1
	};

	DG::Browser		browser;

	void InitBrowserControl ();
	void RegisterACAPIJavaScriptObject ();
	void UpdateSelectedElementsOnHTML ();
	void SetMenuItemCheckedState (bool);

	virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
	virtual	void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;

	static GS::Array<IFCPropertyReaderPalette::ElementInfo> GetSelectedElements ();
	static void ModifySelection (const GS::UniString& elemGuidStr, SelectionModification modification);

	static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);

	static GS::Ref<IFCPropertyReaderPalette> instance;

	IFCPropertyReaderPalette ();

public:
	virtual ~IFCPropertyReaderPalette ();

	static bool				HasInstance ();
	static void				CreateInstance ();
	static IFCPropertyReaderPalette&	GetInstance ();
	static void				DestroyInstance ();

	void Show ();
	void Hide ();

	void UpdateIFCPropertiesOnHTML ();

	static GSErrCode				RegisterPaletteControlCallBack ();
	static GSErrCode SelectionChangeHandler (const API_Neig*);
};

#endif // IFCPROPERTYREADERPALETTE_HPP
