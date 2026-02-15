#if !defined (PLUGINPALETTE_HPP)
#define PLUGINPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"

class PluginPalette : public DG::Palette,
	public DG::PanelObserver,
	public DG::ButtonItemObserver
{
public:
	static const short PaletteResId = 32520;
	
	enum {
		TitleLabelId = 1
	};

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
	DG::LeftText titleLabel;

	void SetMenuItemCheckedState (bool checked);

	PluginPalette ();

	virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif