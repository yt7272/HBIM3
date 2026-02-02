#ifndef GS_APITOOLS_APIDEVKITEXAMPLES_EXAMPLERUNNER_HPP
#define GS_APITOOLS_APIDEVKITEXAMPLES_EXAMPLERUNNER_HPP

#include "APIdefs_Interface.h"
#include "ACAPI/PropertyOperationManager.hpp"
#include "DGButton.hpp"
#include "DGDialog.hpp"
#include "DGPanel.hpp"
#include "DGStaticItem.hpp"
#include "Definitions.hpp"
#include "GSGuid.hpp"
#include <memory>

/**********************************************************************************************************************/

namespace APIDevKitExamples::PropertyRegistration {


class ExampleRunner : public DG::Palette, public DG::PanelObserver, public DG::ButtonItemObserver
{

private:

	static const GS::Guid s_paletteGuid;
	static const GS::Guid s_propertyGroup1Guid;
	static const GS::Guid s_propertyGroup2Guid;
	static const GS::Guid s_property1_1Guid;
	static const GS::Guid s_property1_2Guid;
	static const GS::Guid s_property2_1Guid;
	static const GS::Guid s_property2_2Guid;
	static const GS::Guid s_property2_3Guid;

	static std::unique_ptr<ExampleRunner> s_instance;

private:

	enum {
		RegisterButtonId = 1,
		UnregisterButtonId = 2,
		RegisterStateTextId = 3,
	};

	DG::Button m_registerButton;
	DG::Button m_unregisterButton;
	DG::LeftText m_registerStateText;
	ACAPI::PropertyOperationManager m_propertyOperationManager;

public:

	[[nodiscard]] static GSErrCode RegisterPaletteControlCallBack ();

	static void CreateInstance ();
	static void DestroyInstance ();
	static void Toggle ();

private:

	[[nodiscard]] static bool HaveInstance ();
	[[nodiscard]] static ExampleRunner&	GetInstance ();

	[[nodiscard]] static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);

public:

	ExampleRunner ();
	virtual ~ExampleRunner ();

	void PanelResized (const DG::PanelResizeEvent& ev) override;
	void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	void ButtonClicked (const DG::ButtonClickEvent& ev) override;

private:

	void Show ();
	void Hide ();
	void SetMenuItemCheckedState (bool checked);
	
	void SetupProperties ();
	void Register ();
	void Unregister ();

};


}

/**********************************************************************************************************************/

#endif
