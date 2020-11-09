/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file console_cmd.c
 *     
 * @par Purpose:
 *     Define various console commans
 * @par Comment:
 *     None
 * @author   Sim
 * @date     07 Jul 2020 - 07 Jul 2020
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "console_cmd.h"
#include "globals.h"

#include "actionpt.h"
#include "bflib_datetm.h"
#include "bflib_sound.h"
#include "config.h"
#include "config_campaigns.h"
#include "config_magic.h"
#include "config_rules.h"
#include "config_terrain.h"
#include "config_trapdoor.h"
#include "creature_instances.h"
#include "creature_jobs.h"
#include "creature_states.h"
#include "creature_states_hero.h"
#include "dungeon_data.h"
#include "frontend.h"
#include "frontmenu_ingame_evnt.h"
#include "frontmenu_ingame_tabs.h"
#include "game_legacy.h"
#include "game_merge.h"
#include "gui_boxmenu.h"
#include "gui_msgs.h"
#include "gui_soundmsgs.h"
#include "keeperfx.hpp"
#include "map_blocks.h"
#include "map_columns.h"
#include "map_utils.h"
#include "math.h"
#include "music_player.h"
#include "packets.h"
#include "player_computer.h"
#include "player_instances.h"
#include "player_utils.h"
#include "room_data.h"
#include "room_util.h"
#include "slab_data.h"
#include "thing_factory.h"
#include "thing_list.h"
#include "thing_objects.h"
#include "thing_navigate.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct GuiBoxOption cmd_comp_procs_data[COMPUTER_PROCESSES_COUNT + 3] = {
  {"!", 1, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0 }
  };

static char cmd_comp_procs_label[COMPUTER_PROCESSES_COUNT + 2][COMMAND_WORD_LEN + 8];

struct GuiBoxOption cmd_comp_checks_data[COMPUTER_CHECKS_COUNT + 1] = { 0 };
static char cmd_comp_checks_label[COMPUTER_CHECKS_COUNT][COMMAND_WORD_LEN + 8];

struct GuiBoxOption cmd_comp_events_data[COMPUTER_EVENTS_COUNT + 1] = { 0 };
  static char cmd_comp_events_label[COMPUTER_EVENTS_COUNT][COMMAND_WORD_LEN + 8];

static long cmd_comp_procs_click(struct GuiBox *gbox, struct GuiBoxOption *goptn, unsigned char btn, long *args)
{
    struct Computer2 *comp;
    comp = get_computer_player(args[0]);
    struct ComputerProcess* cproc = &comp->processes[args[1]];

    if (cproc->flags & ComProc_Unkn0020)
        message_add_fmt(args[0], "resuming %s", cproc->name?cproc->name:"(null)");
    else
        message_add_fmt(args[0], "suspending %s", cproc->name?cproc->name:"(null)");
    
    cproc->flags ^= ComProc_Unkn0020; // Suspend, but do not update running time
    return 1;
}

static long cmd_comp_procs_update(struct GuiBox *gbox, struct GuiBoxOption *goptn, long *args)
{
    struct Computer2 *comp = get_computer_player(args[0]);
    int i = 0;

    for (; i < args[1]; i++)
    {
        struct ComputerProcess* cproc = &comp->processes[i];
        if (cproc != NULL)
        {
            char *label = (char*)goptn[i].label;
            sprintf(label, "%02lx", cproc->flags);
            label[2] = ' ';
        }
    }

    sprintf(cmd_comp_procs_label[i], "comp=%d, wait=%ld", 0, comp->gameturn_wait);
    return 1;
}

static long cmd_comp_checks_update(struct GuiBox *gbox, struct GuiBoxOption *goptn, long *args)
{
    struct Computer2 *comp = get_computer_player(args[0]);
    int i = 0;

    for (; i < args[1]; i++)
    {
        struct ComputerCheck* check = &comp->checks[i];
        if (check != NULL)
        {
            char *label = (char*)goptn[i].label;
            sprintf(label, "%02lx", check->flags);
            label[2] = ' ';
        }
    }
    return 1;
}

int cmd_comp_list(PlayerNumber plyr_idx, int max_count,
    struct GuiBoxOption *data_list, char label_list[][COMMAND_WORD_LEN + 8],
    const char *(*get_name)(struct Computer2 *, int),
    unsigned long (*get_flags)(struct Computer2 *, int),
    Gf_OptnBox_4Callback click_fn
    )
{
    close_creature_cheat_menu();
    //gui_cheat_box
    int i = 0;
    struct Computer2 *comp;
    comp = get_computer_player(plyr_idx);
    for (; i < max_count; i++)
    {
        unsigned long flags = get_flags(comp, i); 
        const char *name = get_name(comp, i);
        if (name == NULL)
            sprintf(label_list[i], "%02lx %s", flags, "(null2)");  
        else
          sprintf(label_list[i], "%02lx %s", flags, name);
        data_list[i].label = label_list[i];

        data_list[i].numfield_4 = 1;
        data_list[i].callback = click_fn;
        data_list[i].field_D = plyr_idx;
        data_list[i].field_11 = max_count;
        data_list[i].field_19 = plyr_idx;
        data_list[i].field_1D = i;
    }
    data_list[i].label = "!";
    data_list[i].numfield_4 = 0;
    return i;
}

static const char *get_process_name(struct Computer2 *comp, int i)
{
    return comp->processes[i].name;
}
static unsigned long  get_process_flags(struct Computer2 *comp, int i)
{
    return comp->processes[i].flags;
}
static void cmd_comp_procs(PlayerNumber plyr_idx)
{
    int i = cmd_comp_list(plyr_idx, COMPUTER_PROCESSES_COUNT, 
        cmd_comp_procs_data, cmd_comp_procs_label,
        &get_process_name, &get_process_flags,
        &cmd_comp_procs_click);

    cmd_comp_procs_data[0].active_cb = &cmd_comp_procs_update;
    
    cmd_comp_procs_data[i].label = "======";
    cmd_comp_procs_data[i].numfield_4 = 1;
    i++;
    cmd_comp_procs_data[i].label = cmd_comp_procs_label[COMPUTER_PROCESSES_COUNT];
    cmd_comp_procs_data[i].numfield_4 = 1;
    i++;
    cmd_comp_procs_data[i].label = "!";
    cmd_comp_procs_data[i].numfield_4 = 0;

    gui_cheat_box = gui_create_box(my_mouse_x, 20, cmd_comp_procs_data);
}

static const char *get_event_name(struct Computer2 *comp, int i)
{
    return comp->events[i].name;
}
static unsigned long  get_event_flags(struct Computer2 *comp, int i)
{
    return 0;
}
static void cmd_comp_events(PlayerNumber plyr_idx)
{
    cmd_comp_list(plyr_idx, COMPUTER_EVENTS_COUNT, 
        cmd_comp_events_data, cmd_comp_events_label,
        &get_event_name, &get_event_flags, NULL);
    cmd_comp_events_data[0].active_cb = NULL;

    gui_cheat_box = gui_create_box(my_mouse_x, 20, cmd_comp_events_data);
}


static long cmd_comp_checks_click(struct GuiBox *gbox, struct GuiBoxOption *goptn, unsigned char btn, long *args)
{
    struct Computer2 *comp;
    comp = get_computer_player(args[0]);
    struct ComputerCheck* cproc = &comp->checks[args[1]];

    if (cproc->flags & ComChk_Unkn0001)
        message_add_fmt(args[0], "resuming %s", cproc->name?cproc->name:"(null)");
    else
        message_add_fmt(args[0], "suspending %s", cproc->name?cproc->name:"(null)");
    
    cproc->flags ^= ComChk_Unkn0001;
    return 1;
}
static const char *get_check_name(struct Computer2 *comp, int i)
{
    return comp->checks[i].name;
}
static unsigned long  get_check_flags(struct Computer2 *comp, int i)
{
    return comp->checks[i].flags;
}

static void cmd_comp_checks(PlayerNumber plyr_idx)
{
    cmd_comp_list(plyr_idx, COMPUTER_CHECKS_COUNT, 
        cmd_comp_checks_data, cmd_comp_checks_label,
        &get_check_name, &get_check_flags, &cmd_comp_checks_click);
    cmd_comp_checks_data[0].active_cb = NULL;

    gui_cheat_box = gui_create_box(my_mouse_x, 20, cmd_comp_checks_data);
}

static char *cmd_strtok(char *tail)
{
    char* next = strchr(tail,' ');
    if (next == NULL)
        return next;
    next[0] = '\0';
    next++; // it was space
    while (*next == ' ')
      next++;
    return next;
}

TbBool cmd_exec(PlayerNumber plyr_idx, char *msg)
{
    SYNCDBG(2,"Command %d: %s",(int)plyr_idx, msg);
    const char * parstr = msg + 1;
    const char * pr2str = cmd_strtok(msg + 1);
    const char * pr3str = (pr2str != NULL) ? cmd_strtok(pr2str + 1) : NULL;
    const char * pr4str = (pr3str != NULL) ? cmd_strtok(pr3str + 1) : NULL;
    const char * pr5str = (pr4str != NULL) ? cmd_strtok(pr4str + 1) : NULL;
    struct PlayerInfo* player;
    struct Thing* thing;
    struct Dungeon* dungeon;
    struct Room* room;
    struct Packet* pckt;
    struct SlabMap *slb;
    struct Coord3d pos;
    if (strcasecmp(parstr, "stats") == 0)
    {
      message_add_fmt(plyr_idx, "Now time is %d, last loop time was %d",LbTimerClock(),last_loop_time);
      message_add_fmt(plyr_idx, "clock is %d, requested fps is %d",clock(),game.num_fps);
      return true;
    }
    else if (strcasecmp(parstr, "fps") == 0)
    {
        if (pr2str == NULL)
        {
            message_add_fmt(plyr_idx, "Framerate is %d fps", game.num_fps);
            return true;
        }
        else
        {
            game.num_fps = atoi(pr2str);
            return true;
        }
        return false;
    }
    else if (strcasecmp(parstr, "quit") == 0)
    {
        quit_game = 1;
        exit_keeper = 1;
        return true;
    }
    else if (strcasecmp(parstr, "time") == 0)
    {
        show_game_time_taken(game.play_gameturn);
        return true;
    }
    else if (strcasecmp(parstr, "timer.toggle") == 0)
    {
        game_flags2 ^= GF2_Timer;
        player = get_player(plyr_idx);
        if ( (player->victory_state == VicS_WonLevel) && (timer_enabled) && (TimerGame) )
        {
            dungeon = get_my_dungeon();
            TimerTurns = dungeon->lvstats.hopes_dashed;
        }
        return true;
    }
    else if (strcasecmp(parstr, "timer.switch") == 0)
    {
        TimerGame ^= 1;
        player = get_player(plyr_idx);
        if ( (player->victory_state == VicS_WonLevel) && (timer_enabled) && (TimerGame) )
        {
            dungeon = get_my_dungeon();
            TimerTurns = dungeon->lvstats.hopes_dashed;
        }
        return true;
    }
    else if ( (strcasecmp(parstr, "turn") == 0) || (strcasecmp(parstr, "game.turn") == 0) )
    {
        message_add_fmt(plyr_idx, "turn %ld", game.play_gameturn);
        return true;
    }
    else if (strcasecmp(parstr, "game.kind") == 0)
    {
        message_add_fmt(plyr_idx, "Game kind: %d", game.game_kind);
        return true;
    }
    else if (strcasecmp(parstr, "game.save") == 0)
    {
        long slot_num = atoi(pr2str);
        player = get_player(plyr_idx);
        set_flag_byte(&game.operation_flags,GOF_Paused,true); // games are saved in a paused state
        TbBool result = save_game(slot_num);
        if (result)
        {
            output_message(SMsg_GameSaved, 0, true);
        }
        else
        {
          ERRORLOG("Error in save!");
          // create_error_box(GUIStr_ErrorSaving);
          message_add_fmt(plyr_idx, "Unable to save to slot $d", slot_num);
        }
        set_flag_byte(&game.operation_flags,GOF_Paused,false); // unpause after save attempt
        return result;
    }
    else if (strcasecmp(parstr, "game.load") == 0)
    {
        long slot_num = atoi(pr2str);
        if (is_save_game_loadable(slot_num))
        {
            if (load_game(slot_num))
            {
                player = get_player(plyr_idx);
                set_flag_byte(&game.operation_flags,GOF_Paused,false); // unpause, because games are saved whilst puased
                return true;
            }
            else
            {
                message_add_fmt(plyr_idx, "Unable to load game %d", slot_num);
            }
        }
        else
        {
            message_add_fmt(plyr_idx, "Unable to load game %d", slot_num);
        }
        return false;
    }
    else if (strcasecmp(parstr, "cls") == 0)
    {
        zero_messages();
        return true;
    }
    else if (strcasecmp(parstr, "ver") == 0)
    {
        message_add_fmt(plyr_idx, "%s", PRODUCT_VERSION);
        return true;
    }
    else if ((game.flags_font & FFlg_AlexCheat) != 0)
    {
        if (strcasecmp(parstr, "compuchat") == 0)
        {
            if (pr2str == NULL)
                return false;

            if ((strcasecmp(pr2str,"scarce") == 0) || (strcasecmp(pr2str,"1") == 0))
            {
                for (int i = 0; i < PLAYERS_COUNT; i++)
                {
                    if ((i == game.hero_player_num)
                        || (plyr_idx == game.neutral_player_num))
                        continue;
                    struct Computer2* comp = get_computer_player(i);
                    if (player_exists(get_player(i)) && (!computer_player_invalid(comp)))
                        message_add_fmt(i, "Ai model %d", (int)comp->model);
                }
                gameadd.computer_chat_flags = CChat_TasksScarce;
            } else
            if ((strcasecmp(pr2str,"frequent") == 0) || (strcasecmp(pr2str,"2") == 0))
            {
                message_add_fmt(plyr_idx, "%s", pr2str);
                gameadd.computer_chat_flags = CChat_TasksScarce|CChat_TasksFrequent;
            } else
            {
                message_add(plyr_idx, "none");
                gameadd.computer_chat_flags = CChat_None;
            }
            return true;
        } else if (strcasecmp(parstr, "comp.procs") == 0)
        {
            if (pr2str == NULL)
                return false;
            PlayerNumber id = get_player_number_for_command(pr2str);
            if (id < 0 || id > PLAYERS_COUNT)
                return false;
            cmd_comp_procs(id);
            return true;
        } else if (strcasecmp(parstr, "comp.events") == 0)
        {
            if (pr2str == NULL)
                return false;
            PlayerNumber id = get_player_number_for_command(pr2str);
            if (id < 0 || id > PLAYERS_COUNT)
                return false;
            cmd_comp_events(id);
            return true;
        } else if (strcasecmp(parstr, "comp.checks") == 0)
        {
            if (pr2str == NULL)
                return false;
            PlayerNumber id = get_player_number_for_command(pr2str);
            if (id < 0 || id > PLAYERS_COUNT)
                return false;
            cmd_comp_checks(id);
            return true;
        } else if (strcasecmp(parstr, "reveal") == 0)
        {
            player = get_my_player();
            reveal_whole_map(player);
            return true;
        } else if ( (strcasecmp(parstr, "comp.kill") == 0) || (strcasecmp(parstr, "player.kill") == 0) )
        {
            if (pr2str == NULL)
                return false;
            PlayerNumber id = get_player_number_for_command(pr2str);
            if (id < 0 || id > PLAYERS_COUNT)
                return false;
            thing = get_player_soul_container(id);
            if (thing_is_dungeon_heart(thing))
            {
                thing->health = 0;
                return true;
            }
            return false;
        } else if (strcasecmp(parstr, "comp.me") == 0)
        {
            player = get_player(plyr_idx);
            if (pr2str == NULL)
                return false;
            if (!setup_a_computer_player(plyr_idx, atoi(pr2str))) {
                message_add(plyr_idx, "unable to set assistant");
            } else
                message_add_fmt(plyr_idx, "computer assistant is %d", atoi(pr2str));
            return true;
        }
        else if ( (strcasecmp(parstr, "comp.ai") == 0) || (strcasecmp(parstr, "player.ai") == 0) )
        {
            player = get_player(get_player_number_for_command(pr2str));
            if (player_exists(player))
            {
                struct Computer2* comp = get_computer_player(player->id_number);               
                    if (!computer_player_invalid(comp))
                    {
                        message_add_fmt(plyr_idx, "Player %ld AI type: %ld", player->id_number, comp->model);
                    }
                    return true;
            }
            return false;
        }
        else if ( (strcasecmp(parstr, "give.trap") == 0) || (strcasecmp(parstr, "trap.give") == 0) )
        {
            long id = get_rid(trap_desc, pr2str);
            if (id <= 0)
            {
                if ( (strcasecmp(pr2str, "gas") == 0) || (strcasecmp(pr2str, "poison") == 0) || (strcasecmp(pr2str, "poisongas") == 0) )
                {
                    id = 3;
                }
                else if ( (strcasecmp(pr2str, "word") == 0) || (strcasecmp(pr2str, "wordofpower") == 0) )
                {
                    id = 5;
                }
                else
                {
                    id = atoi(pr2str);
                }
            }
            if (id <= 0 || id > trapdoor_conf.trap_types_count)
                return false;
            unsigned char num = (pr3str != NULL) ? atoi(pr3str) : 1;
            command_add_value(Cmd_TRAP_AVAILABLE, plyr_idx, id, 1, num);
            update_trap_tab_to_config();
            message_add(plyr_idx, "done!");
            return true;
        } else if ( (strcasecmp(parstr, "give.door") == 0) || (strcasecmp(parstr, "door.give") == 0) )
        {
            long id = get_rid(door_desc, pr2str);
            if (id <= 0)
            {
                if (strcasecmp(pr2str, "wooden") == 0)
                {
                    id = 1;
                }
                else if (strcasecmp(pr2str, "iron") == 0)
                {
                    id = 3;
                }
                else if (strcasecmp(pr2str, "magical") == 0)
                {
                    id = 4;
                }
                else
                {
                    id = atoi(pr2str);
                }
            }
            if (id <= 0 || id > trapdoor_conf.door_types_count)
                return false;
            unsigned char num = (pr3str != NULL) ? atoi(pr3str) : 1;
            script_process_value(Cmd_DOOR_AVAILABLE, plyr_idx, id, 1, num);
            update_trap_tab_to_config();
            message_add(plyr_idx, "done!");
            return true;
        }
        else if (strcasecmp(parstr, "room.available") == 0)
        {
            long room = get_rid(room_desc, pr2str);
            if (room <= 0)
            {
                if (strcasecmp(pr2str, "Hatchery" ) == 0)
                {
                    room = RoK_GARDEN;
                }
                else if ( (strcasecmp(pr2str, "Guard" ) == 0) || (strcasecmp(pr2str, "GuardPost" ) == 0) )
                {
                    room = RoK_GUARDPOST;
                }
                else
                {
                    room = atoi(pr2str);
                }
            }
            unsigned char available = (pr3str == NULL) ? 1 : atoi(pr3str);
            PlayerNumber id = get_player_number_for_command(pr4str);
            script_process_value(Cmd_ROOM_AVAILABLE, id, room, (TbBool)available, (TbBool)available);
            update_room_tab_to_config();
            return true;
        }
        else if ( (strcasecmp(parstr, "power.give") == 0) || (strcasecmp(parstr, "spell.give") == 0) )
        {
            long power = get_rid(power_desc, pr2str);
            if (power <= 0)
            {
                if ( (strcasecmp(pr2str, "Imp" ) == 0) || (strcasecmp(pr2str, "CresteImp" ) == 0) )
                {
                    power = PwrK_MKDIGGER;
                }
                else if ( (strcasecmp(pr2str, "Possess" ) == 0) || (strcasecmp(pr2str, "Possession" ) == 0)  || (strcasecmp(pr2str, "PossessCreature" ) == 0))
                {
                    power = PwrK_POSSESS;
                }
                else if ( (strcasecmp(pr2str, "Sight" ) == 0) || (strcasecmp(pr2str, "SightOfEvil" ) == 0) )
                {
                    power = PwrK_SIGHT;
                }
                else if ( (strcasecmp(pr2str, "Speed" ) == 0) || (strcasecmp(pr2str, "SpeedMonster" ) == 0) || (strcasecmp(pr2str, "SpeedCreature" ) == 0) )
                {
                    power = PwrK_SPEEDCRTR;
                }
                else if ( (strcasecmp(pr2str, "Obey" ) == 0) || (strcasecmp(pr2str, "MustObey" ) == 0) )
                {
                    power = PwrK_OBEY;
                }
                else if ( (strcasecmp(pr2str, "CTA" ) == 0) || (strcasecmp(pr2str, "CallToArms" ) == 0) )
                {
                    power = PwrK_CALL2ARMS;
                }
                else if (strcasecmp(pr2str, "CaveIn" ) == 0)
                {
                    power = PwrK_CAVEIN;
                }
                else if (strcasecmp(pr2str, "Heal" ) == 0)
                {
                    power = PwrK_HEALCRTR;
                }
                else if ( (strcasecmp(pr2str, "Audience" ) == 0) || (strcasecmp(pr2str, "HoldAudience" ) == 0) )
                {
                    power = PwrK_HOLDAUDNC;
                }
                else if ( (strcasecmp(pr2str, "Lightning" ) == 0) || (strcasecmp(pr2str, "LightningStrike" ) == 0) )
                {
                    power = PwrK_LIGHTNING;
                }
                else if ( (strcasecmp(pr2str, "Protect" ) == 0) || (strcasecmp(pr2str, "ProtectMonster" ) == 0) || (strcasecmp(pr2str, "ProtectCreature" ) == 0) || (strcasecmp(pr2str, "Armour" ) == 0))
                {
                    power = PwrK_PROTECT;
                }
                else if ( (strcasecmp(pr2str, "Conceal" ) == 0) || (strcasecmp(pr2str, "ConcealMonster" ) == 0) || (strcasecmp(pr2str, "ConcealCreature" ) == 0) || (strcasecmp(pr2str, "Invisibility" ) == 0))
                {
                    power = PwrK_CONCEAL;
                }
                else if (strcasecmp(pr2str, "Disease" ) == 0)
                {
                    power = PwrK_DISEASE;
                }
                else if (strcasecmp(pr2str, "Chicken" ) == 0)
                {
                    power = PwrK_CHICKEN;
                }
                else if ( (strcasecmp(pr2str, "Destroy" ) == 0) || (strcasecmp(pr2str, "DestroyWalls" ) == 0) )
                {
                    power = PwrK_DESTRWALLS;
                }
                else if ( (strcasecmp(pr2str, "Bomb" ) == 0) || (strcasecmp(pr2str, "Time" ) == 0) || (strcasecmp(pr2str, "TimeBomb" ) == 0) )
                {
                    power = PwrK_TIMEBOMB;
                }
                else if (strcasecmp(pr2str, "Armageddon" ) == 0)
                {
                    power = PwrK_ARMAGEDDON;
                }
                else
                {
                    power = atoi(pr2str);
                }                
            }
            script_process_value(Cmd_MAGIC_AVAILABLE, plyr_idx, power, 1, 1);
            update_powers_tab_to_config();
            return true;
        }
        else if (strcasecmp(parstr, "player.heart.health") == 0)
        {
            PlayerNumber id = get_player_number_for_command(pr2str);
            thing = get_player_soul_container(id);
            if (thing_is_dungeon_heart(thing))
            {
                    if (pr3str == NULL)
                    {
                        message_add_fmt(plyr_idx, "Player %d heart health: %ld", id, thing->health);
                        return true;
                    }
                    else
                    {
                        short Health = atoi(pr3str);
                        thing->health = Health;
                        return true;
                    }
            }
            return false;
        }
        else if (strcasecmp(parstr, "player.heart.get") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                PlayerNumber id = get_player_number_for_command(pr2str);
                thing = get_player_soul_container(id);
                if (thing_is_dungeon_heart(thing))
                {
                    message_add_fmt(plyr_idx, "Got thing ID %d", thing->index);
                    player = get_player(plyr_idx);
                    player->influenced_thing_idx = thing->index;
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "creature.available") == 0)
        {
            long crmodel = get_creature_model_for_command(pr2str);
            unsigned char available = (pr3str == NULL) ? 1 : atoi(pr3str);
            PlayerNumber id = get_player_number_for_command(pr4str);
            script_process_value(Cmd_CREATURE_AVAILABLE, id, crmodel, (TbBool)available, (TbBool)available);
            return true;
        }
        else if (strcasecmp(parstr, "creature.show.partytarget") == 0)
        {
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            if (thing_is_creature(thing))
            {
                struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
                message_add_fmt(plyr_idx, "Target player: %d", cctrl->party.target_plyr_idx);
                return true;
            }
            return false;
        }
        else if ( (strcasecmp(parstr, "creature.addhealth") == 0) || (strcasecmp(parstr, "creature.health.add") == 0) )
        {
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            if (thing_is_creature(thing))
            {
                thing->health += atoi(pr2str);
                return true;
            }
            return false;
        }
        else if ( (strcasecmp(parstr, "creature.subhealth") == 0) || (strcasecmp(parstr, "creature.health.sub") == 0) )
        {
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            if (thing_is_creature(thing))
            {
                thing->health -= atoi(pr2str);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "digger.sendto") == 0)
        {
            PlayerNumber id = get_player_number_for_command(pr2str);
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            if (thing_is_creature(thing))
            {
                if (thing->model == get_players_special_digger_model(thing->owner))
                {
                    player = get_player(id);
                    if (player_exists(player))
                    {
                        get_random_position_in_dungeon_for_creature(id, CrWaS_WithinDungeon, thing, &pos);
                        return send_tunneller_to_point_in_dungeon(thing, id, &pos);
                    }
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "creature.instance.set") == 0)
        {
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            unsigned char inst = atoi(pr2str);
            if (thing_is_creature(thing))
            {
                if (inst >= 0)
                {
                    set_creature_instance(thing, inst, 0, 0, 0);
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "creature.state.set") == 0)
        {
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            unsigned char state = atoi(pr2str);
            if (thing_is_creature(thing))
            {
                if (state >= 0)
                {
                    if (can_change_from_state_to(thing, thing->active_state, state))
                    {
                        return internal_set_thing_state(thing, state);
                    }
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "creature.job.set") == 0)
        {
            player = get_my_player();
            thing = thing_get(player->influenced_thing_idx);
            unsigned char new_job = atoi(pr2str);
            if (thing_is_creature(thing))
            {
                if (new_job >= 0)
                {
                    if (creature_can_do_job_for_player(thing, thing->owner, 1LL<<new_job, JobChk_None))
                    {
                        return send_creature_to_job_for_player(thing, thing->owner, 1LL<<new_job);
                    }
                    else
                    {
                        message_add_fmt(plyr_idx, "Cannot do job %d.", new_job);
                        return true;
                    }
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "creature.attackheart") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                player = get_player(plyr_idx);
                thing = thing_get(player->influenced_thing_idx);
                if (thing_is_creature(thing))
                {
                    PlayerNumber id = get_player_number_for_command(pr2str);
                    struct Thing* heartng = get_player_soul_container(id);
                    if (thing_is_dungeon_heart(heartng))
                    {
                        TRACE_THING(heartng);
                        set_creature_object_combat(thing, heartng);
                        return true;
                    }
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "player.gold") == 0)
        {
            PlayerNumber id = get_player_number_for_command(pr2str);
            player = get_player(id);
            if (player_exists(player))
            {
                dungeon = get_dungeon(id);
                message_add_fmt(plyr_idx, "Player %d gold: %ld", id, dungeon->total_money_owned);
                return true;
            }
            return false;
        }
        else if ( (strcasecmp(parstr, "player.addgold") == 0) || (strcasecmp(parstr, "player.gold.add") == 0) )
        {
            PlayerNumber id = get_player_number_for_command(pr2str);
            player = get_player(id);
            if (player_exists(player))
            {
                // dungeon = get_dungeon(id);
                if (pr3str == NULL)
                {
                    return false;
                }
                else
                {
                    // dungeon->total_money_owned += atoi(pr3str);
                    script_process_value(Cmd_ADD_GOLD_TO_PLAYER, id, atoi(pr3str), 0, 0);
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.get") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            thing = (pr2str != NULL) ? thing_get(atoi(pr2str)) : get_nearest_object_at_position(stl_x, stl_y);
            if (!thing_is_invalid(thing))
            {
                message_add_fmt(plyr_idx, "Got thing ID %d", thing->index);
                player->influenced_thing_idx = thing->index;
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.model") == 0)
        {
            player = get_player(plyr_idx);
            thing = thing_get(player->influenced_thing_idx);
            if (!thing_is_invalid(thing))
            {
                message_add_fmt(plyr_idx, "Thing model: %d", thing->model);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.health") == 0)
        {
            player = get_player(plyr_idx);
            thing = thing_get(player->influenced_thing_idx);
            if (!thing_is_invalid(thing))
            {
                if (pr2str != NULL)
                {
                    thing->health = atoi(pr2str);
                }
                else
                {
                    message_add_fmt(plyr_idx, "Thing ID: %d health: %d", thing->index, thing->health);
                }
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.pos") == 0)
        {
            player = get_player(plyr_idx);
            thing = thing_get(player->influenced_thing_idx);
            if (!thing_is_invalid(thing))
            {
                message_add_fmt(plyr_idx, "Thing ID %d X: %d Y: %d", thing->index, thing->mappos.x.stl.num, thing->mappos.y.stl.num);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.class") == 0 )
        {
            player = get_player(plyr_idx);
            thing = thing_get(player->influenced_thing_idx);
            if (!thing_is_invalid(thing))
            {
                message_add_fmt(plyr_idx, "Thing ID %d class: %d", thing->index, thing->class_id);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.owner") == 0 )
        {
            player = get_player(plyr_idx);
            thing = thing_get(player->influenced_thing_idx);
            if (!thing_is_invalid(thing))
            {
                message_add_fmt(plyr_idx, "Thing ID %d owner: %d", thing->index, thing->owner);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.count") == 0 )
        {
            message_add_fmt(plyr_idx, "Things count: %d", game.free_things_start_index);
            return true;
        }
        else if (strcasecmp(parstr, "thing.destroy") == 0)
        {
            player = get_player(plyr_idx);
            thing = thing_get(player->influenced_thing_idx);
            if (!thing_is_invalid(thing))
            {
                destroy_object(thing);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "thing.create") == 0)
        {
            if ( (pr2str == NULL) || (pr3str == NULL) )
            {
                return false;
            }
            else
            {
                player = get_player(plyr_idx);
                pckt = get_packet_direct(player->packet_num);
                pos.x.stl.num = coord_subtile(((unsigned short)pckt->pos_x));
                pos.y.stl.num = coord_subtile(((unsigned short)pckt->pos_y));
                unsigned short tngclass = atoi(pr2str);
                unsigned short tngmodel = atoi(pr3str);
                PlayerNumber id = get_player_number_for_command(pr4str);
                thing = create_thing(&pos, tngclass, tngmodel, id, -1);
                if (!thing_is_invalid(thing))
                {
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "room.get") == 0 )
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            room = (pr2str != NULL) ? room_get(atoi(pr2str)) : subtile_room_get(stl_x, stl_y);
            if (room_exists(room))
            {
                message_add_fmt(plyr_idx, "Got room ID %d", room->index);
                player->influenced_thing_idx = room->index;
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "room.efficiency") == 0)
        {
            player = get_player(plyr_idx);
            room = room_get(player->influenced_thing_idx);
            if (!room_is_invalid(room))
            {
                float percent = ((float)room->efficiency / (float)ROOM_EFFICIENCY_MAX) * 100;
                message_add_fmt(plyr_idx, "Room ID %d efficiency: %d (%d per cent)", room->index, room->efficiency, (unsigned int)round(percent));
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "room.health") == 0)
        {
            player = get_player(plyr_idx);
            room = room_get(player->influenced_thing_idx);
            if (!room_is_invalid(room))
            {
                if (pr2str == NULL)
                {
                    message_add_fmt(plyr_idx, "Room ID %d health: %d", room->index, room->health);
                    return true;
                }
                else
                {
                    room-> health = atoi(pr2str);
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "room.pos") == 0 )
        {
            player = get_player(plyr_idx);
            room = room_get(player->influenced_thing_idx);
            if (!room_is_invalid(room))
            {
                message_add_fmt(plyr_idx, "Room ID %d X: %d Y: %d", room->index, room->central_stl_x, room->central_stl_y);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "slab.kind") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            slb = get_slabmap_for_subtile(stl_x, stl_y);
            if (!slabmap_block_invalid(slb))
            {
                message_add_fmt(plyr_idx, "Slab kind: %d", slb->kind);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "slab.health") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            slb = get_slabmap_for_subtile(stl_x, stl_y);
            if (!slabmap_block_invalid(slb))
            {
                if (pr2str == NULL)
                {                    
                    message_add_fmt(plyr_idx, "Slab health: %d", slb->health);
                    return true;
                }
                else
                {
                    slb->health = atoi(pr2str);
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "slab.place") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            MapSlabCoord slb_x = subtile_slab(stl_x);
            MapSlabCoord slb_y = subtile_slab(stl_y);
            slb = get_slabmap_block(slb_x, slb_y);
            if (!slabmap_block_invalid(slb))
            {
                PlayerNumber id = (pr3str == NULL) ? slabmap_owner(slb) : get_player_number_for_command(pr3str);
                short slbkind = get_rid(slab_desc, pr2str);
                if (slbkind <= 0)
                {
                    long rid = get_rid(room_desc, pr2str);
                    if (rid > 0)
                    {
                        struct RoomConfigStats *roomst = get_room_kind_stats(rid);
                        slbkind = roomst->assigned_slab;
                    }
                    else
                    {
                        if (strcasecmp(pr2str, "Earth") == 0)
                        {
                            slbkind = rand() % (4 - 2) + 2;
                        }
                        else if ( (strcasecmp(pr2str, "Reinforced") == 0) || (strcasecmp(pr2str, "Fortified") == 0) )
                        {
                            slbkind = rand() % (9 - 4) + 4;
                        }
                        else if (strcasecmp(pr2str, "Claimed") == 0)
                        {
                            slbkind = SlbT_CLAIMED;
                        }
                        else if ( (strcasecmp(pr2str, "Rock") == 0) || (strcasecmp(pr2str, "Impenetrable") == 0) )
                        {
                            slbkind = SlbT_ROCK;
                        }
                        else
                        {
                            slbkind = atoi(pr2str);
                        }
                    }
                }
                if (subtile_is_room(stl_x, stl_y)) 
                {
                    room = subtile_room_get(stl_x, stl_y);
                    delete_room_slab(slb_x, slb_y, true);
                }
                if (slab_kind_is_animated(slbkind))
                {
                    place_animating_slab_type_on_map(slbkind, 0, stl_x, stl_y, id);  
                }
                else
                {
                    place_slab_type_on_map(slbkind, stl_x, stl_y, id, 0);
                }
                do_slab_efficiency_alteration(slb_x, slb_y);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "slab.isblocking") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            if (subtile_coords_invalid(stl_x, stl_y))
            {
                return false;
            }
            else
            {
                if (subtile_is_blocking_wall_or_lava(stl_x, stl_y, plyr_idx))
                {
                    message_add_fmt(plyr_idx, "Slab is blocking");   
                }
                else
                {
                    message_add_fmt(plyr_idx, "Slab is not blocking");   
                }
                return true;
            }                
        }
        else if (strcasecmp(parstr, "column.get") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            struct Map *mapblk = get_map_block_at(stl_x, stl_y);
            if (!map_block_invalid(mapblk))
            {
                message_add_fmt(plyr_idx, "Column index: %d", get_mapblk_column_index(mapblk));
                return true;
            }
            return false;            
        }
        else if (strcasecmp(parstr, "cube.get") == 0)
        {
            player = get_player(plyr_idx);
            pckt = get_packet_direct(player->packet_num);
            MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
            MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
            if (!subtile_coords_invalid(stl_x, stl_y))
            {
                long cube_id = get_top_cube_at(stl_x, stl_y, NULL);
                message_add_fmt(plyr_idx, "Cube ID: %d", cube_id);
                return true;
            }
            return false;
        }
        else if ( (strcasecmp(parstr, "creature.pool") == 0) || (strcasecmp(parstr, "creature.inby") == 0) )
        {
            long crmodel = get_creature_model_for_command(pr2str);
            if (crmodel == -1)
            {
                crmodel = atoi(pr2str);
            }
            if (pr3str != NULL)
            {
                // game.pool.crtr_kind[crmodel] = atoi(pr3str);
                set_creature_pool(crmodel, atoi(pr3str));
            }
            else
            {
                message_add_fmt(plyr_idx, "%d in pool", game.pool.crtr_kind[crmodel]);    
            }
            return true;
        }
        else if (strcasecmp(parstr, "creature.pool.add") == 0)
        {
            if (pr3str == NULL)
            {
                return false;
            }
            else
            {
                long crmodel = get_creature_model_for_command(pr2str);
                if (crmodel == -1)
                {
                    crmodel = atoi(pr2str);
                }
                game.pool.crtr_kind[crmodel] += atoi(pr3str);
                return true;
            }
        }
        else if ( (strcasecmp(parstr, "creature.pool.sub") == 0) || (strcasecmp(parstr, "creature.pool.remove") == 0) )
        {
            if (pr3str == NULL)
            {
                return false;
            }
            else
            {
                long crmodel = get_creature_model_for_command(pr2str);
                if (crmodel == -1)
                {
                    crmodel = atoi(pr2str);
                }
                game.pool.crtr_kind[crmodel] -= atoi(pr3str);
                return true;
            }
        }
        else if (strcasecmp(parstr, "creature.create") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                long crmodel = get_creature_model_for_command(pr2str);
                if (crmodel == -1)
                {
                    crmodel = atoi(pr2str);
                }
                if ( (crmodel > 0) && (crmodel <= 31) )
                {
                    player = get_player(plyr_idx);
                    pckt = get_packet_direct(player->packet_num);
                    MapSubtlCoord stl_x = coord_subtile(((unsigned short)pckt->pos_x));
                    MapSubtlCoord stl_y = coord_subtile(((unsigned short)pckt->pos_y));
                    MapSlabCoord slb_x = subtile_slab(stl_x);
                    MapSlabCoord slb_y = subtile_slab(stl_y);
                    PlayerNumber id = get_player_number_for_command(pr5str);
                    if ( (slab_is_wall(slb_x, slb_y)) || (slab_coords_invalid(slb_x, slb_y)) )
                    {
                        thing = get_player_soul_container(plyr_idx);
                        if (thing_is_dungeon_heart(thing))
                        {
                            pos = thing->mappos;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!subtile_coords_invalid(stl_x, stl_y))
                        {
                            pos.x.stl.num = stl_x;
                            pos.y.stl.num = stl_y;
                        }
                        else
                        {
                            return false;
                        }
                    }                        
                    unsigned int count = (pr4str != NULL) ? atoi(pr4str) : 1;
                    unsigned int i;
                    for (i = 0; i < count; i++)
                    {                            
                        struct Thing* creatng = create_creature(&pos, crmodel, id);
                        if (thing_is_creature(creatng))
                        {
                            set_creature_level(creatng, (atoi(pr3str) - (unsigned char)(pr3str != NULL)));
                        }
                    }
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "creatures.max") == 0)
        {
            PlayerNumber id = get_player_number_for_command(pr2str);
            dungeon = get_dungeon(id);
            if (!dungeon_invalid(dungeon))
            {
                message_add_fmt(plyr_idx, "Max creatures: %d", dungeon->max_creatures_attracted);
                return true;    
            }
            return false;
        }
        else if (strcasecmp(parstr, "floating.spirit") == 0)
        {
            level_lost_go_first_person(plyr_idx);
            return true;
        }
        else if (strcasecmp(parstr, "music") == 0)
        {
            message_add_fmt(plyr_idx, "Current music track: %d", game.audiotrack);
            return true;
        }
        else if (strcasecmp(parstr, "music.set") == 0)
        {
            int track = atoi(pr2str);
            if (track >= FIRST_TRACK && track <= max_track)
            {
                StopMusicPlayer();
                game.audiotrack = track;
                PlayMusicPlayer(track);
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "zoomto") == 0)
        {
            if ( (pr2str != NULL) && (pr3str != NULL) )
            {
                MapSubtlCoord stl_x = atoi(pr2str);
                MapSubtlCoord stl_y = atoi(pr3str);
                if (!subtile_coords_invalid(stl_x, stl_y))
                {
                    player = get_player(plyr_idx);
                    player->zoom_to_pos_x = subtile_coord_center(stl_x);
                    player->zoom_to_pos_y = subtile_coord_center(stl_y);
                    set_player_instance(player, PI_ZoomToPos, 0);
                    return true;
                }
                else
                {
                    message_add_fmt(plyr_idx, "Co-ordinates specified are invalid");
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "bug.toggle") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                unsigned char bug = atoi(pr2str);
                unsigned short flg = (bug > 2) ? (1 << (bug - 1)) : bug; 
                toggle_flag_word(&gameadd.classic_bugs_flags, flg);
                message_add_fmt(plyr_idx, "%s %s", get_conf_parameter_text(rules_game_classicbugs_commands, bug), ((gameadd.classic_bugs_flags & flg) != 0) ? "enabled" : "disabled");
                return true;
            }
            return false;
        }
        else if (strcasecmp(parstr, "actionpoint.pos") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                unsigned char ap = atoi(pr2str);
                if (action_point_exists_idx(ap))
                {
                    struct ActionPoint* actionpt = action_point_get(ap);
                    message_add_fmt(plyr_idx, "Action Point %d X: %d Y: %d", ap, actionpt->mappos.x.stl.num, actionpt->mappos.y.stl.num);
                    return true;
                }
            }
            return false;            
        }
        else if (strcasecmp(parstr, "actionpoint.reset") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                unsigned char ap = atoi(pr2str);
                if (action_point_exists_idx(ap))
                {
                    return action_point_reset_idx(ap);
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "actionpoint.count") == 0)
        {
            unsigned char count = 0;
            unsigned long i;
            for (i = 0; i <= ACTN_POINTS_COUNT; i++)
            {
                if (action_point_exists_idx(i))
                {
                    count++;
                }
            }
            message_add_fmt(plyr_idx, "Action Point count: %d", count);
            return true;
        }
        else if (strcasecmp(parstr, "actionpoint.activated") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                unsigned char ap = atoi(pr2str);
                if (action_point_exists_idx(ap))
                {
                    struct ActionPoint* actionpt = action_point_get(ap);
                    message_add_fmt(plyr_idx, "%d", actionpt->activated);
                    return true;
                }
            }
            return false;            
        }
        else if (strcasecmp(parstr, "herogate.pos") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                unsigned char hg = atoi(pr2str);
                thing = find_hero_gate_of_number(hg);
                if (object_is_hero_gate(thing))
                {
                    message_add_fmt(plyr_idx, "Hero Gate %d X: %d Y: %d", hg, thing->mappos.x.stl.num, thing->mappos.y.stl.num);
                    return true;
                }
            }
            return false;
        }
        else if (strcasecmp(parstr, "sound.test") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                play_non_3d_sample(atoi(pr2str));
                return true;
            }
        }
        else if (strcasecmp(parstr, "speech.test") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                play_speech_sample(atoi(pr2str));
                return true;
            }
        }
        else if (strcasecmp(parstr, "campaign.name") == 0)
        {
            message_add_fmt(plyr_idx, "%s", campaign.name);
            return true;
        }
        else if (strcasecmp(parstr, "campaign.level.num") == 0)
        {
            message_add_fmt(plyr_idx, "%d", campaign.lvinfos->lvnum);
            return true;
        }
        else if (strcasecmp(parstr, "game.level.num") == 0)
        {
            message_add_fmt(plyr_idx, "%d", game.loaded_level_number);
            return true;    
        }
        else if (strcasecmp(parstr, "level.restart") == 0)
        {
            restart_current_level();
            return true;
        }
        else if (strcasecmp(parstr, "object.create") == 0)
        {
            if (pr2str == NULL)
            {
                return false;
            }
            else
            {
                player = get_player(plyr_idx);
                pckt = get_packet_direct(player->packet_num);
                pos.x.stl.num = coord_subtile(((unsigned short)pckt->pos_x));
                pos.y.stl.num = coord_subtile(((unsigned short)pckt->pos_y));
                PlayerNumber id = get_player_number_for_command(pr3str);
                thing = create_object(&pos, atoi(pr2str), id, -1);
                if (thing_is_object(thing))
                {
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

long get_creature_model_for_command(char *msg)
{
    long rid = get_rid(creature_desc, msg);
    if (rid >= 1)
    {
        return rid;
    }
    else
    {
        if (strcasecmp(msg, "beetle") == 0)
        {
            return 24;
        }
        else if (strcasecmp(msg, "mistress") == 0)
        {
            return 20;
        }
        else if (strcasecmp(msg, "biledemon") == 0)
        {
            return 22;
        }
        else if (strcasecmp(msg, "hound") == 0)
        {
            return 27;
        }
        else if (strcasecmp(msg, "priestess") == 0)
        {
            return 9;
        }
        else if ( (strcasecmp(msg, "warlock") == 0) || (strcasecmp(msg, "sorcerer") == 0) )
        {
            return 21;
        }
        else if ( (strcasecmp(msg, "reaper") == 0) || (strcasecmp(msg, "hornedreaper") == 0) )
        {
            return 14;
        }
        else if ( (strcasecmp(msg, "dwarf") == 0) || (strcasecmp(msg, "mountaindwarf") == 0) )
        {
            return 5;
        }
        else if ( (strcasecmp(msg, "spirit") == 0) || (strcasecmp(msg, "floatingspirit") == 0) )
        {
            return 31;
        }
        else
        {
            return -1;
        }    
    }
}

PlayerNumber get_player_number_for_command(char *msg)
{
    PlayerNumber id = (msg == NULL) ? my_player_number : get_rid(cmpgn_human_player_options, msg);
    if (id == -1)
    {
        if (strcasecmp(msg, "neutral") == 0)
        {
            id = game.neutral_player_num;
        }
        else
        {
            id = atoi(msg);
        }                            
    }
    return id;
}


#ifdef __cplusplus
}
#endif
