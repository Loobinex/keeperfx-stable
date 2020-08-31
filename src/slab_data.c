/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file slab_data.c
 *     Map Slabs support functions.
 * @par Purpose:
 *     Definitions and functions to maintain map slabs.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     25 Apr 2009 - 12 May 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "slab_data.h"
#include "globals.h"

#include "bflib_memory.h"
#include "player_instances.h"
#include "config_terrain.h"
#include "map_blocks.h"
#include "ariadne.h"
#include "ariadne_wallhug.h"
#include "map_utils.h"
#include "frontmenu_ingame_map.h"
#include "game_legacy.h"
#include "creature_states.h"
#include "map_data.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
const short around_slab[] = {-86, -85, -84,  -1,   0,   1,  84,  85,  86};
const short small_around_slab[] = {-85,   1,  85,  -1};
struct SlabMap bad_slabmap_block;
/******************************************************************************/
/******************************************************************************/
/**
 * Returns slab number, which stores both X and Y coords in one number.
 */
SlabCodedCoords get_slab_number(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
  if (slb_x > map_tiles_x) slb_x = map_tiles_x;
  if (slb_y > map_tiles_y) slb_y = map_tiles_y;
  if (slb_x < 0)  slb_x = 0;
  if (slb_y < 0) slb_y = 0;
  return slb_y*(map_tiles_x) + (SlabCodedCoords)slb_x;
}

/**
 * Decodes X coordinate from slab number.
 */
MapSlabCoord slb_num_decode_x(SlabCodedCoords slb_num)
{
  return slb_num % (map_tiles_x);
}

/**
 * Decodes Y coordinate from slab number.
 */
MapSlabCoord slb_num_decode_y(SlabCodedCoords slb_num)
{
  return (slb_num/(map_tiles_x))%map_tiles_y;
}

/**
 * Returns SlabMap struct for given slab number.
 */
struct SlabMap *get_slabmap_direct(SlabCodedCoords slab_num)
{
  if (slab_num >= map_tiles_x*map_tiles_y)
      return INVALID_SLABMAP_BLOCK;
  return &game.slabmap[slab_num];
}

/**
 * Returns SlabMap struct for given (X,Y) slab coords.
 */
struct SlabMap *get_slabmap_block(MapSlabCoord slab_x, MapSlabCoord slab_y)
{
  if ((slab_x < 0) || (slab_x >= map_tiles_x))
      return INVALID_SLABMAP_BLOCK;
  if ((slab_y < 0) || (slab_y >= map_tiles_y))
      return INVALID_SLABMAP_BLOCK;
  return &game.slabmap[slab_y*(map_tiles_x) + slab_x];
}

/**
 * Gives SlabMap struct for given (X,Y) subtile coords.
 * @param stl_x
 * @param stl_y
 */
struct SlabMap *get_slabmap_for_subtile(MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    if ((stl_x < 0) || (stl_x >= map_subtiles_x))
        return INVALID_SLABMAP_BLOCK;
    if ((stl_y < 0) || (stl_y >= map_subtiles_y))
        return INVALID_SLABMAP_BLOCK;
    return &game.slabmap[subtile_slab(stl_y)*(map_tiles_x) + subtile_slab(stl_x)];
}

/**
 * Gives SlabMap struct for slab on which given thing is placed.
 * @param thing The thing which coordinates are used to retrieve SlabMap.
 */
struct SlabMap *get_slabmap_thing_is_on(const struct Thing *thing)
{
    if (thing_is_invalid(thing))
        return INVALID_SLABMAP_BLOCK;
    return get_slabmap_for_subtile(thing->mappos.x.stl.num, thing->mappos.y.stl.num);
}

PlayerNumber get_slab_owner_thing_is_on(const struct Thing *thing)
{
    if (thing_is_invalid(thing))
        return game.neutral_player_num;
    struct SlabMap* slb = get_slabmap_for_subtile(thing->mappos.x.stl.num, thing->mappos.y.stl.num);
    return slabmap_owner(slb);
}

/**
 * Returns if given SlabMap is not a part of the map.
 */
TbBool slabmap_block_invalid(const struct SlabMap *slb)
{
    if (slb == NULL)
        return true;
    if (slb == INVALID_SLABMAP_BLOCK)
        return true;
    return (slb < &game.slabmap[0]);
}

/**
 * Returns if the slab coords are in range of existing map slabs.
 */
TbBool slab_coords_invalid(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
  if ((slb_x < 0) || (slb_x >= map_tiles_x))
      return true;
  if ((slb_y < 0) || (slb_y >= map_tiles_y))
      return true;
  return false;
}

/**
 * Returns owner index of given SlabMap.
 */
long slabmap_owner(const struct SlabMap *slb)
{
    if (slabmap_block_invalid(slb))
        return 5;
    return slb->field_5 & 0x07;
}

/**
 * Sets owner of given SlabMap.
 */
void slabmap_set_owner(struct SlabMap *slb, PlayerNumber owner)
{
    if (slabmap_block_invalid(slb))
        return;
    slb->field_5 ^= (slb->field_5 ^ owner) & 0x07;
}

/**
 * Sets owner of a slab on given position.
 */
void set_whole_slab_owner(MapSlabCoord slb_x, MapSlabCoord slb_y, PlayerNumber owner)
{
    MapSubtlCoord stl_x = STL_PER_SLB * slb_x;
    MapSubtlCoord stl_y = STL_PER_SLB * slb_y;
    for (long i = 0; i < STL_PER_SLB; i++)
    {
        for (long k = 0; k < STL_PER_SLB; k++)
        {
            struct SlabMap* slb = get_slabmap_for_subtile(stl_x + k, stl_y + i);
            slabmap_set_owner(slb, owner);
        }
    }
}

/**
 * Returns Water-Lava under Bridge flags for given SlabMap.
 */
unsigned long slabmap_wlb(struct SlabMap *slb)
{
    if (slabmap_block_invalid(slb))
        return 0;
    return (slb->field_5 >> 3) & 0x03;
}

/**
 * Sets Water-Lava under Bridge flags for given SlabMap.
 */
void slabmap_set_wlb(struct SlabMap *slb, unsigned long wlbflag)
{
    if (slabmap_block_invalid(slb))
        return;
    slb->field_5 ^= (slb->field_5 ^ (wlbflag << 3)) & 0x18;
}

/**
 * Returns slab number of the next tile in a room, after the given one.
 */
long get_next_slab_number_in_room(SlabCodedCoords slab_num)
{
    if (slab_num >= map_tiles_x*map_tiles_y)
        return 0;
    return game.slabmap[slab_num].next_in_room;
}

TbBool slab_is_safe_land(PlayerNumber plyr_idx, MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    struct SlabAttr* slbattr = get_slab_attrs(slb);
    int slb_owner = slabmap_owner(slb);
    if ((slb_owner == plyr_idx) || (slb_owner == game.neutral_player_num))
    {
        return slbattr->is_safe_land;
    }
    return false;
}

TbBool slab_is_door(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    return slab_kind_is_door(slb->kind);
}

TbBool slab_is_liquid(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    return slab_kind_is_liquid(slb->kind);
}

TbBool slab_is_wall(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    if ( (slb->kind <= SlbT_WALLPAIRSHR) || (slb->kind == SlbT_GEMS) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

TbBool slab_kind_is_animated(SlabKind slbkind)
{
    if (slab_kind_is_door(slbkind))
        return true;
    if ((slbkind == SlbT_GUARDPOST) || (slbkind == SlbT_BRIDGE) || (slbkind == SlbT_GEMS))
        return true;
    return false;
}

TbBool can_build_room_at_slab(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    if (!subtile_revealed(slab_subtile_center(slb_x), slab_subtile_center(slb_y), plyr_idx)) {
        SYNCDBG(7,"Cannot place %s owner %d as slab (%d,%d) is not revealed",room_code_name(rkind),(int)plyr_idx,(int)slb_x,(int)slb_y);
        return false;
    }
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    if (slb->room_index > 0) {
        SYNCDBG(7,"Cannot place %s owner %d as slab (%d,%d) has room index %d",room_code_name(rkind),(int)plyr_idx,(int)slb_x,(int)slb_y,(int)slb->room_index);
        return false;
    }
    if (slab_has_trap_on(slb_x, slb_y) || slab_has_door_thing_on(slb_x, slb_y)) {
        SYNCDBG(7,"Cannot place %s owner %d as slab (%d,%d) has blocking thing on it",room_code_name(rkind),(int)plyr_idx,(int)slb_x,(int)slb_y);
        return false;
    }
    if (rkind == RoK_BRIDGE) {
        return slab_kind_is_liquid(slb->kind) && slab_by_players_land(plyr_idx, slb_x, slb_y);
    }
    if (slabmap_owner(slb) != plyr_idx) {
        return false;
    }
    return (slb->kind == SlbT_CLAIMED);
}

int can_build_room_of_radius(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y, int radius, TbBool even)
{
    MapCoord buildx;
    MapCoord buildy;
    int count = 0;
    for (buildy = slb_y - radius; buildy <= slb_y + (radius + even); buildy += 1)
    {
        for (buildx = slb_x - radius; buildx <= slb_x + (radius + even); buildx += 1)
        {
            if (can_build_room_at_slab(plyr_idx, rkind, buildx, buildy))
            {
                count++;
            }
        }
    }
    return count;
}

int calc_distance_from_centre(int totalDistance, TbBool offset)
{
    return ((totalDistance - 1 + offset) / 2);
}

int can_build_room_of_dimensions(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y, int width, int height, int mode)
{
    //struct SlabMap* slbAA = get_slabmap_block(slb_x, slb_y);
    //struct SlabAttr* slbattrAA = get_slab_attrs(slbAA);
    MapCoord buildx;
    MapCoord buildy;
    int count = 0;
    int airBlocks = 0;
    int airBlocksX1 = 0, airBlocksX2 = 0, airBlocksY1 = 0, airBlocksY2 = 0;
    int RectX1 = slb_x - calc_distance_from_centre(width,0);
    int RectX2 = slb_x + calc_distance_from_centre(width,(width % 2 == 0));
    int RectY1 = slb_y - calc_distance_from_centre(height,0);
    int RectY2 = slb_y + calc_distance_from_centre(height,(height % 2 == 0));
    //int THEAIRBLOCKS
    
    for (buildy = RectY1; buildy <= RectY2; buildy++)
    {
        for (buildx = RectX1; buildx <= RectX2; buildx++)
        {
            struct SlabMap* slb = get_slabmap_block(buildx, buildy);
            struct SlabAttr* slbattr = get_slab_attrs(slb);
            if ((mode & 1) == 1) // "strict blocking"
            {
                if ( !slab_is_wall(buildx, buildy) && !slab_is_liquid(buildx, buildy) ) //!((slbattr->block_flags & SlbAtFlg_Blocking) == SlbAtFlg_Blocking) ) // && !slab_is_wall(buildx, buildy)) // || slb->kind == SlbT_ROCK)
                {
                    count++;
                }
            }
            else if ((mode & 2) == 2) // "loose blocking"
            {
                if (!slab_is_wall(buildx, buildy)) // && !slab_is_liquid(buildx, buildy)) //!slab_is_door(buildx, buildy) && !slab_is_wall(buildx, buildy) )
                {
                    count++;
                }
            }
            else // "all blocking"
            {
                if ( can_build_room_at_slab(plyr_idx, rkind, buildx, buildy) )
                {
                    count++;
                }
            }
            if (((mode & 8) == 8) && (slab_is_liquid(buildx, buildy))) // || slb->kind == SlbT_ROCK)) // "air" blocks (in air block ignoring mode)
            {
                if ((buildy == RectY1 || buildy == RectY2 || buildx == RectX1 || buildx == RectX2))
                {
                    airBlocks++;
                }
                if (buildx == RectX1)
                {
                    airBlocksX1++;
                }
                if (buildx == RectX2)
                {
                    airBlocksX2++;
                }
                if (buildy == RectY1)
                {
                    airBlocksY1++;
                }
                if (buildy == RectY2)
                {
                    airBlocksY2++;
                }
            }
        }
    }
    if ((mode & 8) == 8) // "air" blocks mode (reject room if there is at least 1 column/row's worth of air blocks)
    {
        if ( ( (airBlocksX1 >= height) || (airBlocksX2 >= height) ) && ( (airBlocksY1 >= width)  || (airBlocksY2 >= width) ) )
        {
            return 0;
        }
        /*else if ( (airBlocks >= max(width * 2,height * 2)) && ( ( (airBlocksX1 < height) && (airBlocksX2 < height) ) && ( (airBlocksY1 < width)  && (airBlocksY2 < width) ) ) )
        {
            if (slab_is_liquid(buildx, buildy)) //check centre of room is a valid tile
            {
                return 0;
            }
            //return 0;
        }*/

    }
    return count;
}

struct RoomMap get_biggest_room(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord cursor_x, MapSlabCoord cursor_y, short slabCost, int totalMoney, int mode)
{
    int maxRoomRadius = 5; // 9x9 Room
    int maxRoomWidth = ((maxRoomRadius * 2) - 1);
    int minRoomWidth = 2; // Don't look for rooms smaller than 2x2
    int min_width = minRoomWidth, min_height = minRoomWidth;
    int subRoomCheckCount = 5; // the number of sub-rooms to combine in to a final meta-room
    struct RoomMap bestRooms[subRoomCheckCount];
    int bestRoomsCount = 0;
    // Set default "room" - i.e. 1x1 slabs, centred on the cursor
    struct RoomMap current_biggest_room = { {{false}}, 1, true, 1, 1, cursor_x, cursor_y, cursor_x, cursor_y, cursor_x, cursor_y };
    /*for (int y = 0; y <= MAX_ROOM_WIDTH; y++) // initialise entire room_grid 2D array
    {
        for (int x = 0; x <= MAX_ROOM_WIDTH; x++)
        {
            current_biggest_room.room_grid[x][y] = false; // set to false by default
        }
    }*/
    //current_biggest_room.room_grid[0][0] = true;
    struct RoomMap best_room = current_biggest_room;

    // Find the biggest room
    // =====================

    // loop through the 3 sub rooms, that are used to construct the meta room (mode 32 only, otherwise it only loops once)
    do
    {
        // loop through the slabs in the search radius
        for (int r = cursor_y - maxRoomRadius; r <= cursor_y + maxRoomRadius; r++)
        {
            for (int c = cursor_x - maxRoomRadius; c <= cursor_x + maxRoomRadius; c++)
            {
                // for each tile within the search radius, taken as the centre of a "room" :- loop through the room sizes, from biggest width/height to smallest width/height
                for (int w = maxRoomWidth; w >= min_width; w--)
                {
                    if ((w * maxRoomWidth) < current_biggest_room.slabCount) { break; } // sanity check, to stop pointless iterations of the loop

                    for (int h = maxRoomWidth; h >= min_height; h--)
                    {
                        if ((w * h) < current_biggest_room.slabCount) { break; } // sanity check, to stop pointless iterations of the loop

                        // check aspect ratio of the new room
                        float minimumRatio = (2.0 / 5.0);
                        if (((mode & 32) == 32))
                        {
                            minimumRatio = 0.0; //accept any ratio in mode 32
                            if (bestRoomsCount == 0) { minimumRatio = 1.0; } //except on the first pass, where it must be square
                        }
                        if (((min(w,h) * 1.0) / (max(w,h) * 1.0)) < minimumRatio) 
                        {
                            continue; //don't make super long rooms (avoids detecting corridors)
                        }
                        // get the extents of the current room
                        int leftExtent = c - ((w - 1 - (w % 2 == 0)) / 2);
                        int rightExtent = c + ((w     - (w % 2 != 0)) / 2);
                        int topExtent = r - ((h - 1 - (h % 2 == 0)) / 2);
                        int bottomExtent = r + ((h     - (h % 2 != 0)) / 2);
                        // check if cursor is not in the current room
                        if ((cursor_x >= leftExtent && cursor_x <= rightExtent && cursor_y >= topExtent && cursor_y <= bottomExtent) == 0)
                        {
                            continue; // not a valid room
                        }
                        // check to see if the room collides with any walls (etc)
                        int slabs = w * h;
                        int leniency = (((mode & 16) == 16) && bestRoomsCount == 0) ? 1 : 0; // mode=16 :- (setting to 1 would allow e.g. 1 dirt block in the room)
                        int roomarea = can_build_room_of_dimensions(plyr_idx, rkind, c, r, w, h, mode);
                        if (((roomarea) >= slabs - leniency) && ((roomarea * slabCost) <= totalMoney))
                        {
                            if (roomarea > current_biggest_room.slabCount)
                            {
                                current_biggest_room.slabCount = roomarea;
                                current_biggest_room.centreX = c;
                                current_biggest_room.centreY = r;
                                current_biggest_room.width = w;
                                current_biggest_room.height = h;
                                current_biggest_room.left = leftExtent;
                                current_biggest_room.right = rightExtent;
                                current_biggest_room.top = topExtent;
                                current_biggest_room.bottom = bottomExtent;
                                current_biggest_room.isRoomABox = true;
                            }
                            break;
                        }
                    }
                }
                // end loop of room sizes
            }
        }
        // end loop of search area

        if (((mode & 32) == 32)) // new auto placement mode
        {
            // new auto placement mode - it scans for the biggest room, then the biggest room that is wider, then the biggest room that is taller...
            // ...then combines these in to one meta room (which is stored in a 2D array of booleans [1BPP array])

            // get the information on the biggest room just found
            // if no room was found, this info describes the previously found biggest room (so no empty values)
            // if no room is ever found, current_biggest_room is the the default defined above (a single slab)
            bestRooms[bestRoomsCount] = current_biggest_room;
            bestRoomsCount++;
            int correct_index = ((bestRoomsCount > 2) ? bestRoomsCount - 2 : 0); // we want to compare to the previously stored widest/tallest room
            if (bestRoomsCount % 2 != 0) // check for wider rooms
            {
                min_width = min(maxRoomWidth,max(bestRooms[correct_index].width + 1, minRoomWidth));
                min_height = minRoomWidth;
            }
            else // check for taller rooms
            {
                min_width = minRoomWidth;
                min_height = min(maxRoomWidth,max(bestRooms[correct_index].height + 1, minRoomWidth));
            }
            current_biggest_room.slabCount = 1; // reset biggest recorded room to a single slab
        }
    }
    while(((mode & 32) == 32) && (bestRoomsCount < subRoomCheckCount)); // loop again, if in mode 32, for the extra room checks

    // Return the best_room to the calling function
    // ============================================

    if ((mode & 32) == 32) // new auto placement mode
    {  
        // find the extents of the new meta room
        int minX = bestRooms[0].left;
        int maxX = bestRooms[0].right;
        int minY = bestRooms[0].top;
        int maxY = bestRooms[0].bottom;
        for (int j = 1; j < bestRoomsCount; j++)
        {
            if (((maxX - minX) < MAX_ROOM_WIDTH) && (bestRooms[j].left < minX)) minX = bestRooms[j].left;
            if (((maxX - minX) < MAX_ROOM_WIDTH) && (bestRooms[j].right > maxX)) maxX = bestRooms[j].right;
            if (((maxY - minY) < MAX_ROOM_WIDTH) && (bestRooms[j].top < minY)) minY = bestRooms[j].top;
            if (((maxY - minY) < MAX_ROOM_WIDTH) && (bestRooms[j].bottom > maxY)) maxY = bestRooms[j].bottom;
        }
        int metaRoomWidth = (maxX - minX + 1);
        int metaRoomHeight = (maxY - minY + 1);
        // idiot check for empty room
        if ((metaRoomWidth * metaRoomHeight) <= 1) 
        {
            return best_room; // if no room is found, return the default room (a single slab) [set at the start of this function]
        }
        // else, populate best_room
        best_room.width = metaRoomWidth;
        best_room.height = metaRoomHeight;
        best_room.centreX = minX + ((best_room.width - 1 - (best_room.width % 2 == 0)) / 2);
        best_room.centreY = minY + ((best_room.height - 1 - (best_room.height % 2 == 0)) / 2);
        best_room.left = minX;
        best_room.right = maxX;
        best_room.top = minY;
        best_room.bottom = maxY;
        best_room.slabCount = 0;
        best_room.isRoomABox = true;
        // loop through all of the tiles within the extents of the meta room, and check if they are in any of the sub rooms
        for (int y = 0; y <= best_room.height; y++)
        {
            int current_y = minY + y;
            for (int x = 0; x <= best_room.width; x++)
            {
                int current_x = minX + x;
                best_room.room_grid[x][y] = false; // set to false by default
                for (int j = 0; j < bestRoomsCount; j++)
                {
                    if (current_x >= bestRooms[j].left && current_x <= bestRooms[j].right && current_y >= bestRooms[j].top && current_y <= bestRooms[j].bottom)
                    {
                        best_room.room_grid[x][y] = true; //set to true if the current slab is found in any of the sub rooms
                        best_room.slabCount++;
                        break;
                    }
                }
            }
        }
        if (best_room.slabCount != (best_room.width * best_room.height))
        {
            best_room.isRoomABox = false;
        }
    }
    else // not mode 32 (not new auto placement mode)
    {
        if (current_biggest_room.slabCount > 3)
        {
            // return biggest square/rectangular room
            best_room = current_biggest_room;
        }
        // if area less than 4, then no room was found (default of a single slab is returned instead)
    }
    return best_room;
}

/**
 * Clears all SlabMap structures in the map.
 */
void clear_slabs(void)
{
    for (unsigned long y = 0; y < map_tiles_y; y++)
    {
        for (unsigned long x = 0; x < map_tiles_x; x++)
        {
            struct SlabMap* slb = &game.slabmap[y * map_tiles_x + x];
            LbMemorySet(slb, 0, sizeof(struct SlabMap));
            slb->kind = SlbT_ROCK;
        }
    }
}

SlabKind find_core_slab_type(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    struct SlabAttr* slbattr = get_slab_attrs(slb);
    SlabKind corekind;
    switch (slbattr->category)
    {
    case SlbAtCtg_FriableDirt:
        corekind = SlbT_EARTH;
        break;
    case SlbAtCtg_FortifiedWall:
        corekind = SlbT_WALLDRAPE;
        break;
    case SlbAtCtg_Obstacle:
        // originally, 99 was returned by this case, without further conditions
        if ((slbattr->block_flags & SlbAtFlg_IsRoom) != 0)
            corekind = SlbT_BRIDGE;
        else
            corekind = SlbT_DOORWOOD1;
        break;
    default:
        corekind = slb->kind;
        break;
    }
    return corekind;
}

long calculate_effeciency_score_for_room_slab(SlabCodedCoords slab_num, PlayerNumber plyr_idx)
{
    //return _DK_calculate_effeciency_score_for_room_slab(slab_num, plyr_idx);
    TbBool is_room_inside = true;
    long eff_score = 0;
    struct SlabMap* slb = get_slabmap_direct(slab_num);
    long n;
    for (n=1; n < AROUND_SLAB_LENGTH; n+=2)
    {
        long round_slab_num = slab_num + around_slab[n];
        struct SlabMap* round_slb = get_slabmap_direct(round_slab_num);
        if (!slabmap_block_invalid(round_slb))
        {
            MapSlabCoord slb_x = slb_num_decode_x(round_slab_num);
            MapSlabCoord slb_y = slb_num_decode_y(round_slab_num);
            // Per slab code
            if ((slabmap_owner(round_slb) == slabmap_owner(slb)) && (round_slb->kind == slb->kind))
            {
                eff_score += 2;
            } else
            {
                is_room_inside = false;
                switch (find_core_slab_type(slb_x, slb_y))
                {
                  case SlbT_ROCK:
                  case SlbT_GOLD:
                  case SlbT_EARTH:
                  case SlbT_GEMS:
                    eff_score++;
                    break;
                  case SlbT_WALLDRAPE:
                    if (slabmap_owner(round_slb) == slabmap_owner(slb))
                        eff_score += 2;
                    break;
                  case SlbT_DOORWOOD1:
                    if (slabmap_owner(round_slb) == slabmap_owner(slb))
                        eff_score += 2;
                    break;
                  default:
                    break;
                }
            }
            // Per slab code ends
        }
    }
    // If we already know this is not an inside - finish
    if (!is_room_inside) {
        return eff_score;
    }
    // Make sure this is room inside by checking corners
    for (n=0; n < AROUND_SLAB_LENGTH; n+=2)
    {
        long round_slab_num = slab_num + around_slab[n];
        struct SlabMap* round_slb = get_slabmap_direct(round_slab_num);
        if (!slabmap_block_invalid(round_slb))
        {
            // Per slab code
            if ((slabmap_owner(round_slb) != slabmap_owner(slb)) || (round_slb->kind != slb->kind))
            {
                is_room_inside = 0;
                break;
            }
            // Per slab code ends
        }
    }
    if (is_room_inside) {
        eff_score += 2;
    }
    return eff_score;
}

/**
 * Reveals the whole map for specific player.
 */
void reveal_whole_map(struct PlayerInfo *player)
{
    clear_dig_for_map_rect(player->id_number,0,map_tiles_x,0,map_tiles_y);
    reveal_map_rect(player->id_number,1,map_subtiles_x,1,map_subtiles_y);
    pannel_map_update(0, 0, map_subtiles_x+1, map_subtiles_y+1);
}

void update_blocks_in_area(MapSubtlCoord sx, MapSubtlCoord sy, MapSubtlCoord ex, MapSubtlCoord ey)
{
    update_navigation_triangulation(sx, sy, ex, ey);
    ceiling_partially_recompute_heights(sx, sy, ex, ey);
    light_signal_update_in_area(sx, sy, ex, ey);
}

void update_blocks_around_slab(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    SYNCDBG(7,"Starting");
    MapSubtlCoord stl_x = STL_PER_SLB * slb_x;
    MapSubtlCoord stl_y = STL_PER_SLB * slb_y;

    MapSubtlCoord ey = stl_y + 5;
    if (ey > map_subtiles_y)
        ey = map_subtiles_y;
    MapSubtlCoord ex = stl_x + 5;
    if (ex > map_subtiles_x)
        ex = map_subtiles_x;
    MapSubtlCoord sy = stl_y - STL_PER_SLB;
    if (sy <= 0)
        sy = 0;
    MapSubtlCoord sx = stl_x - STL_PER_SLB;
    if (sx <= 0)
        sx = 0;
    update_blocks_in_area(sx, sy, ex, ey);
}

void update_map_collide(SlabKind slbkind, MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    struct Map* mapblk = get_map_block_at(stl_x, stl_y);
    struct Column* colmn = get_map_column(mapblk);
    if (column_invalid(colmn)) {
        ERRORLOG("Invalid column at (%d,%d)",(int)stl_x,(int)stl_y);
    }
    unsigned long smask = colmn->solidmask;
    MapSubtlCoord stl_z;
    for (stl_z=0; stl_z < map_subtiles_z; stl_z++)
    {
        if ((smask & 0x01) == 0)
            break;
        smask >>= 1;
    }
    struct SlabAttr* slbattr = get_slab_kind_attrs(slbkind);
    unsigned long nflags;
    if (slbattr->field_2 < stl_z) {
      nflags = slbattr->block_flags;
    } else {
      nflags = slbattr->noblck_flags;
    }
    mapblk->flags &= (SlbAtFlg_TaggedValuable|SlbAtFlg_Unexplored);
    mapblk->flags |= nflags;
}

void do_slab_efficiency_alteration(MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    for (long n = 0; n < SMALL_AROUND_SLAB_LENGTH; n++)
    {
        MapSlabCoord sslb_x = slb_x + small_around[n].delta_x;
        MapSlabCoord sslb_y = slb_y + small_around[n].delta_y;
        struct SlabMap* slb = get_slabmap_block(sslb_x, sslb_y);
        if (slabmap_block_invalid(slb)) {
            continue;
        }
        struct SlabAttr* slbattr = get_slab_attrs(slb);
        if (slbattr->category == SlbAtCtg_RoomInterior)
        {
            struct Room* room = slab_room_get(sslb_x, sslb_y);
            set_room_efficiency(room);
            set_room_capacity(room, true);
        }
    }
}

SlabKind choose_rock_type(PlayerNumber plyr_idx, MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    unsigned char flags = 0;
    if ((slb_x % 5) == 0)
    {
        struct SlabMap* pvslb = get_slabmap_block(slb_x, slb_y - 1);
        struct SlabMap* nxslb = get_slabmap_block(slb_x, slb_y + 1);
        if ((pvslb->kind == SlbT_CLAIMED) || (nxslb->kind == SlbT_CLAIMED)) {
            flags |= 0x01;
        }
    }
    if ((slb_y % 5) == 0)
    {
        struct SlabMap* pvslb = get_slabmap_block(slb_x - 1, slb_y);
        struct SlabMap* nxslb = get_slabmap_block(slb_x + 1, slb_y);
        if ((pvslb->kind == SlbT_CLAIMED) || (nxslb->kind == SlbT_CLAIMED)) {
            flags |= 0x02;
        }
    }
    if (flags < 1)
        return SlbT_EARTH;
    else
        return SlbT_TORCHDIRT;
}

/**
 * Counts amount of tiles owned by given player around given slab.
 * @param plyr_idx Owning player to be checked.
 * @param slb_x Target slab to check around, X coord.
 * @param slb_y Target slab to check around, Y coord.
 * @return Amount 0-4 of owned slabs, or just 4 if there is any owned ground or room slab.
 */
int count_owned_ground_around(PlayerNumber plyr_idx, MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    int num_owned = 0;
    for (int i = 0; i < SMALL_AROUND_SLAB_LENGTH; i++)
    {
        MapSlabCoord sslb_x = slb_x + small_around[i].delta_x;
        MapSlabCoord sslb_y = slb_y + small_around[i].delta_y;
        struct SlabMap* slb = get_slabmap_block(sslb_x, sslb_y);
        if (slabmap_owner(slb) == plyr_idx)
        {
            struct SlabAttr* slbattr = get_slab_attrs(slb);
            if ((slbattr->category == SlbAtCtg_FortifiedGround) || (slbattr->category == SlbAtCtg_RoomInterior)) {
                num_owned = 4;
                break;
            } else {
                num_owned++;
            }
        }

    }
    return num_owned;
}

void unfill_reinforced_corners(PlayerNumber keep_plyr_idx, MapSlabCoord base_slb_x, MapSlabCoord base_slb_y)
{
    //_DK_unfill_reinforced_corners(plyr_idx, base_slb_x, base_slb_y); return;
    for (int i = 0; i < SMALL_AROUND_SLAB_LENGTH; i++)
    {
        MapSlabCoord slb_x = base_slb_x + small_around[i].delta_x;
        MapSlabCoord slb_y = base_slb_y + small_around[i].delta_y;
        struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
        struct SlabAttr* slbattr = get_slab_attrs(slb);
        if ((slbattr->category == SlbAtCtg_FortifiedWall) && (slabmap_owner(slb) != keep_plyr_idx))
        {
            int num_owned_around = count_owned_ground_around(slabmap_owner(slb), slb_x, slb_y);
            if (num_owned_around < 2)
            {
                SlabKind slbkind = alter_rock_style(SlbT_EARTH, slb_x, slb_y, game.neutral_player_num);
                place_slab_type_on_map(slbkind, slab_subtile_center(slb_x), slab_subtile_center(slb_y), game.neutral_player_num, 0);
                do_slab_efficiency_alteration(slb_x, slb_y);
            }
        }
    }
}

/**
 * Removes reinforces walls from tiles around given slab, except given owner.
 * @param keep_plyr_idx The owning player whose walls are not to be affected.
 * @param slb_x Central slab for the unprettying effect, X coord.
 * @param slb_y Central slab for the unprettying effect, Y coord.
 */
void do_unprettying(PlayerNumber keep_plyr_idx, MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    for (long n = 0; n < SMALL_AROUND_SLAB_LENGTH; n++)
    {
        long sslb_x = slb_x + (long)small_around[n].delta_x;
        long sslb_y = slb_y + (long)small_around[n].delta_y;
        struct SlabMap* slb = get_slabmap_block(sslb_x, sslb_y);
        struct SlabAttr* slbattr = get_slab_attrs(slb);
        if ((slbattr->category == SlbAtCtg_FortifiedWall) && (slabmap_owner(slb) != keep_plyr_idx))
        {
            if (!slab_by_players_land(slabmap_owner(slb), sslb_x, sslb_y))
            {
                SlabKind newslab = choose_rock_type(keep_plyr_idx, sslb_x, sslb_y);
                place_slab_type_on_map(newslab, slab_subtile_center(sslb_x), slab_subtile_center(sslb_y), game.neutral_player_num, 0);
                unfill_reinforced_corners(keep_plyr_idx, sslb_x, sslb_y);
                do_slab_efficiency_alteration(sslb_x, sslb_y);
            }
        }
    }
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif
