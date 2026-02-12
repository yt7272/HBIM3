#if !defined (PLUGINPALETTE_HPP)
#define PLUGINPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGBrowser.hpp"
#include "Location.hpp"

class PluginPalette : public DG::Palette,
	public DG::PanelObserver
{
public:
	enum {
		BrowserId          = 1
	};

	struct ElementInfo {
		GS::UniString	guidStr;
		GS::UniString	typeName;
		GS::UniString	elemID;
	};

	static const short PaletteResId = 32520;

	static GS::Array<ElementInfo> GetSelectedElements ();
	static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);
	static GSErrCode SelectionChangeHandler (const API_Neig* selElemNeig);
	static GSErrCode RegisterPaletteControlCallBack ();
	static Int32 GetPaletteReferenceId ();

	static bool HasInstance ();
	static void CreateInstance ();
	static PluginPalette& GetInstance ();
	static void DestroyInstance ();

	void Show ();
	void Hide ();
	void UpdateFromSelection ();
	static bool IsInEditMode ();
	static void SetEditMode (bool editMode);

	virtual ~PluginPalette ();

private:
	DG::Browser      browser;

	API_Guid currentElemGuid;
	bool isEditMode;
	bool hasUnsavedChanges;
	bool selectionDisabled;

	void RefreshContent ();
	void SetMenuItemCheckedState (bool checked);

	void DoSave ();
	void UpdateEditModeUI ();

	bool HasUnsavedChanges () const;

	void InitializeJavaScriptBridge ();

	PluginPalette ();

	virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
	virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
};

#endif