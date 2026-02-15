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
	struct ElementInfo {
		GS::UniString	guidStr;
		GS::UniString	typeName;
		GS::UniString	elemID;
	};

	static const short PaletteResId = 32520;
	
	// DG control IDs
	enum {
		TitleLabelId = 1,
		GuidLabelId = 2,
		TypeLabelId = 3,
		InfoTitleLabelId = 4,
		CompIdLabelId = 5,
		ComponentIdEditId = 6,
		PhotoDescLabelId = 7,
		PhotoDescEditId = 8,
		PhotoPreviewTitleLabelId = 9,
		PhotoPreviewLabelId = 10,
		EditInfoButtonId = 11,
		SelectPhotoButtonId = 12,
		ViewPhotoButtonId = 13,
		SaveButtonId = 14,
		CancelButtonId = 15
	};

	static GS::Array<ElementInfo> GetSelectedElements ();
	static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);
	static GSErrCode SelectionChangeHandler (const API_Neig* selElemNeig);
	static GSErrCode RegisterPaletteControlCallBack ();
	static Int32 GetPaletteReferenceId ();

	// Singleton pattern methods
	static bool HasInstance ();
	static void CreateInstance ();
	static PluginPalette& GetInstance ();
	static void DestroyInstance ();

	// Version number method
	static GS::UniString GetVersionString ();

	void Show ();
	void Hide ();
	void UpdateFromSelection ();
	static bool IsInEditMode ();
	static void SetEditMode (bool editMode);

	virtual ~PluginPalette ();

private:
	API_Guid currentElemGuid;
	bool isEditMode;
	bool hasUnsavedChanges;
	GS::Array<IO::Location> pendingPhotoLocations;
	
	// DG controls
	DG::LeftText titleLabel;
	DG::LeftText guidLabel;
	DG::LeftText typeLabel;
	DG::LeftText infoTitleLabel;
	DG::LeftText compIdLabel;
	DG::TextEdit componentIdEdit;
	DG::LeftText photoDescLabel;
	DG::TextEdit photoDescEdit;
	DG::LeftText photoPreviewTitleLabel;
	DG::LeftText photoPreviewLabel;
	DG::Button editInfoButton;
	DG::Button selectPhotoButton;
	DG::Button viewPhotoButton;
	DG::Button saveButton;
	DG::Button cancelButton;

	void RefreshContent ();
	void SetMenuItemCheckedState (bool checked);

	void DoSave ();
	void DoSelectPhotos ();
	void DoViewPhotos ();
	void UpdateEditModeUI ();

	bool HasUnsavedChanges () const;

	PluginPalette ();

	virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif