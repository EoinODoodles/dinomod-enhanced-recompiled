#include "modding.h"
#include "dbgui.h"

#include "dinomod_dbgui.h"

RECOMP_DECLARE_EVENT(dinomod_dbgui_event())

static void dinomod_debug_menus(void) {
    dinomod_dbgui_chapter_select_menu();
}

RECOMP_CALLBACK("*", recomp_on_dbgui) void dinomod_dbgui_callback(void) {
    if (!dbgui_is_open()) return;

    if (dbgui_begin_main_menu_bar()) {
        if (dbgui_begin_menu("Debug")) {
            dbgui_separator_text("Dinomod");
            dinomod_debug_menus();
            dbgui_end_menu();
        }
        
        dbgui_end_main_menu_bar();
    }

    dinomod_dbgui_event();
}
