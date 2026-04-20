#include "PR/os.h"
#include "libc/string.h"
#include "PR/ultratypes.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "sys/asset_thread.h"
#include "sys/dll.h"
#include "sys/fs.h"
#include "sys/memory.h"
#include "sys/print.h"
#include "sys/string.h"
#include "dll.h"
#include "types.h"
#include "game/objects/object_id.h"
#include "dlls/engine/29_gplay.h"

#include "recomp/dlls/engine/21_gametext_recomp.h"

extern s8 sCurrentBankIndex;
extern u16 sBankCount;
extern u16 sBankEntryCount;
extern u16 sBankSize;
extern u8 *sCurrentBank;
extern s32 sCurrentBank_GlobalOffset;
extern u8 *sCurrentBank_StrCounts;
extern u16 *sCurrentBank_Sizes;
extern u16 *sCurrentBank_Offsets;

extern void gametext_set_bank(s8 bank);

static char CUSTOM_GAMETEXT_STRING[50] = "Custom!";

enum GameTextIDs {
    GAMETEXT_191_Garunda_Te_First_Meeting = 191,
    GAMETEXT_008_Garunda_Te_Trapped_Under_Ice_Chat = 8,
};

typedef struct {
    char english[256];
    char francais[256];
    char deutsch[256];
    char espanol[256];
    char italiano[256];
    char nihongo[256];
} StringLanguages;

typedef union {
    StringLanguages languages;
    char strings[6][256];
} CustomText;

static char LANGUAGE_NAMES[6][10] = {"English", "Français", "Deutsch", "Español", "Italiano", "Nihongo" };

/** Logs a gametext file's strings to the console */
PRAGMA_IGNORE_PUSH("-Wunused")
static void recomp_log_strings(u16 fileID, GameTextChunk *gametext){
    s32 index;

    if (sCurrentBankIndex >= 6 || !gametext || gametext->count == 0){
        return;
    }

    recomp_eprintf("GAMETEXT_%0d (language: %s)\n", fileID, LANGUAGE_NAMES[sCurrentBankIndex]);
    for (index = 0; index < gametext->count; index++){
        recomp_eprintf("#%0d) %s\n", index, gametext->strings[index]);
    }
}
PRAGMA_IGNORE_POP()

/** Moves to the next language index, to help debug across all languages */
PRAGMA_IGNORE_PUSH("-Wunused")
static void recomp_pick_next_language(){
    s8 nextLanguage = sCurrentBankIndex;

    nextLanguage++;
    if (nextLanguage >= 6)
        nextLanguage = 0;

    gametext_set_bank(nextLanguage);
}
PRAGMA_IGNORE_POP()

/** Searches for a substring within a string */
static char* recomp_strstr(char* string, char* searchSubstring){
    s32 index;
    s32 searchIndex;
    s32 length = strlen(string);
    s32 searchLength = strlen(searchSubstring);
    char* result = NULL;

    if (searchLength == 0 || length == 0)
        return result;

    for (index = 0; index < length; index++){
        for (searchIndex = 0; (index + searchIndex) < length; searchIndex++){
            if (string[index + searchIndex] != searchSubstring[searchIndex]){
                break;
            }
            if (searchIndex == searchLength - 1){
                result = (char*)&string[index];
                return result;                
            }
        }
    }

    return result;
}

/** Replaces a single instance of a substring in a string, storing the result to a destination string
  * The function will bail if the edited string runs longer than the original.
  */
static void recomp_replace_substring(char* originalString, char* searchSubstring, char* replacement, char* destination){
    char* substringPtr = NULL;
    char* remainderPtr = NULL;
    s32 matchPosition;
    s32 length = strlen(originalString);
    s32 searchLength = strlen(searchSubstring);
    s32 replacementLength = strlen(replacement);

    if (searchLength == 0 || length == 0)
        return;

    if ((replacementLength - searchLength) > 0){
        recomp_eprintf("Edited string can't run longer than original!\n");
        return;
    }
    
    substringPtr = recomp_strstr(originalString, searchSubstring);
    if (!substringPtr){
        recomp_eprintf("Couldn't find substring '%s' in string '%s'\n", searchSubstring, originalString);
        return;
    }
    
    matchPosition = substringPtr - originalString;
    remainderPtr = substringPtr + searchLength;
    recomp_eprintf("Substring '%s' at index %d in string '%s'\n", searchSubstring, matchPosition, originalString);

    bcopy(originalString, destination, matchPosition);
    bcopy(replacement, destination + matchPosition, replacementLength);
    bcopy(remainderPtr, destination + matchPosition + replacementLength, strlen(remainderPtr));
    destination[matchPosition + replacementLength + strlen(remainderPtr)] = 0;
}

/** Searches through a gametext file until a particular substring is found.
  * Returns the index of the string that contained the substring. */
static s32 find_stringID_with_substring(GameTextChunk *gametext, char *substring){
    s32 index;

    if (!gametext || gametext->count == 0){
        return -1;
    }

    for (index = 0; index < gametext->count; index++){
        if (recomp_strstr(gametext->strings[index], substring)){
            return index;
        }
    }

    return -1;
}

/** Edits the FrostWeed count mentioned in the Gametext lines related to Garunda Te's FrostWeed quest */
static void recomp_garunda_te_dynamic_frostweed_counts(u16 fileID, GameTextChunk *gametext){
    s32 editStringID = -1;
    s32 frostWeedsTotal = recomp_get_config_u32("garunda_te_frostweeds_override");
    char replacementNumberText[] = "";
    char frostWeedTranslations[6][2][40] = {
        {"Frost Weed", "Frost Weeds"},
        {"Herbe Gelée", "Herbes Gelées"},
        {"", ""},
        {"Semilla Congelada", "Semillas Congeladas"},
        {"", ""},
        {"", ""}
    };
    u8 frostWeedsFrench[] = {0x48, 0x65, 0x72, 0x62, 0x65, 0x73, 0x20, 0x47, 0x65, 0x6C, 0xE9, 0x65, 0x73, 0};
    s32 index;

    //Encoding the é is causing trouble, so constructing the French string numerically!
    for (index = 0; index < 14; index++){
        frostWeedTranslations[1][1][index] = frostWeedsFrench[index];
    }
    frostWeedTranslations[1][0][9] = 0xe9;

    //Find relevant string in gametext file
    editStringID = find_stringID_with_substring(gametext, "12");
    if (editStringID < 0){
        return;
    }

    //Replace the number
    recomp_sprintf(replacementNumberText, "%d", frostWeedsTotal);
    recomp_replace_substring(gametext->strings[editStringID], "12", replacementNumberText, CUSTOM_GAMETEXT_STRING);
    
    //Edit the pluralisation if needed (language-specific)
    if (frostWeedsTotal == 1){
        recomp_replace_substring(CUSTOM_GAMETEXT_STRING,
            frostWeedTranslations[sCurrentBankIndex][1], 
            frostWeedTranslations[sCurrentBankIndex][0], 
            CUSTOM_GAMETEXT_STRING);
    }

    gametext->strings[editStringID] = CUSTOM_GAMETEXT_STRING;
}

/** Main function for setting up fileID-specific Gametext edits */
static void recomp_edit_text(u16 fileID, GameTextChunk *gametext){
    switch (fileID){
        //Handle Garunda Te's FrostWeed count text
        case GAMETEXT_191_Garunda_Te_First_Meeting:
        case GAMETEXT_008_Garunda_Te_Trapped_Under_Ice_Chat:
            recomp_garunda_te_dynamic_frostweed_counts(fileID, gametext);
            return;

        //Handle other cases if they come up in future
        default:
            return;
    }
}

// Adds a function call to edit the gametext as needed
RECOMP_PATCH GameTextChunk *gametext_get_chunk(u16 chunk) {
    GameTextChunk *chunkPtr;
    s32 headerSize;
    s32 alignmentPad;
    s32 i;
    s32 k;

    // Size for struct and string pointers
    headerSize = sCurrentBank_StrCounts[chunk] * 4 + sizeof(GameTextChunk);
    // Room for alignment
    alignmentPad = headerSize % 8;

    headerSize += alignmentPad + sCurrentBank_Sizes[chunk];

    chunkPtr = (GameTextChunk*)mmAlloc(headerSize, ALLOC_TAG_GAME_COL, NULL);

    chunkPtr->strings = (char**)((u32)chunkPtr + sizeof(GameTextChunk));

    chunkPtr->count = sCurrentBank_StrCounts[chunk];

    // Where to load actual chunk contents to (also contains data for some chunks)
    chunkPtr->commands = (void*)((u32)chunkPtr->strings + sCurrentBank_StrCounts[chunk] * 4 + alignmentPad);
    
    // Set up first string pointer
    chunkPtr->strings[0] = (char*)((u32)chunkPtr->commands + sCurrentBank_StrCounts[chunk] * 2);

    queue_load_file_region_to_ptr(
        (void**)chunkPtr->commands,
        GAMETEXT_BIN,
        sCurrentBank_Offsets[chunk] * 2 + sCurrentBank_GlobalOffset,
        sCurrentBank_Sizes[chunk]);
    
    // Set up remaining pointers to each string
    for (i = 1; i < sCurrentBank_StrCounts[chunk]; i++) {
        k = 0;

        while (chunkPtr->strings[i - 1][k++] != '\0') {}

        chunkPtr->strings[i] = chunkPtr->strings[i - 1] + k;
    }

    //@recomp: modify the text if needed
    recomp_edit_text(chunk, chunkPtr);

    return chunkPtr;
}

/** Fix for English text appearing in non-English languages (originally by MusicalProgrammer) */
RECOMP_PATCH char *gametext_get_text(u16 chunk, u16 strIndex) {
    u8 *text;
    char *str;
    char *copy;
    s32 len;
    s32 i;     
    s32 k;

    text = mmAlloc(sCurrentBank_Sizes[chunk], ALLOC_TAG_GAME_COL, NULL);

    queue_load_file_region_to_ptr(
        (void**)text, 
        GAMETEXT_BIN, 
        sCurrentBank_Offsets[chunk] * 2 + sCurrentBank_GlobalOffset, //@recomp: calculate offset in same way as "gametext_get_chunk"
        sCurrentBank_Sizes[chunk]);

    str = (char*)(text + sCurrentBank_StrCounts[chunk] * 2);

    for (i = 0; i != strIndex; i++) {
        k = 0;

        while (str[k++] != '\0') {}

        str += k;
    }

    len = strlen(str) + 1;

    copy = mmAlloc(len, ALLOC_TAG_GAME_COL, NULL);
    bcopy(str, copy, len);

    mmFree(text);

    return copy;
}
