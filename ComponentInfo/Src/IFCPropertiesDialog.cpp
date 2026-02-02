#include "IFCPropertiesDialog.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"

IFCPropertiesDialog::IFCPropertiesDialog (GSResModule resModule, short dialogId, GSResModule iconModule) :
	DG::ModalDialog (resModule, dialogId, iconModule),
	okButton (GetReference (), OkButtonId),
	cancelButton (GetReference (), CancelButtonId),
	propertiesEdit (GetReference (), PropertiesEditId)
{
	AttachToAllItems (*this);
	propertiesEdit.Disable (); // 设为只读，仅用于显示
}

void IFCPropertiesDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (ev.GetSource () == &okButton)
		PostCloseRequest (DG::ModalDialog::Accept);
	else if (ev.GetSource () == &cancelButton)
		PostCloseRequest (DG::ModalDialog::Cancel);
}

void IFCPropertiesDialog::SetPropertiesText (const GS::UniString& propertiesText)
{
	propertiesEdit.SetText (propertiesText);
}

GS::UniString IFCPropertiesDialog::GetPropertiesText () const
{
	return propertiesEdit.GetText ();
}