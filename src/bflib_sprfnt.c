/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file bflib_sprfnt.c
 *     Bitmap sprite fonts support library.
 * @par Purpose:
 *     Functions for reading bitmap sprite fonts and using them to display text.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     29 Dec 2008 - 11 Jan 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "bflib_sprfnt.h"

#include "bflib_sprite.h"
#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
DLLIMPORT int __cdecl _DK_LbTextDraw(int posx, int posy, const char *text);
/******************************************************************************/

int LbTextDraw(int posx, int posy, const char *text)
{
  return _DK_LbTextDraw(posx, posy, text);
}

int LbTextHeight(const char *text)
{
  if (lbFontPtr == NULL)
    return 0;
  return lbFontPtr[1].SHeight;
}

/*
char __fastcall font_height(const unsigned char c)
{
  char height;
  char tmph;
  if ( (lbFontPtr != sprites.Font0) && (lbFontPtr != sprites.Font5) )
  {
    if ( lbFontPtr == sprites.Font3 )
    {
      if ( (c >= 'a') && (c <= 'z') )
      {
        if ( lbFontPtr )
          tmph = lbFontPtr[(unsigned char)c-31].SHeight;
        else
          tmph = 0;
        height = tmph;
      } else
      {
        if ( lbFontPtr )
          tmph = lbFontPtr[(unsigned char)c-31].SHeight;
        else
          tmph = 0;
        height = tmph - 2;
      }
    }
    else
    {
      if ( (lbFontPtr!=sprites.Font1) && (lbFontPtr!=sprites.Font4) )
      {
        if ( lbFontPtr == sprites.Font2 )
        {
          if ( lbFontPtr )
            tmph = lbFontPtr[(unsigned char)c-31].SHeight;
          else
            tmph = 0;
          height = tmph - 4;
        }
        else
        {
          if ( lbFontPtr )
            tmph = lbFontPtr[(unsigned char)c-31].SHeight;
          else
            tmph = 0;
          height = tmph;
        }
      }
      else
      {
        if ( lbFontPtr )
          tmph = lbFontPtr[(unsigned char)c-31].SHeight;
        else
          tmph = 0;
        height = tmph - 2;
      }
    }
  } else
  { //so we have font0 or font5
    if ( lbFontPtr )
      tmph = lbFontPtr[(unsigned char)c-31].SHeight;
    else
      tmph = 0;
    height = tmph - 1;
  }
  return height;
}

unsigned long __fastcall my_string_width(const char *str)
{
  char *chr = (char *)str;
  if (chr==NULL) return 0;
  unsigned long len = 0;
  while ( *chr )
  {
    if ( *chr > 31 )
    {
      char c;
      if ( (lbFontPtr==sprites.Font3) && (lang_selection==1) )
        c = *chr;
      else
        c = my_to_upper(*chr);
      int cidx=(unsigned char)c - 31;
      len += lbFontPtr[cidx].SWidth;
    }
    ++chr;
  }
  return len;
}
*/

/******************************************************************************/
#ifdef __cplusplus
}
#endif
