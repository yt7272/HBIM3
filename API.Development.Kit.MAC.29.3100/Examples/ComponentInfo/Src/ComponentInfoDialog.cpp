#include "ComponentInfoDialog.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "FileTypeManager.hpp"

ComponentInfoDialog::ComponentInfoDialog (GSResModule resModule, short dialogId, GSResModule iconModule) :
	DG::ModalDialog (resModule, dialogId, iconModule),
	okButton (GetReference (), OkButtonId),
	cancelButton (GetReference (), CancelButtonId),
	editId (GetReference (), EditIdId),
	editDesc (GetReference (), EditDescId),
	selectPhotosButton (GetReference (), SelectPhotosId),
	photoCountText (GetReference (), PhotoCountTextId)
{
	AttachToAllItems (*this);
	UpdatePhotoCountLabel ();
}

void ComponentInfoDialog::UpdatePhotoCountLabel ()
{
	GS::UniString t;
	t += "已选 ";
	t += GS::ValueToUniString (selectedPhotoLocations.GetSize ());
	t += " 张";
	photoCountText.SetText (t);
}

void ComponentInfoDialog::DoSelectPhotos ()
{
	DG::FileDialog dlg (DG::FileDialog::OpenMultiFile);
	FTM::FileTypeManager mgr ("ComponentInfoPhotos");
	FTM::FileType typeJpg ("JPEG", "jpg", 0, 0, 0);
	FTM::FileType typePng ("PNG", "png", 0, 0, 0);
	FTM::TypeID idJpg = mgr.AddType (typeJpg);
	FTM::TypeID idPng = mgr.AddType (typePng);
	dlg.AddFilter (idJpg);
	dlg.AddFilter (idPng);
	dlg.SetTitle ("选择照片");
	if (dlg.Invoke ()) {
		selectedPhotoLocations.Clear ();
		USize n = dlg.GetSelectionCount ();
		for (UIndex i = 0; i < n; ++i)
			selectedPhotoLocations.Push (dlg.GetSelectedFile (i));
		UpdatePhotoCountLabel ();
	}
}

void ComponentInfoDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (ev.GetSource () == &okButton)
		PostCloseRequest (DG::ModalDialog::Accept);
	else if (ev.GetSource () == &cancelButton)
		PostCloseRequest (DG::ModalDialog::Cancel);
	else if (ev.GetSource () == &selectPhotosButton)
		DoSelectPhotos ();
}

GS::UniString ComponentInfoDialog::GetComponentId () const
{
	return editId.GetText ();
}

GS::UniString ComponentInfoDialog::GetPhotoDescription () const
{
	return editDesc.GetText ();
}

void ComponentInfoDialog::SetComponentId (const GS::UniString& s)
{
	editId.SetText (s);
}

void ComponentInfoDialog::SetPhotoDescription (const GS::UniString& s)
{
	editDesc.SetText (s);
}

void ComponentInfoDialog::SetSelectedPhotoLocations (const GS::Array<IO::Location>& locs)
{
	selectedPhotoLocations = locs;
	UpdatePhotoCountLabel ();
}
