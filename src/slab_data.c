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
#include "gui_topmsg.h"
#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
const short around_slab[] = {-86, -85, -84,  -1,   0,   1,  84,  85,  86};
const short small_around_slab[] = {-85,   1,  85,  -1};
struct SlabMap bad_slabmap_block;
//struct RoomQuery room_query;
struct RoomQuery new_room_query;
//struct RoomQuery meta_room;
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

TbBool can_build_room_at_slab_fast(PlayerNumber plyr_idx, RoomKind rkind, MapSlabCoord slb_x, MapSlabCoord slb_y)
{
    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    if (rkind == RoK_BRIDGE)
    {
        return (slab_kind_is_liquid(slb->kind) && slab_by_players_land(plyr_idx, slb_x, slb_y));
    }
    else
    {
        if (slb->kind == SlbT_CLAIMED)
        {
            return (slabmap_owner(slb) == plyr_idx);
        }
    }
    return false;
}

/**
 * Match a value from enum RoomKinds with enum SlabTypes.
**/
TbBool match_room_kind_to_slab_type(RoomKind room_kind, int slab_type)
{
    if (room_corresponding_slab(room_kind) == slab_type)
    {
        return true;
    }
    return false;
}

int check_room_at_slab_loose(PlayerNumber plyr_idx, RoomKind rkind, MapSlabCoord slb_x, MapSlabCoord slb_y, int looseness)
{
    // looseness:
    // don't allow tile = 0
    // valid tile to place room = 1 (i.e. tile owned by current player, and is claimed path)
    // allow same room type = 2
    // allow other room types = 3
    // allow gems = 4
    // allow gold = 5
    // allow liquid = 6
    // allow rock = 7
    // allow path = 8
    // allow path claimed by others = 9

    struct SlabMap* slb = get_slabmap_block(slb_x, slb_y);
    int result = 0;
    if (rkind == RoK_BRIDGE)
    {
        result = (slab_kind_is_liquid(slb->kind) && slab_by_players_land(plyr_idx, slb_x, slb_y)); // 0 or 1
    }
    else
    {
        if (slb->kind == SlbT_CLAIMED)
        {
            if (slabmap_owner(slb) == plyr_idx)
            {
                result = 1; // valid tile
            }
            else
            {
                result = 9; // claimed dirt owned by other player
            }
        }
        else
        {
            if (slab_is_wall(slb_x, slb_y))
            {
                if (slb->kind == SlbT_GEMS)
                {
                    result = 4;
                }
                else if (slb->kind == SlbT_GOLD)
                {
                    result = 5;
                }
                else if (slb->kind == SlbT_ROCK)
                {
                    result = 7; // is unbreakable rock
                }
            }
            else if (slab_kind_is_liquid(slb->kind))
            {
                result = 6; // is water or lava
            }
            else if (slb->kind == SlbT_PATH)
            {
                result = 8; //unclaimed path
            }
            else if (slabmap_owner(slb) == plyr_idx)
            {
                int slab_type_from_room_kind = room_corresponding_slab(rkind);
                
                if (slab_type_from_room_kind == slb->kind)
                {
                    result = 3; // same room type
                }
                else if (slab_type_from_room_kind > 0)
                {
                    result = 2; // different room type
                }
                
            }
        }
    }
    if (result > looseness)
    {
        // adjusting the "looseness" that is passed to this function allows different slab types to be considered "valid" tiles to place a room, for the purposes of finding a room.
        // A room is checked for valid/invalid tiles later in the process, before it is shown to the user with the bounding box.
        result = 0;
    }
    return result;
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

TbBool can_afford_room(PlayerNumber plyr_idx, RoomKind rkind, int slab_count)
{
    struct PlayerInfo* player = get_player(plyr_idx);
    struct Dungeon* dungeon = get_players_dungeon(player);
    struct RoomStats* rstat = room_stats_get_for_kind(rkind);
    return (slab_count * rstat->cost <= dungeon->total_money_owned);
}

int calc_distance_from_centre(int totalDistance, TbBool offset)
{
    return ((totalDistance - 1 + offset) / 2);
}

int can_build_room_of_dimensions_loose(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y, int width, int height, int *invalid_blocks, int room_discovery_looseness)
{
    MapCoord buildx;
    MapCoord buildy;
    int count = 0;
    (*invalid_blocks) = 0;
    int leftExtent = slb_x - calc_distance_from_centre(width,0);
    int rightExtent = slb_x + calc_distance_from_centre(width,(width % 2 == 0));
    int topExtent = slb_y - calc_distance_from_centre(height,0);
    int bottomExtent = slb_y + calc_distance_from_centre(height,(height % 2 == 0));
    
    for (buildy = topExtent; buildy <= bottomExtent; buildy++)
    {
        for (buildx = leftExtent; buildx <= rightExtent; buildx++)
        {
            int room_check = check_room_at_slab_loose(plyr_idx, rkind, buildx, buildy, room_discovery_looseness);
            if (room_check > 0)
            {
                count++;
            }
            if (room_check > 1)
            {
                (*invalid_blocks)++;
            }
        }
    }
    return count;
}

struct RoomMap create_box_room(struct RoomMap room, int width, int height, int centre_x, int centre_y)
{
    TbBool blank_room_grid[MAX_ROOM_WIDTH][MAX_ROOM_WIDTH] = {{false}};
    memcpy(&room.room_grid, &blank_room_grid, sizeof(blank_room_grid));
    room.left   = centre_x - calc_distance_from_centre(width,0);
    room.right  = centre_x + calc_distance_from_centre(width,(width % 2 == 0));
    room.top    = centre_y - calc_distance_from_centre(height,0);
    room.bottom = centre_y + calc_distance_from_centre(height,(height % 2 == 0));
    room.width = width;
    room.height = height;
    room.slabCount = room.width * room.height;
    room.centreX = centre_x;
    room.centreY = centre_y;
    return room;
}

int can_build_room_of_dimensions(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y, int width, int height, TbBool full_check)
{
    MapCoord buildx;
    MapCoord buildy;
    int count = 0;
    int leftExtent = slb_x - calc_distance_from_centre(width,0);
    int rightExtent = slb_x + calc_distance_from_centre(width,(width % 2 == 0));
    int topExtent = slb_y - calc_distance_from_centre(height,0);
    int bottomExtent = slb_y + calc_distance_from_centre(height,(height % 2 == 0));
    
    for (buildy = topExtent; buildy <= bottomExtent; buildy++)
    {
        for (buildx = leftExtent; buildx <= rightExtent; buildx++)
        {
            if (full_check)
            {
                if (can_build_room_at_slab(plyr_idx, rkind, buildx, buildy))
                {
                    count++;
                }
            }
            else
            {
                if (can_build_room_at_slab_fast(plyr_idx, rkind, buildx, buildy))
                {
                    count++;
                }
            }
        }
    }
    if (full_check)
    {
        if (!can_afford_room(plyr_idx, rkind, count))
        {
            return 0;
        }
    }
    return count;
}

int can_build_fancy_room(PlayerNumber plyr_idx, RoomKind rkind, struct RoomMap room)
{
    if (!can_afford_room(plyr_idx, rkind, room.slabCount))
    {
        return 0;
    }
    return room.slabCount;
}

void test_rooms_from_biggest_to_smallest(struct RoomQuery *room_query)
{
    TbBool findCorridors = room_query->findCorridors;
    struct RoomMap current_biggest_room = room_query->best_room;
    MapSlabCoord centre_x = room_query->centre_x, centre_y = room_query->centre_y;
    MapSlabCoord cursor_x = room_query->cursor_x, cursor_y = room_query->cursor_y;
    int max_width = room_query->maxRoomWidth;
    int min_width, min_height;
    if (findCorridors)
    {
        max_width += 1; //(+ 1 to check for corridors)// Check for 10x10, so as to detect corridors
        int distance_from_cursor_x = max(cursor_x, centre_x) - min(cursor_x, centre_x);
        int distance_from_cursor_y = max(cursor_y, centre_y) - min(cursor_y, centre_y);
        distance_from_cursor_x = min(distance_from_cursor_x, max_width);
        distance_from_cursor_y = min(distance_from_cursor_y, max_width);
        min_width = max(room_query->minRoomWidth, ((distance_from_cursor_x * 2)));
        min_height = max(room_query->minRoomHeight, ((distance_from_cursor_y * 2)));
    }
    else // we have found a corridor, and want to find the nearest square room to the cursor that fits in the found corridor
    {
        min_width = min_height = max_width;
    }
    float minimumRatio = room_query->minimumRatio;
    float minimumComparisonRatio = room_query->minimumComparisonRatio;
    struct RoomMap best_corridor = room_query->best_corridor;
    int roomarea = 0;

    //don't check for rooms when they can't be found
    if ((room_query->mode & 2) == 2)
    {
        if ((check_room_at_slab_loose(room_query->plyr_idx, room_query->rkind, centre_x, centre_y, room_query->room_discovery_looseness)  + room_query->leniency) <= 0)
        {
            return;
        }
    }
    else if ((can_build_room_at_slab_fast(room_query->plyr_idx, room_query->rkind, centre_x, centre_y) + room_query->leniency) <= 0)
    {
        return;
    }

    // for a tile, with a given X (centre_x) and Y (centre_y) coordinate :- loop through the room sizes, from biggest width/height to smallest width/height
    for (int w = max_width; w >= min_width; w--)
    {
        if ((w * max_width) < current_biggest_room.slabCount) // || (findCorridors && ((w * max_width) < best_corridor.slabCount)))
        {   // sanity check, to stop pointless iterations of the loop
            break; 
        }

        for (int h = max_width; h >= min_height; h--)
        {
            if ((w * h) < current_biggest_room.slabCount) // || (findCorridors && ((w * h) < best_corridor.slabCount)))
            {   // sanity check, to stop pointless iterations of the loop
                break;
            }
            int slabs = w * h;
            // check aspect ratio of the new room, and if the room w/h is = 10
            if ((((min(w,h) * 1.0) / (max(w,h) * 1.0)) < minimumComparisonRatio) || (max(w,h) >= 10))
            {   // this is a corridor
                if (!findCorridors || (slabs <= best_corridor.slabCount))
                {   // skip small corridors (this is trigger most of the time, as we either have a large corridor (1st one found) or we are not finding any corridors.
                    continue;
                }
            }
            // get the extents of the current room
            int leftExtent   = centre_x - calc_distance_from_centre(w,0);
            int rightExtent  = centre_x + calc_distance_from_centre(w,(w % 2 == 0));
            int topExtent    = centre_y - calc_distance_from_centre(h,0);
            int bottomExtent = centre_y + calc_distance_from_centre(h,(h % 2 == 0));
            // check if cursor is not in the current room
            if (!(cursor_x >= leftExtent && cursor_x <= rightExtent && cursor_y >= topExtent && cursor_y <= bottomExtent))
            {
                continue; // not a valid room
            }
            // check to see if the room collides with any walls (etc)
            int invalidBlocks = 0;
            if ((room_query->mode & 2) == 2)
            {
                roomarea = can_build_room_of_dimensions_loose(room_query->plyr_idx, room_query->rkind, centre_x, centre_y, w, h, &invalidBlocks, room_query->room_discovery_looseness);
            }
            else
            {
                roomarea = can_build_room_of_dimensions(room_query->plyr_idx, room_query->rkind, centre_x, centre_y, w, h, false);
            }
            if (roomarea >= (slabs - room_query->leniency))
            {
                // check aspect ratio of the new room, and if the room w/h is = 10
                if ((((min(w,h) * 1.0) / (max(w,h) * 1.0)) < minimumComparisonRatio) || ((max(w,h) >= 10)))
                { 
                    // this is a corridor
                    if (roomarea > best_corridor.slabCount)
                    {
                        best_corridor.slabCount = roomarea;
                        best_corridor.centreX = centre_x;
                        best_corridor.centreY = centre_y;
                        best_corridor.width = w;
                        best_corridor.height = h;
                        best_corridor.left = leftExtent;
                        best_corridor.right = rightExtent;
                        best_corridor.top = topExtent;
                        best_corridor.bottom = bottomExtent;
                        best_corridor.isRoomABox = true;
                    }
                    break;
                }
                //else if (((roomarea * room_query->slabCost) <= room_query->moneyLeft) && (((min(w,h) * 1.0) / (max(w,h) * 1.0)) >= minimumRatio))
                else if (((min(w,h) * 1.0) / (max(w,h) * 1.0)) >= minimumRatio)
                { 
                    TbBool isCorridorHorizontal = (best_corridor.width >= best_corridor.height);
                    TbBool isInCorridor = isCorridorHorizontal ? (topExtent == best_corridor.top && bottomExtent == best_corridor.bottom) : (leftExtent == best_corridor.left && rightExtent == best_corridor.right);
                    if (!isInCorridor || !findCorridors)
                    {
                        // this is a room
                        if (roomarea > current_biggest_room.slabCount)
                        {
                            current_biggest_room.slabCount = roomarea;
                            current_biggest_room.invalidBlocksCount = invalidBlocks;
                            current_biggest_room.centreX = centre_x;
                            current_biggest_room.centreY = centre_y;
                            current_biggest_room.width = w;
                            current_biggest_room.height = h;
                            current_biggest_room.left = leftExtent;
                            current_biggest_room.right = rightExtent;
                            current_biggest_room.top = topExtent;
                            current_biggest_room.bottom = bottomExtent;
                            current_biggest_room.isRoomABox = true;
                            room_query->foundRoom = true;
                        }
                        break;
                    }
                }
            }
        }
    } // end loop of room sizes
    
    room_query->best_corridor = best_corridor;
    room_query->best_room = current_biggest_room;
    if (findCorridors && best_corridor.width > 1 && best_corridor.height > 1)
    {
        TbBool isCorridorAreaSmaller = (best_corridor.slabCount < current_biggest_room.slabCount);
        TbBool isCorridorHorizontal = (best_corridor.width >= best_corridor.height);
        TbBool isCorridorHeightSmaller = (min(best_corridor.width, best_corridor.height) < (isCorridorHorizontal ? current_biggest_room.height : current_biggest_room.width));
        room_query->isCorridor = (isCorridorHeightSmaller || isCorridorAreaSmaller) ? false : true; // small 2xA corridors might be found as part of a composite room check, these are ignored along with any other corridors (and any potential subrooms that might be in that corridor)
        if (room_query->isCorridor)
        {
            room_query->foundRoom = false;
        }
    }
}

void find_room_within_radius(struct RoomQuery *room_query)
{
    // Loop through all of the tiles in a search area, and then test for rooms centred on each of these tiles.

    int direction = 0; // current direction; 0=RIGHT, 1=DOWN, 2=LEFT, 3=UP
    int tile_counter = 0; // the number of tiles that have been processed
    int chain_size = 1; // a spiral is constructed out of chains, increasing in size around the centre
    int searchWidth = room_query->maxRoomWidth + (room_query->maxRoomWidth % 2 == 0);
    int max_count = searchWidth * searchWidth; // total number of tiles to iterate over
    int chain_position = 0; // position along the current chain
    int chain_iterations = 0; // every 2 iterations, the chain size is increased

    // starting point (centre of area - start at current cursor position)
    int x = room_query->cursor_x; // current position; x
    int y = room_query->cursor_y; // current position; y

    do
    {
        tile_counter++;
        room_query->centre_x = x;
        room_query->centre_y = y;
        test_rooms_from_biggest_to_smallest(room_query);
        if (chain_position < chain_size) // move along the current chain
        {
            chain_position++;
        }
        else // we need to rotate and start the next chain
        {
            chain_position = 1; // only use 0 to place the first centre tile
            chain_iterations++;
            if (chain_iterations % 2 == 0) // 2 chains of movement of the same length, then increase chain length by 1
            {
                chain_size++;
            }
            direction = (direction + 1) % 4; // switch direction (Clockwise rotation)
        }
        switch (direction) // increase x/y based on direction of the chain
        {
            case 0: y = y + 1; break; // moving Right
            case 1: x = x + 1; break; // moving Down
            case 2: y = y - 1; break; // moving Left
            case 3: x = x - 1; break; // moving Up
        }
    } while (tile_counter < max_count); // loop through every tile in the search area
}

struct RoomMap check_slabs_in_room(struct RoomMap room, PlayerNumber plyr_idx, RoomKind rkind, short slabCost)
{
    room.slabCount = 0;
    room.invalidBlocksCount = 0;
    for (int y = 0; y < room.height; y++)
    {
        int current_y = room.top + y;
        for (int x = 0; x < room.width; x++)
        {
            int current_x = room.left + x;
            if (room.isRoomABox || room.room_grid[x][y] == true) //only check slabs in the room
            {
                if (can_build_room_at_slab(plyr_idx, rkind, current_x, current_y))
                {
                    room.room_grid[x][y] = true;
                    room.slabCount++;
                }
                else
                {
                    room.room_grid[x][y] = false;
                    room.invalidBlocksCount++;
                }
            }
        }
    }
    room.totalRoomCost = room.slabCount * slabCost;
    if (room.slabCount != (room.width * room.height))
    {
        room.isRoomABox = false;
    }
    return room;
}

void add_to_composite_room(struct RoomQuery *room_query, struct RoomQuery *meta_room)
{
    // find the extents of the meta room
    int minX = meta_room->best_room.left;
    int maxX = meta_room->best_room.right;
    int minY = meta_room->best_room.top;
    int maxY = meta_room->best_room.bottom;
    if (((maxX - minX) < MAX_ROOM_WIDTH) && (room_query->best_room.left < minX))
    {
        minX = room_query->best_room.left;
    }
    if (((maxX - minX) < MAX_ROOM_WIDTH) && (room_query->best_room.right > maxX))
    {
        maxX = room_query->best_room.right;
    }
    if (((maxY - minY) < MAX_ROOM_WIDTH) && (room_query->best_room.top < minY))
    {
        minY = room_query->best_room.top;
    }
    if (((maxY - minY) < MAX_ROOM_WIDTH) && (room_query->best_room.bottom > maxY))
    {
        maxY = room_query->best_room.bottom;
    }
    int metaRoomWidth = (maxX - minX + 1);
    int metaRoomHeight = (maxY - minY + 1);
    // idiot check for empty room
    if ((metaRoomWidth * metaRoomHeight) <= 1) 
    {
        return;
    }
    // else, populate best_room
    struct RoomMap best_room = meta_room->best_room;
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
    // loop through all of the tiles within the extents of the meta room, and check if it is found in the current sub room
    for (int y = 0; y < best_room.height; y++)
    {
        int current_y = minY + y;
        for (int x = 0; x < best_room.width; x++)
        {
            int current_x = minX + x;
            best_room.room_grid[x][y] = false; // set to false by default
            TbBool isSlabInMetaRoom = (current_x >= meta_room->best_room.left && current_x <= meta_room->best_room.right && current_y >= meta_room->best_room.top && current_y <= meta_room->best_room.bottom);
            TbBool isSlabInNewRoom = (current_x >= room_query->best_room.left && current_x <= room_query->best_room.right && current_y >= room_query->best_room.top && current_y <= room_query->best_room.bottom);
            if (isSlabInMetaRoom)
            {
                best_room.room_grid[x][y] = meta_room->best_room.room_grid[current_x-meta_room->best_room.left][current_y-meta_room->best_room.top];
            }
            if (isSlabInNewRoom && (best_room.room_grid[x][y] == false))
            {
                best_room.room_grid[x][y] = true;
            }
             best_room.slabCount += best_room.room_grid[x][y];
        }
    }
    if (best_room.slabCount != (best_room.width * best_room.height))
    {
        best_room.isRoomABox = false;
    }
    meta_room->best_room = best_room;
}

void find_composite_room(struct RoomQuery *room_query)
{
    //struct RoomQuery bestRooms[room_query.subRoomCheckCount];
    new_room_query = (*room_query);
    struct RoomQuery meta_room = new_room_query;
    int bestRoomsCount = 0;
    int mode = room_query->mode;
    int subRoomCheckCount = room_query->subRoomCheckCount;

    // Find the biggest room
    // loop through the room_query.subRoomCheckCount sub rooms, that are used to construct the meta room (mode 32 only, otherwise it only loops once)
    do
    {
        find_room_within_radius(&new_room_query);
        //room_query->maxRoomRadius = room_query->maxRoomRadius - 1;
        //room_query->maxRoomWidth = (room_query->maxRoomRadius * 2) - 1;
        //new_room_query.findCorridors = false;
        //new_room_query.bestRoomsCount = bestRoomsCount;
        if (((mode & 32) == 32) && (bestRoomsCount < subRoomCheckCount))
        {
            // Adjust leniency counter
            if ((new_room_query.leniency > 0) && new_room_query.best_room.slabCount > 1) // make sure we found a subroom, and then adjust leniency allowance as needed
            {
                int usedLeniency = new_room_query.best_room.invalidBlocksCount;
                if ((usedLeniency > 0) && !new_room_query.isCorridor)
                {
                    new_room_query.leniency = room_query->leniency - usedLeniency;
                    new_room_query.InvalidBlocksIgnored += usedLeniency;
                }
            }
            if (bestRoomsCount == 0) // first time through only
            {
                meta_room = new_room_query; // establish the first room
                if (!meta_room.foundRoom && !meta_room.isCorridor)
                {
                    return; // if the first room found is a single tile, exit now
                }
                meta_room.best_room = room_query->best_room;
            }
            // add new subroom to meta room
            if (new_room_query.foundRoom)
            {
                
                if (!(meta_room.best_room.slabCount == new_room_query.best_room.slabCount && meta_room.best_room.width == new_room_query.best_room.width && meta_room.best_room.height == new_room_query.best_room.height))
                {
                    add_to_composite_room(&new_room_query, &meta_room);
                    meta_room.isCorridor = false;
                }
            }
            bestRoomsCount++;
            if (bestRoomsCount >= subRoomCheckCount)
            {
                new_room_query = meta_room;
            }
            else
            {   // set up settings for next pass for subrooms
                //int correct_index = ((bestRoomsCount > 3) ? bestRoomsCount - 3 : 0); // we want to compare to the previously stored widest/tallest room
                if (bestRoomsCount % 3 == 1) // check for wider rooms
                {
                    new_room_query.minRoomWidth = min(room_query->maxRoomWidth,max(meta_room.best_room.width + 1, room_query->minRoomWidth));
                    new_room_query.minRoomHeight = room_query->minRoomWidth;
                    //new_room_query.minimumRatio = (1.0 / 4.0);
                    new_room_query.minimumRatio = room_query->minimumRatio;
                }
                else if (bestRoomsCount % 3 == 2)  // check for taller rooms
                {
                    new_room_query.minRoomWidth = room_query->minRoomWidth;
                    new_room_query.minRoomHeight = min(room_query->maxRoomWidth,max(meta_room.best_room.height + 1, room_query->minRoomWidth));
                    //new_room_query.minimumRatio = (1.0 / 4.0);
                    new_room_query.minimumRatio = room_query->minimumRatio;
                }
                else //check for square room (only worth running once)
                {
                    new_room_query.minRoomWidth = room_query->minRoomWidth;
                    new_room_query.minRoomHeight = room_query->minRoomWidth;
                    new_room_query.minimumRatio = 1.0;
                }
                // finish meta room checks
                new_room_query.best_room.slabCount = 1;
                new_room_query.best_room.invalidBlocksCount = 0;
                
            }
        }
    } while(((mode & 32) == 32) && (bestRoomsCount < subRoomCheckCount)); // loop again, if in mode 32, for the extra room checks

    // if the "best room" is a corridor, then grab the best AxA room in the corridor.
    if (new_room_query.isCorridor) //  if the "best room" is a corridor, then grab the best AxA room in the corridor.
    {
        new_room_query.moneyLeft = room_query->moneyLeft;
        new_room_query.maxRoomWidth = min(new_room_query.best_corridor.width, new_room_query.best_corridor.height);
        new_room_query.minRoomHeight = new_room_query.minRoomWidth = new_room_query.maxRoomWidth;
        new_room_query.minimumRatio = 1.0;
        new_room_query.isCorridor = false;
        new_room_query.findCorridors = false;
        new_room_query.best_room.slabCount = 1;
        new_room_query.best_corridor.slabCount = 1;
        //new_room_query.best_corridor = room_query->best_corridor;
        /*if ((new_room_query.best_corridor.width * new_room_query.best_corridor.height) >= ((room_query->maxRoomWidth - 1) * (room_query->maxRoomWidth - 1))) // 10x10 room = open plan
        {   // so return a 5x5 room?? 9x9???
            new_room_query.maxRoomWidth = room_query->maxRoomWidth;
            new_room_query.minRoomWidth = room_query->minRoomWidth;
            new_room_query.maxRoomRadius = 0;
        }
        else
        {*/
           // grab the closest AxA room to the cursor, that is within the detected corridor
            
        //}
        
        find_room_within_radius(&new_room_query);
    }
    (*room_query) = new_room_query;
}

struct RoomMap get_biggest_room(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord cursor_x, MapSlabCoord cursor_y, short slabCost, int totalMoney, int mode, int room_discovery_looseness)
{
    int maxRoomWidth = 9; // 9x9 Room
    int minRoomWidth = 2; // Don't look for rooms smaller than 2x2
    float minimumRatio = (1.0 / 3.0);
    float minimumComparisonRatio = minimumRatio;
    int subRoomCheckCount = 6; // the number of sub-rooms to combine in to a final meta-room
    int bestRoomsCount = 0;
    // Set default "room" - i.e. 1x1 slabs, centred on the cursor
    struct RoomMap best_room = { {{false}}, 1, true, 1, 1, cursor_x, cursor_y, cursor_x, cursor_y, cursor_x, cursor_y, slabCost, 0 };
    //int leniency = (((mode & 16) == 16)) ? tolerance : 0; // mode=16 :- (setting to 1 would allow e.g. 1 dirt block in the room)
    int leniency = 0;
    struct RoomMap best_corridor = best_room;
    if (room_discovery_looseness > 0)
    {
        mode |= 2;
    }
    //don't check for rooms when they can't be found
    if ((mode & 2) == 2)
    {
        int room_check = check_room_at_slab_loose(plyr_idx, rkind, cursor_x, cursor_y, room_discovery_looseness);
        if (room_check == 0 || room_check == 6) //reject invalid and liquid slabs
        {
            return best_room;
        }
    }
    else if ((can_build_room_at_slab_fast(plyr_idx, rkind, cursor_x, cursor_y) + leniency) <= 0)
    {
        return best_room;
    }
    struct RoomQuery room_query = { slabCost, totalMoney, mode, 0, maxRoomWidth, minRoomWidth, minRoomWidth, subRoomCheckCount, bestRoomsCount, best_room, best_corridor, cursor_x, cursor_y, cursor_x, cursor_y, plyr_idx, rkind, minimumRatio, minimumComparisonRatio, false, false, leniency, totalMoney, 0, true, false, room_discovery_looseness};
    find_composite_room(&room_query);
    room_query.best_room = check_slabs_in_room(room_query.best_room, plyr_idx, rkind, slabCost);
    if (room_query.best_room.slabCount > 0)
    {
        return room_query.best_room;
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
