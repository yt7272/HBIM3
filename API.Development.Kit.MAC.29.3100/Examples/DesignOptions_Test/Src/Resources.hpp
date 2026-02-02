#if !defined (RESOURCES_HPP)
#define RESOURCES_HPP

#define	DESIGNOPTIONS_TEST_ADDON_NAME			32000	//'STR#' 32000
#define	DESIGNOPTIONS_TEST_MENU_STRINGS			32500	//'STR#' 32500
#define	DESIGNOPTIONS_TEST_MENU_PROMPT_STRINGS	32600	//'STR#' 32600

enum MenuItem {
	DumpViewMapDesignOptionCombinationsID = 1,
	SetAllDesignOptionCombinationsToMainModelOnlyID = 2,
	DumpDesignOptionCombinationsID = 3,
	DumpDesignOptionRelatedInfoForSelectedElementsPropertyBasedID = 4,
	DumpDesignOptionRelatedInfoForSelectedElementsID = 5,
	ModifyDesignOptionsNameID = 6,
	ModifyDesignOptionSetsNameID = 7,
	CreateDesignOptionID = 8,
	DeleteTheLastDesignOptionFromTheFirstNonEmptySetID = 9,
	MoveElemsToDesignOptionID = 10,
	MoveElemsToMainModelID = 11,
	GetHotlinkDesignOptionTableID = 12,
	GetHotlinkInstanceDesignOptionTableID = 13,
	SetHotlinkDesignOptionTableID = 14,
	MoveTheFirstDesignOptionToTheEndOfTheLastSetID = 15,
	MoveTheLastDesignOptionOfTheFirstSetToTheFrontID = 16,
	CreateDesignOptionSetID = 17,
	DumpDesignOptionsOfFirstSetID = 18,
	DumpDesignOptionSetsID = 19,
	DumpAllDesignOptionsID = 20,
	DumpActiveDesignOptionsOfFirstCombinationID = 21,
	DumpElementsOfFirstDesignOptionID = 22,
	MoveDesignOptionSetID = 23,
	SetLastDesignOptionAsDefaultID = 24,
	SetMainModelAsDefaultID = 25,
	DeleteDesignOptionSetID = 26,
	ModifyDesignOptionsIdID = 27,
	CreateDesignOptionCombinationID = 28,
	DeleteDesignOptionCombinationID = 29,
	ModifyDesignOptionsCombinationsNameID = 30,
	ActivateFirstOptionOfEverySetInCombinationID = 31,
	DeactivateEveryOptionInCombinationID = 32,
	DuplicateDesignOptionCombinationID = 33,
	GetVisibleWallsByDesignOptionsID = 34
};

#endif