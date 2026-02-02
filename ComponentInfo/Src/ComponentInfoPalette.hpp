#if !defined (COMPONENTINFOPALETTE_HPP)
#define COMPONENTINFOPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGImage.hpp"
#include "Location.hpp"

namespace ComponentInfo { struct PropertyDefinitionGuids; }

class ComponentInfoPalette : public DG::Palette,
	public DG::PanelObserver,
	public DG::ButtonItemObserver
{
public:
	enum {
		TextComponentIdentifierId = 1,  // 构件标识显示区域
		TextClassificationId = 2,       // 分类标签
		EditCompIdId       = 4,         // 构件编号编辑框 (ID 3是标签，不需要)
		EditPhotoDescId    = 6,         // 照片说明编辑框 (ID 5是标签，不需要)
		TextPhotoPathId    = 8,         // 照片位置值显示 (ID 7是标签，不需要)
		PictureItemId      = 9,         // 图片控件
		BtnPrevPhotoId     = 10,        // "<" button
		BtnDeletePhotoId   = 11,        // "删除" button
		TextPhotoIndexId   = 12,        // Index text
		BtnNextPhotoId     = 13,        // ">" button
		BtnViewPhotosId    = 14,        // "查看照片" button
		BtnEditInfoId      = 15,        // "填写信息" button
		BtnSelectPhotosId  = 16,        // "选择照片" button
		BtnSaveId          = 17,        // "保存" button
		BtnCancelId        = 18,        // "取消" button
		BtnViewIFCPropertiesId = 19
	};

	static const short PaletteResId = 32520;

	static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);
	static GSErrCode SelectionChangeHandler (const API_Neig* selElemNeig);
	static GSErrCode RegisterPaletteControlCallBack ();
	static Int32 GetPaletteReferenceId ();

	static bool HasInstance ();
	static void CreateInstance ();
	static ComponentInfoPalette& GetInstance ();
	static void DestroyInstance ();

	void Show ();
	void Hide ();
	void UpdateFromSelection ();
	static bool IsInEditMode (); // 检查是否处于编辑模式

	virtual ~ComponentInfoPalette ();

private:
	DG::LeftText     textComponentIdentifier;
	DG::LeftText     textClassification;
	DG::TextEdit     editCompId;
	DG::TextEdit     editPhotoDesc;
	DG::LeftText     textPhotoPaths;
	DG::Button       btnViewPhotos;
	DG::PictureItem  pictureItem;
	DG::Button       btnPrevPhoto;
	DG::Button       btnDeletePhoto;
	DG::CenterText   textPhotoIndex;
	DG::Button       btnNextPhoto;
	DG::Button       btnEditInfo;
	DG::Button       btnSelectPhotos;
	DG::Button       btnSave;
	DG::Button       btnCancel;
	DG::Button       btnViewIFCProperties;

	API_Guid currentElemGuid;
	GS::UniString originalCompId;
	GS::UniString originalPhotoDesc;
	GS::Array<GS::UniString> currentPhotoRelPaths;
	GS::Array<GS::UniString> deletedPhotoRelPaths;
	UInt32 currentPhotoIndex;
	bool isEditMode;
	GS::Array<IO::Location> pendingPhotoLocations;

	void RefreshContent ();
	void SetMenuItemCheckedState (bool checked);
	void DoViewPhotos ();
	void ShowAllPhotos ();
	void ShowPhotoAtIndex (UInt32 index);
	void UpdatePhotoIndexText ();
	void DoPrevPhoto ();
	void DoNextPhoto ();
	void DoDeletePhoto ();
	void EnterEditMode ();
	void ExitEditMode (bool save);
	void DoSelectPhotos ();
	void DoSave ();
	void UpdateEditModeUI ();
	void ShowPendingPhotoPreview (UInt32 index);
	bool HasUnsavedChanges () const;
	void DoViewIFCProperties ();

	ComponentInfoPalette ();

	virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif
