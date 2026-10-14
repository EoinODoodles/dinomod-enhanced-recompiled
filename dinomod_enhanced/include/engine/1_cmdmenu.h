#pragma once

#include "PR/ultratypes.h"
#include "dlls/engine/1_cmdmenu.h"

s16 cmdmenu_info_popup_is_visible();
int cmdmenu_is_button_override_active();

void cmdmenu_InventoryOpen(void);
void cmdmenu_InventoryOpenItems(void);
void cmdmenu_InventoryOpenFoodbagActions(void);
void cmdmenu_InventoryOpenFoodbag(void);
void cmdmenu_InventoryClose(void);
void cmdmenu_InventoryCloseInstantly(void);
void cmdmenu_InventoryHideSidekickMeterInstantly(void);
void cmdmenu_InventoryLeftAlign(void);
void cmdmenu_InventorySetVerticalOffset(s16 offset);
void cmdmenu_InventoryResetOffset(void);
void cmdmenu_InventoryMoveDown(void);
s32 cmdmenu_GetSelectedItemGamebit(void);
s16 cmdmenu_GetSelectedItemTextLineIdx(void);
_Bool cmdmenu_GetInventoryOpen(void);

void cmdmenu_InfoScrollSetWidthOverride(s16 width);
void cmdmenu_InfoScrollResetWidth(void);
void cmdmenu_InfoScrollSetHeightOverride(s16 height);
void cmdmenu_InfoScrollResetHeight(void);
