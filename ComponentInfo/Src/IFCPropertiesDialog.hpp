#if !defined (IFCPROPERTIESDIALOG_HPP)
#define IFCPROPERTIESDIALOG_HPP

#include "APIEnvir.h"
#include "DGModule.hpp"

class IFCPropertiesDialog : public DG::ModalDialog,
	public DG::ButtonItemObserver,
	public DG::CompoundItemObserver
{
	enum {
		OkButtonId         = 1,
		CancelButtonId     = 2,
		PropertiesEditId   = 3
	};

	DG::Button       okButton;
	DG::Button       cancelButton;
	DG::TextEdit     propertiesEdit;

public:
	IFCPropertiesDialog (GSResModule resModule, short dialogId, GSResModule iconModule);
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;

	void SetPropertiesText (const GS::UniString& propertiesText);
	GS::UniString GetPropertiesText () const;
};

#endif