#if !defined (MERGEDPLUGINPALETTE_HPP)
#define MERGEDPLUGINPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGBrowser.hpp"
#include "Location.hpp"

namespace MergedPlugin { struct PropertyDefinitionGuids; }

class MergedPluginPalette : public DG::Palette,
	public DG::PanelObserver
{
public:
	// 与 IFCPropertyReader 一致：BrowserId=1，GDLG 中唯一控件为 Browser 0 0 450 150
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
	static MergedPluginPalette& GetInstance ();
	static void DestroyInstance ();

	void Show ();
	void Hide ();
	void UpdateFromSelection ();
	static bool IsInEditMode (); // 检查是否处于编辑模式

	virtual ~MergedPluginPalette ();

private:
	DG::Browser      browser;

	API_Guid currentElemGuid;
	GS::UniString originalCompId;
	GS::UniString originalPhotoDesc;
	GS::Array<GS::UniString> currentPhotoRelPaths;
	GS::Array<GS::UniString> deletedPhotoRelPaths;

	bool isEditMode;


	void RefreshContent ();
	void SetMenuItemCheckedState (bool checked);




	void DoSave ();
	void UpdateEditModeUI ();

	bool HasUnsavedChanges () const;

	void InitializeJavaScriptBridge ();

	MergedPluginPalette ();

	virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
	virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
};

#endif