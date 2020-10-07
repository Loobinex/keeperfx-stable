/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file roomspace.c
 *     Functions to facilitate the use of a "room space" (an area of many slabs)
 *     instead of a single slab when placing and selling rooms.
 
 * @par Purpose:
 *     Establishes a "room space" as a 2D array of booleans, where a value of 1
 *     represents the slabs that are in the "room space", and a value of 0 
 *     represents the slabs that are not in the "room space".
 * @par Comment:
 *     None.
 * @author   KeeperFx Team
 * @date     10 Jun 2020 - 07 Oct 2020
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "game_legacy.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
int user_defined_roomspace_width = DEFAULT_USER_ROOMSPACE_WIDTH;
int roomspace_detection_looseness = DEFAULT_USER_ROOMSPACE_DETECTION_LOOSENESS;
struct RoomSpace render_roomspace = { {{false}}, 1, true, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
/******************************************************************************/
TbBool can_afford_roomspace(PlayerNumber plyr_idx, RoomKind rkind, int slab_count)
{
    struct PlayerInfo* player = get_player(plyr_idx);
    struct Dungeon* dungeon = get_players_dungeon(player);
    struct RoomStats* rstat = room_stats_get_for_kind(rkind);
    return (slab_count * rstat->cost <= dungeon->total_money_owned);
}

int calc_distance_from_roomspace_centre(int total_distance, TbBool offset)
{
    return ((total_distance - 1 + offset) / 2);
}

int can_build_roomspace_of_dimensions_loose(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y, int width, int height, int *invalid_blocks, int roomspace_discovery_looseness)
{
    MapCoord buildx;
    MapCoord buildy;
    int count = 0;
    (*invalid_blocks) = 0;
    int leftExtent = slb_x - calc_distance_from_roomspace_centre(width,0);
    int rightExtent = slb_x + calc_distance_from_roomspace_centre(width,(width % 2 == 0));
    int topExtent = slb_y - calc_distance_from_roomspace_centre(height,0);
    int bottomExtent = slb_y + calc_distance_from_roomspace_centre(height,(height % 2 == 0));
    
    for (buildy = topExtent; buildy <= bottomExtent; buildy++)
    {
        for (buildx = leftExtent; buildx <= rightExtent; buildx++)
        {
            int room_check = check_room_at_slab_loose(plyr_idx, rkind, buildx, buildy, roomspace_discovery_looseness);
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

struct RoomSpace create_box_roomspace(struct RoomSpace roomspace, int width, int height, int centre_x, int centre_y)
{
    TbBool blank_slab_grid[MAX_ROOMSPACE_WIDTH][MAX_ROOMSPACE_WIDTH] = {{false}};
    memcpy(&roomspace.slab_grid, &blank_slab_grid, sizeof(blank_slab_grid));
    roomspace.left   = centre_x - calc_distance_from_roomspace_centre(width,0);
    roomspace.right  = centre_x + calc_distance_from_roomspace_centre(width,(width % 2 == 0));
    roomspace.top    = centre_y - calc_distance_from_roomspace_centre(height,0);
    roomspace.bottom = centre_y + calc_distance_from_roomspace_centre(height,(height % 2 == 0));
    roomspace.width = width;
    roomspace.height = height;
    roomspace.slab_count = roomspace.width * roomspace.height;
    roomspace.centreX = centre_x;
    roomspace.centreY = centre_y;
    return roomspace;
}

int can_build_roomspace_of_dimensions(PlayerNumber plyr_idx, RoomKind rkind,
    MapSlabCoord slb_x, MapSlabCoord slb_y, int width, int height, TbBool full_check)
{
    MapCoord buildx;
    MapCoord buildy;
    int count = 0;
    int leftExtent = slb_x - calc_distance_from_roomspace_centre(width,0);
    int rightExtent = slb_x + calc_distance_from_roomspace_centre(width,(width % 2 == 0));
    int topExtent = slb_y - calc_distance_from_roomspace_centre(height,0);
    int bottomExtent = slb_y + calc_distance_from_roomspace_centre(height,(height % 2 == 0));
    
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
        if (!can_afford_roomspace(plyr_idx, rkind, count))
        {
            return 0;
        }
    }
    return count;
}

int can_build_fancy_roomspace(PlayerNumber plyr_idx, RoomKind rkind, struct RoomSpace roomspace)
{
    if (!can_afford_roomspace(plyr_idx, rkind, roomspace.slab_count))
    {
        return 0;
    }
    return roomspace.slab_count;
}

struct RoomSpace check_slabs_in_roomspace(struct RoomSpace roomspace, PlayerNumber plyr_idx, RoomKind rkind, short rkind_cost)
{
    roomspace.slab_count = 0;
    roomspace.invalid_slabs_count = 0;
    for (int y = 0; y < roomspace.height; y++)
    {
        int current_y = roomspace.top + y;
        for (int x = 0; x < roomspace.width; x++)
        {
            int current_x = roomspace.left + x;
            if (roomspace.is_roomspace_a_box || roomspace.slab_grid[x][y] == true) // only check slabs in the roomspace
            {
                if (can_build_room_at_slab(plyr_idx, rkind, current_x, current_y))
                {
                    roomspace.slab_grid[x][y] = true;
                    roomspace.slab_count++;
                }
                else
                {
                    roomspace.slab_grid[x][y] = false;
                    roomspace.invalid_slabs_count++;
                }
            }
        }
    }
    roomspace.total_roomspace_cost = roomspace.slab_count * rkind_cost;
    if (roomspace.slab_count != (roomspace.width * roomspace.height))
    {
        roomspace.is_roomspace_a_box = false;
    }
    return roomspace;
}

int can_build_roomspace(PlayerNumber plyr_idx, RoomKind rkind, struct RoomSpace roomspace)
{
    int canbuild = 0;
    if (roomspace.is_roomspace_a_box)
    {
        canbuild = can_build_roomspace_of_dimensions(plyr_idx, rkind, roomspace.centreX, roomspace.centreY, roomspace.width, roomspace.height, true);
    }
    else
    {
        canbuild = can_build_fancy_roomspace(plyr_idx, rkind, roomspace);
    }
    return canbuild;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
