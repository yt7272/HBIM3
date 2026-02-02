#include "Notification_Manager.h"

#include "ACAPinc.h"
#include "ACAPI/UI/NotificationBubble/NotificationBubbleManager.hpp"
#include "ACAPI/UI/NotificationBubble/NotificationBubble.hpp"

#include "DGUtility.hpp"


void	ShowNotificationBubble ()
{
	// ! [NotificationBubble-Example]

	API_Coord3D localOrigo3D;
	if (auto getLocalOrigoResult = ACAPI_Database_GetLocOrigo (&localOrigo3D); getLocalOrigoResult != NoError) {
		ACAPI_WriteReport ("Failed to get Local Origo.", false);
		return;
	}

	API_Coord localOrigo;
	ACAPI_View_ModelToScreen (localOrigo3D, localOrigo);

	API_Point point;
	ACAPI_View_CoordToPoint (&localOrigo, &point);

	static constexpr const char* TestGuid = "255EACB7-197C-4176-B231-813D8F39AB29";

	auto notificationBubble = ACAPI::UI::Notification::CreateNotificationBubbleManager ().CreateNotificationBubble (APIGuidFromString (TestGuid));

	GS::UniString message = "This is a notification bubble at the local origo";

	notificationBubble.SetText (message);
	notificationBubble.SetIcon (DG::Icon (SysResModule, DG::Icon::Floppy));
	notificationBubble.SetTimeout (2000);

	notificationBubble.Show (point);

	// ! [NotificationBubble-Example]
}
