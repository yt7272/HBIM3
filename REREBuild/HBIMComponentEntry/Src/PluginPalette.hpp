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
		TitleLabelId = 1,
		UnderlineId = 2,
		TypeLabelId = 3,
		TypeValueId = 4,
		IdLabelId = 5,
		IdValueId = 6,
		HBIMSeparatorId = 7,
		HBIMTitleId = 8,
		HBIMTitleUnderlineId = 9,
		HBIMIdLabelId = 10,
		HBIMIdValueId = 11,
		HBIMDescLabelId = 12,
		HBIMDescValueId = 13,
		HBIMActionButtonId = 14,
		HBIMCancelButtonId = 15,
		ImageSeparatorId = 16,
		ImageTitleId = 17,
		ImageTitleUnderlineId = 18,
		ImageCountLabelId = 19,
		ImageCountValueId = 20,
		ImageCurrentLabelId = 21,
		ImageCurrentValueId = 22,
		ImageSelectButtonId = 23,
		ImageDeleteButtonId = 24,
		ImagePrevButtonId = 25,
		ImageNextButtonId = 26,
		ImagePreviewId = 27,
		ImageOKButtonId = 28,
		ImageCancelButtonId = 29
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
	DG::CenterText titleLabel;
	DG::LeftText underlineLabel;
	DG::LeftText typeLabel;
	DG::LeftText typeValue;
	DG::LeftText idLabel;
	DG::LeftText idValue;
	
	// HBIM属性信息控件
	DG::LeftText hbimSeparator;
	DG::CenterText hbimTitle;
	DG::LeftText hbimTitleUnderline;
	DG::LeftText hbimIdLabel;
	DG::TextEdit hbimIdValue;
	DG::LeftText hbimDescLabel;
	DG::TextEdit hbimDescValue;
	DG::Button hbimActionButton;
	DG::Button hbimCancelButton;
	
	// HBIM图片控件
	DG::LeftText imageSeparator;
	DG::CenterText imageTitle;
	DG::LeftText imageTitleUnderline;
	DG::LeftText imageCountLabel;
	DG::LeftText imageCountValue;
	DG::LeftText imageCurrentLabel;
	DG::LeftText imageCurrentValue;
	DG::Button imageSelectButton;
	DG::Button imageDeleteButton;
	DG::Button imagePrevButton;
	DG::Button imageNextButton;
	DG::Button imageOKButton;
	DG::Button imageCancelButton;
	DG::LeftText imagePreview;
	
	// HBIM属性状态
	bool hasHBIMProperties;
	bool isHBIMEditMode;
	GS::UniString originalHBIMId;
	GS::UniString originalHBIMDesc;
	API_Guid hbimGroupGuid;
	API_Guid hbimIdGuid;
	API_Guid hbimDescGuid;
	API_Guid currentElemGuid;
	bool isReSelectingElement;
	
	// HBIM图片状态
	bool hasHBIMImages;
	bool isImageEditMode;
	GS::Array<GS::UniString> imagePaths;
	GS::Array<GS::UniString> originalImagePaths; // 用于取消编辑时恢复
	UInt32 currentImageIndex;
	GS::UniString projectHash;

	// HBIM属性管理函数
	void UpdateHBIMUI ();
	void EnterHBIMEditMode ();
	void ExitHBIMEditMode (bool save);
	void CheckHBIMProperties (const API_Guid& elementGuid);
	void ReadHBIMProperties (const API_Guid& elementGuid);
	void WriteHBIMProperties (const API_Guid& elementGuid);
	
 	// HBIM属性初始化
 	GSErrCode EnsureHBIMPropertiesInitialized ();
 	bool TryFindExistingHBIMPropertyGroupAndDefinitions ();
 	
  	// HBIM图片管理函数
  	void UpdateHBIMImageUI ();
  	void CheckHBIMImages ();
  	void SelectHBIMImages ();
  	void DeleteCurrentHBIMImage ();
  	void NavigateHBIMImage (bool forward);
  	void EnterImageEditMode ();
  	void ExitImageEditMode (bool save);
  	GSErrCode EnsureHBIMImageFolder ();
  	GS::UniString CalculateProjectHash ();
  	bool IsProjectSaved ();
	
	void SetMenuItemCheckedState (bool checked);

	PluginPalette ();

	virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif