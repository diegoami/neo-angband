/* Diagnostic native C equipment/monster scenario, not proposal source. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "effects.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-make.h"
#include "mon-spell.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "borg/borg-trait.h"
#include "borg/borg-cave.h"
#include "obj-util.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "project.h"
#include "ui-term.h"
#include "ui-menu.h"
#include "ui-map.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-init.h"
#include "borg/borg-item.h"
#include "borg/borg-item-wear.h"
#include "borg/borg-trait-swap.h"
#include "borg/borg-inventory.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-io.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct object *cold_shield, *fire_weapon, *primary_shield;
static struct monster *dragon, *second_dragon;
static int breath_spell=RSF_BR_COLD;
static int danger_now=-1, danger_armour=-1, danger_weapon=-1;
static int selected_index=-1;
static const char *last_message="";
static char message_text[1024];
static const char *mode;

static void json_string(const char *text)
{
    putchar('"');
    for (const unsigned char *p=(const unsigned char *)text;*p;++p) {
        if(*p=='"' || *p=='\\') {putchar('\\');putchar(*p);}
        else if(*p<32) printf("\\u%04x",*p);
        else putchar(*p);
    }
    putchar('"');
}
static void frame(const char *stage)
{
    printf("FRAME {\"stage\":");json_string(stage);
    printf(",\"hp\":%d,\"max_hp\":%d,\"cold\":%d,\"fire\":%d,\"elec\":%d,\"danger\":%d,\"armour_danger\":%d,\"weapon_danger\":%d,\"selected_index\":%d,\"message\":",player->chp,player->mhp,player->state.el_info[ELEM_COLD].res_level,player->state.el_info[ELEM_FIRE].res_level,player->state.el_info[ELEM_ELEC].res_level,danger_now,danger_armour,danger_weapon,selected_index);
    json_string(last_message);
    char desc[256];
    printf(",\"equipped_shield\":");
    struct object *shield=player->body.slots[wield_slot(primary_shield)].obj;
    if(shield) object_desc(desc,sizeof(desc),shield,ODESC_FULL,player); else my_strcpy(desc,"none",sizeof(desc));
    json_string(desc);
    printf(",\"equipped_weapon\":");
    struct object *weapon=equipped_item_by_slot_name(player,"weapon");
    if(weapon) object_desc(desc,sizeof(desc),weapon,ODESC_FULL,player); else my_strcpy(desc,"none",sizeof(desc));json_string(desc);
    printf(",\"candidate_armour\":");
    if(cold_shield) object_desc(desc,sizeof(desc),cold_shield,ODESC_FULL,player);else my_strcpy(desc,"none",sizeof(desc));json_string(desc);
    printf(",\"candidate_weapon\":");
    if(fire_weapon) object_desc(desc,sizeof(desc),fire_weapon,ODESC_FULL,player);else my_strcpy(desc,"none",sizeof(desc));json_string(desc);
    printf(",\"map\":[");
    for(int y=0;y<24;++y) {
        char row[25];
        for(int x=0;x<24;++x) {
            struct grid_data data;int a,ta;wchar_t c,tc;
            map_info(loc(x,y),&data);grid_data_as_text(&data,&a,&c,&ta,&tc);
            row[x]=(c>0 && c<128) ? (char)c : '?';
        }
        row[24]=0;if(y)putchar(',');json_string(row);
    }
    printf("]}\n");
}
static void message(game_event_type type, game_event_data *data, void *user)
{
    if (data->message.msg) {
        my_strcpy(message_text,data->message.msg,sizeof(message_text));last_message=message_text;
        printf("ENGINE: %s\n",data->message.msg);
    }
}
static void refresh_items(void)
{
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA;
    update_stuff(player);
    borg_equipment();
    borg_inventory();
    memcpy(safe_items, borg_items, QUIVER_END * sizeof(borg_item));
    borg_notice(true);
    borg.c = player->grid;
    borg.avoidance = player->chp;
}
static struct object *make_item(int tval, const char *kind_name, const char *ego_name)
{
    struct object *obj = object_new();
    int sval = lookup_sval(tval, kind_name);
    object_prep(obj, lookup_kind(tval, sval), 50, AVERAGE);
    if (ego_name) {
        obj->ego = lookup_ego_item(ego_name, tval, sval);
        if (!obj->ego) quit("unknown ego");
        ego_apply_magic(obj, 50);
    }
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}
int setup_tests(void **state)
{
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER=string_make("/tmp/native-c-66-expanded/profile");
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE=string_make("/tmp/native-c-66-expanded/profile/save");
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC=string_make("/tmp/native-c-66-expanded/profile/panic");
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES=string_make("/tmp/native-c-66-expanded/profile/scores");
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE=string_make("/tmp/native-c-66-expanded/profile/archive");
    if (!init_angband()) {puts("setup: init failed");return 1;}
    create_needed_dirs();
    Rand_quick=false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "CBUG0066")) {puts("setup: birth failed");return 1;}
    term_init(&screen, 160, 60, 256); Term_activate(&screen); angband_term[0]=&screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave,player,loc(12,12));
    player->depth=player->max_depth=cave->depth=player->cave->depth=50;
    character_dungeon=true;
    for (int y=1;y<23;++y) for (int x=1;x<23;++x)
        sqinfo_on(square(cave,loc(x,y))->info,SQUARE_GLOW);
    mode=getenv("CBUG0066_MODE"); if(!mode) mode="armour-better";
    player_exp_gain(player,streq(mode,"both-helpful") ? 2000000 : 1000000);
    player->upkeep->update |= PU_BONUS|PU_HP|PU_TORCH; update_stuff(player);
    player->chp=player->mhp;
    player_learn_all_runes(player);
    primary_shield = make_item(TV_SHIELD,"Small Metal Shield","of Resist Acid");
    inven_wield(primary_shield,wield_slot(primary_shield));
    struct object *boots=make_item(TV_BOOTS,"Pair of Leather Boots","of Speed");
    inven_wield(boots,wield_slot(boots));
    if (!streq(mode,"weapon-only"))
        cold_shield=make_item(TV_SHIELD,"Small Metal Shield",
            streq(mode,"both-helpful") ? "of Resistance" :
            streq(mode,"fire-armour") ? "of Resist Fire" :
            (streq(mode,"weapon-better") || streq(mode,"no-benefit")) ? "of Resist Fire" : "of Resist Cold");
    if (!streq(mode,"armour-only"))
        fire_weapon=make_item(TV_SWORD,"Dagger",
            streq(mode,"electric-weapon") ? "of Lightning" :
            (streq(mode,"weapon-better") || streq(mode,"weapon-only") || streq(mode,"tie") || streq(mode,"fire-armour") || streq(mode,"both-helpful")) ? "of Frost" : "of Flame");
    borg_rand_local=1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player,cheat_live)) {printf("setup: borg failure=%d cheat=%d live=%d\n",borg_init_failure,borg_cfg[BORG_CHEAT_DEATH],OPT(player,cheat_live));return 1;}
    refresh_items();
    for (int y=0;y<24;++y) for(int x=0;x<24;++x)
        borg_grids[y][x].feat = square(cave,loc(x,y))->feat;
    update_view(cave,player);
    const char *monster_name=streq(mode,"fire-armour") ? "Young red dragon" : streq(mode,"electric-weapon") ? "Young blue dragon" : "Young white dragon";
    breath_spell=streq(mode,"fire-armour") ? RSF_BR_FIRE : streq(mode,"electric-weapon") ? RSF_BR_ELEC : RSF_BR_COLD;
    dragon=t_add_monster(cave,loc(12,9),monster_name);
    update_mon(dragon,cave,true);
    if (!mflag_has(dragon->mflag,MFLAG_VISIBLE)) {puts("setup: dragon not visible");return 1;}
    if (!observe_kill_diff(dragon->grid.y,dragon->grid.x,0,0)) return 1;
    int kill=borg_grids[dragon->grid.y][dragon->grid.x].kill;
    if (kill<=0) {printf("setup: kill lookup=%d\n",kill);return 1;}
    event_add_handler(EVENT_MESSAGE,message,NULL);
    if(streq(mode,"both-helpful")) {
        second_dragon=t_add_monster(cave,loc(15,12),"Young red dragon");
        update_mon(second_dragon,cave,true);
        if(!mflag_has(second_dragon->mflag,MFLAG_VISIBLE)) return 1;
        if(!observe_kill_diff(second_dragon->grid.y,second_dragon->grid.x,0,0)) return 1;
    }
    frame("Initial setup");
    if(!streq(mode,"both-helpful")) {
        do_mon_spell(breath_spell,dragon,true);frame("First preparation breath");
        do_mon_spell(breath_spell,dragon,true);frame("Second preparation breath");
    }
    refresh_items();
    printf("SETUP seed=1000 level=%d max_depth=%d hp=%d monster_hp=%d kill=%d\n",player->lev,player->max_depth,player->chp,dragon->hp,kill);
    printf("SPEED engine=%d borg=%d dragon=%d weight=%d carry=%d\n",player->state.speed,borg.trait[BI_SPEED],dragon->mspeed,borg.trait[BI_WEIGHT],borg.trait[BI_CARRY]);
    printf("CANDIDATES armour=%d weapon=%d cold=%d fire=%d\n",armour_swap,weapon_swap,borg.trait[BI_RCOLD],borg.trait[BI_RFIRE]);
    for(int i=0;i<z_info->pack_size;++i) if (borg_items[i].iqty)
        printf("PACK %d: %s\n",i,borg_items[i].desc);
    return 0;
}
int teardown_tests(void *state)
{
    textui_prefs_free();cleanup_angband();term_nuke(&screen);angband_term[0]=NULL;return 0;
}
static int trial_danger(int index)
{
    int slot=borg_wield_slot(&borg_items[index]);
    memcpy(&borg_items[slot],&safe_items[index],sizeof(borg_item));
    borg_items[slot].iqty=1;borg_items[index].iqty--;
    borg_notice(false);
    int value=borg_danger(borg.c.y,borg.c.x,1,true,false);
    memcpy(borg_items,safe_items,QUIVER_END*sizeof(borg_item));
    borg_notice(true);
    return value;
}
static int scenario(void *state)
{
    printf("MODE %s\n",mode);
    int p=borg_danger(borg.c.y,borg.c.x,1,true,false);
    int a=armour_swap,w=weapon_swap;
    printf("DANGER initial=%d avoidance=%d\n",p,borg.avoidance);
    require(a>0 || w>0);
    danger_now=p;danger_armour=a ? trial_danger(a-1) : -1;danger_weapon=w ? trial_danger(w-1) : -1;
    printf("DANGER armour=%d weapon=%d\n",danger_armour,danger_weapon);
    frame("Borg compares backup items");
    require(p>borg.avoidance/3);
    bool swapped=borg_backup_swap(p);
    int command=borg_inkey(true),letter=borg_inkey(true),index=-1;
    if(command=='w') for(int i=0;i<z_info->pack_size;++i)
        if(all_letters_nohjkl[i]==letter) index=i;
    selected_index=index;
    printf("DECISION swapped=%d command=%d letter=%d pack_index=%d\n",swapped,command,letter,index);
    if(swapped) {
        require(index>=0);
        struct object *selected=player->upkeep->inven[index];
        printf("SELECTED cold_shield=%d fire_weapon=%d\n",selected==cold_shield,selected==fire_weapon);
        inven_wield(selected,wield_slot(selected));
        refresh_items();
    }
    printf("PROTECTION cold=%d fire=%d elec=%d\n",player->state.el_info[ELEM_COLD].res_level,player->state.el_info[ELEM_FIRE].res_level,player->state.el_info[ELEM_ELEC].res_level);
    frame("Borg equipment decision applied");
    int hp=player->chp;
    require(rsf_has(dragon->race->spell_flags,breath_spell));
    do_mon_spell(breath_spell,dragon,true);frame("Test breath hits");
    if(second_dragon) {do_mon_spell(RSF_BR_FIRE,second_dragon,true);frame("Second threat breath hits");}
    printf("OUTCOME hp_before=%d hp_after=%d hp_lost=%d\n",hp,player->chp,hp-player->chp);
    if(streq(mode,"no-benefit")) {
        eq(swapped,false); eq(player->state.el_info[ELEM_COLD].res_level,0);
    } else {
        require(swapped);
        int element=streq(mode,"fire-armour") ? ELEM_FIRE : streq(mode,"electric-weapon") ? ELEM_ELEC : ELEM_COLD;
        eq(player->state.el_info[element].res_level,1);
        if(streq(mode,"both-helpful")) eq(player->state.el_info[ELEM_FIRE].res_level,1);
    }
    ok;
}
const char *suite_name="native-c/CBUG-0066";
struct test tests[]={{"cold_dragon_emergency_swap",scenario},{NULL,NULL}};
