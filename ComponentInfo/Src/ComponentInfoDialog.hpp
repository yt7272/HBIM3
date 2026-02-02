#if !defined (COMPONENTINFODIALOG_HPP)
#define COMPONENTINFODIALOG_HPP

#include "APIEnvir.h"
#include "DGModule.hpp"
#include "DGFileDialog.hpp"

class ComponentInfoDialog : public DG::ModalDialog,
	public DG::ButtonItemObserver,
	public DG::CompoundItemObserver
{
	enum {
		OkButtonId       = 1,
		CancelButtonId   = 2,
		EditIdId         = 4,
		EditDescId       = 6,
		SelectPhotosId   = 8,
		PhotoCountTextId = 9
	};

	DG::Button    okButton;
	DG::Button    cancelButton;
	DG::TextEdit  editId;
	DG::TextEdit  editDesc;
	DG::Button    selectPhotosButton;
	DG::LeftText  photoCountText;

	GS::Array<IO::Location> selectedPhotoLocations;

	void UpdatePhotoCountLabel ();
	void DoSelectPhotos ();

public:
	ComponentInfoDialog (GSResModule resModule, short dialogId, GSResModule iconModule);
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;

	GS::UniString        GetComponentId () const;
	GS::UniString        GetPhotoDescription () const;
	GS::Array<IO::Location> GetSelectedPhotoLocations () const { return selectedPhotoLocations; }

	void SetComponentId (const GS::UniString& s);
	void SetPhotoDescription (const GS::UniString& s);
	void SetSelectedPhotoLocations (const GS::Array<IO::Location>& locs);
};

#endif
