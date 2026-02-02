
#ifndef OPENINGFUNCTIONS_TEST_HPP
#define OPENINGFUNCTIONS_TEST_HPP

// =============================================================================
//
// Opening element functions
//
// =============================================================================

#include "Definitions.hpp"

GSErrCode	PlaceOpeningIntoAnElement ();
GSErrCode	PlacePolygonalOpeningIntoAnElement ();
GSErrCode	ChangeOpeningsParent ();
GSErrCode	ConnectOpeningToAnotherElement ();
GSErrCode	DisconnectOpeningFromAnotherElement ();
GSErrCode	SelectOpeningsConnectedToElement ();
GSErrCode	SelectElementsConnectedToOpening ();
GSErrCode	SelectOpeningsParentElement ();
GSErrCode	ChangeOpeningsDimensionsToCurrentDefault ();
GSErrCode	ChangeOpeningsCutSymbolToCurrentDefault ();
GSErrCode	ChangeOpeningsOverrideSurfacesToCurrentDefault ();
GSErrCode	ChangeOpeningDefaultsCoverFillTypeAndPen ();

#endif

