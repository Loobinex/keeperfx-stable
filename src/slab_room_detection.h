/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file slab_room_detection.h
 *     Header file for slab_room_detection.c.
 * @par Purpose:
 *     Function for finding the best "room" based on the current cursor position.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Ed Kearney
 * @date     28 Aug 2020 - 27 Sep 2020
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_ROOM_DETECT_H
#define DK_ROOM_DETECT_H

#include "slab_data.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
struct RoomQuery {
    short slabCost;
    int totalMoney;
    int mode;
    int maxRoomRadius;
    int maxRoomWidth;
    int minRoomWidth;
    int minRoomHeight;
    int subRoomCheckCount;
    int bestRoomsCount;
    struct RoomMap best_room;
    struct RoomMap best_corridor;
    MapSlabCoord cursor_x;
    MapSlabCoord cursor_y;
    MapSlabCoord centre_x;
    MapSlabCoord centre_y;
    PlayerNumber plyr_idx;
    RoomKind rkind;
    float minimumRatio;
    float minimumComparisonRatio;
    TbBool isCorridor;
    TbBool isCompoundRoom;
    int leniency;
    int moneyLeft;
    int InvalidBlocksIgnored;
    TbBool findCorridors;
    TbBool foundRoom;
    int room_discovery_looseness;
};
/******************************************************************************/
struct RoomMap get_biggest_room(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord cursor_x, MapSlabCoord cursor_y, short slabCost, int totalMoney, int mode, int room_discovery_looseness);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
