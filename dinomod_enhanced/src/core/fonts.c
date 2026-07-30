#include "modding.h"

#include "sys/fonts.h"
#include "sys/gfx/texture.h"
#include "macros.h"

#include "core/fonts.h"

extern s32 gNumFonts;
extern FontData *gFile_FONTS_BIN;
extern FontWindow *gFontWindows;
extern FontString *gFontStrings;
extern s32 gFontSquash;

/** 
 *  A custom functions that counts how many lines a string of text will be wrapped onto. 
 *  It's just a rejigged duplicate of `fontRenderTextWordwrap`, reusing its logic!
 *
 *  This function is used to fix a bug where task lines overlapped on the "Previously On" screen
 *  when they wrapped onto 3 lines instead of the usual 2.
 *
 * `posX` is a left margin offset (assuming left-aligned text) for the font window, useful
 *  since the equivalent margin used by `fontWindowAddStringXY` may not have updated the FontWindow's xpos yet.
 */
s32 fontCountLinesWordwrap(s32 windowID, char* text, s32 posX) {
    s32 loopCond;
    FontData* fontData;
    s32 ypos;
    s32 xpos;
    s32 rectUly;
    s32 rectUlx;
    s32 wordWidth;
    s32 xStart;
    s32 bufIdx;
    s32 wordCharCount;
    s32 yStart;
    s32 wordWrapped;
    s32 rectLrx;
    s32 rectLry;
    Texture *tex;
    u8 charBuffer[128];
    u8 curChar;
    u8 lastTextureIndex;
    s32 textureS;
    s32 textureT;

    s32 lineCount = 1;

    FontWindow* window = &gFontWindows[windowID];
    
    if (window == NULL) {
        return 0;
    }
    if (text == NULL) {
        return 0;
    }
    if (window->font == 0xFF) {
        return 0;
    }
    
    xpos = window->xpos + posX;
    ypos = window->ypos;
    fontData = &gFile_FONTS_BIN[window->font];
    
    loopCond = 0;
    
    while (loopCond >= 0) {
        if (xpos >= window->width) {
            xpos = window->xpos + posX;
            ypos += fontData->y;
            lineCount++;
            wordWrapped = 1;
        } else {
            wordWrapped = 0;
        }

        if ((window->textOffsetY + ypos) >= window->height) {
            // end if beyond window height
            loopCond = -1;
        } else {
            loopCond = 0;
        }
        
        while (loopCond == 0) {
            curChar = *text;
            // if non-printable char
            if ((curChar <= 0x20) || (curChar >= 0x80)) {
                switch (curChar) {
                case '\0':
                    loopCond = -1;
                    break;
                case '\t':
                    xpos += fontData->charHeight - (xpos % fontData->charHeight);
                    break;
                case '\n':
                    wordWrapped = 0;
                    xpos = window->xpos + posX;
                    ypos += fontData->y;
                    lineCount++;
                    break;
                case '\v':
                    wordWrapped = 0;
                    ypos += fontData->y;
                    lineCount++;
                    break;
                case '\r':
                    wordWrapped = 0;
                    xpos = window->xpos + posX;
                    break;
                default:
                    if (wordWrapped == 0) {
                        xpos += fontData->charWidth;
                    }
                    break;
                }
                if (xpos >= window->width) {
                    xpos = window->xpos + posX;
                    ypos += fontData->y;
                    lineCount++;
                    wordWrapped = 1;
                }
                if ((window->textOffsetY + ypos) >= window->height) {
                    loopCond = -1;
                }
                text++;
            } else {
                loopCond = 1;
            }
        }
        
        if (loopCond > 0) {
            wordCharCount = 0;
            wordWidth = 0;

            PRAGMA_IGNORE_PUSH("-Wtype-limits")
            do {
                charBuffer[wordCharCount] = curChar;
                curChar = (curChar - 0x20);
                wordCharCount += 1;
                if (fontData->letters[curChar].textureIndex != 0xFF) {
                    if (fontData->x != 0) {
                        wordWidth += fontData->x;
                    } else {
                        wordWidth += fontData->letters[curChar].kerning;
                    }
                } else {
                    wordWidth += fontData->charWidth;
                }
                text++;
                curChar = *text;
            } while (wordCharCount < 128 && ((s32) curChar > 0x20) && ((s32) curChar < 0x100));
            // curChar < 0x100 is always true since curChar is u8
            PRAGMA_IGNORE_POP()

            if (((xpos + wordWidth) >= window->width) && (xpos != 0)) {
                xpos = window->xpos + posX;
                ypos += fontData->y;
                lineCount++;
            }
            yStart = window->textOffsetY + ypos;
            if (yStart < window->height) {
                xStart = window->textOffsetX + xpos;
                xpos += wordWidth;
                if ((fontData->y + yStart) > 0) {
                    for (bufIdx = 0; bufIdx < wordCharCount; bufIdx++) {
                        curChar = (charBuffer[bufIdx] - 0x20);
                        if (fontData->letters[curChar].textureIndex != 0xFF) {                            
                            if (fontData->x == 0) {
                                xStart += fontData->letters[curChar].kerning;
                            } else {
                                xStart += fontData->x;
                            }
                        } else {
                            xStart += fontData->charWidth;
                        }
                    }
                }
            } else {
                loopCond = -1;
            }
        }
    }
    
    return lineCount;
}
