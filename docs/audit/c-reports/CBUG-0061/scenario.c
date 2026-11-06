/* Synthetic native C ring-attack diagnostic, excluded from the source proposal. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "target.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item-val.h"
#include "borg/borg-projection.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>


static term screen;
static const char *mode;
static struct object *potions;
static int initial_seed;
static int potion_count(void) {
    int n=0;
    for (struct object *obj=player->gear;obj;obj=obj->next)
        if (obj->tval==TV_POTION && obj->sval==sv_potion_cure_critical) n+=obj->number;
    return n;
}
static void message(game_event_type type, game_event_data *data, void *user) {
    if(data->message.msg) printf("ENGINE %s\n",data->message.msg);
}
static void frame(const char *stage) {
    printf("FRAME {\"stage\":\"%s\",\"hp\":%d,\"max_hp\":%d,\"potions\":%d,\"confused\":%d}\n",stage,player->chp,player->mhp,potion_count(),player->timed[TMD_CONFUSED]);
}
static bool execute(bool bypass) {
    bool decision=borg_quaff_crit(bypass);
    keycode_t keys[16],key;int len=0;
    while((key=borg_inkey(true))!=0 && len<16)keys[len++]=key;
    printf("DECISION use=%d bypass=%d keys=%d sequence=",decision,bypass,len);
    for(int i=0;i<len;i++) {printf("%c",(char)keys[i]);} putchar('\n');
    if(decision) {
        if(len!=2 || keys[0]!='q')quit("Unexpected potion command");
        struct object *obj=NULL;
        for(int i=0;i<z_info->pack_size;i++)
            if((keycode_t)all_letters_nohjkl[i]==keys[1])obj=player->upkeep->inven[i];
        if(!obj || obj->tval!=TV_POTION || obj->sval!=sv_potion_cure_critical)quit("Wrong potion selected");
        struct command cmd={.code=CMD_QUAFF,.context=CTX_GAME};
        cmd_set_arg_item(&cmd,"item",obj);do_cmd_quaff_potion(&cmd);cmd_release(&cmd);
        calc_inventory(player);borg_inventory();borg_notice(true);
    } else if(len)quit("Keys queued on rejection");
    return decision;
}
int setup_tests(void **state) {
    const char *profile=getenv("CBUG0061_PROFILE");if(!profile)return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER);ANGBAND_DIR_USER=string_make(profile);
    string_free(ANGBAND_DIR_SAVE);ANGBAND_DIR_SAVE=string_make(format("%s/save",profile));
    string_free(ANGBAND_DIR_PANIC);ANGBAND_DIR_PANIC=string_make(format("%s/panic",profile));
    string_free(ANGBAND_DIR_SCORES);ANGBAND_DIR_SCORES=string_make(format("%s/scores",profile));
    string_free(ANGBAND_DIR_ARCHIVE);ANGBAND_DIR_ARCHIVE=string_make(format("%s/archive",profile));
    if(!init_angband()) {return 1;} create_needed_dirs();
    initial_seed=getenv("CBUG0061_SEED")?atoi(getenv("CBUG0061_SEED")):1000;
    Rand_quick=false;Rand_state_init(initial_seed);
    if(!player_make_simple("Human","Warrior","NativePotionTiming"))return 1;
    term_init(&screen,160,60,256);Term_activate(&screen);angband_term[0]=&screen;
    cave=t_build_arena(40,40);player->cave=cave_new(40,40);
    player_place(cave,player,loc(20,20));character_dungeon=true;
    player->depth=player->max_depth=cave->depth=player->cave->depth=30;
    for(int y=1;y<39;y++)for(int x=1;x<39;x++)sqinfo_on(square(cave,loc(x,y))->info,SQUARE_GLOW);
    player_exp_gain(player,player_exp[38]*player->expfact/100);
    potions=object_new();object_prep(potions,lookup_kind(TV_POTION,lookup_sval(TV_POTION,"Cure Critical Wounds")),50,MAXIMISE);
    potions->number=8;potions->known=object_new();object_set_base_known(player,potions);
    object_flavor_aware(player,potions);inven_carry(player,potions,false,false);object_touch(player,potions);player_know_object(player,potions);
    player_learn_all_runes(player);calc_inventory(player);
    player->upkeep->update|=PU_BONUS|PU_HP|PU_MANA;update_stuff(player);player->chp=player->mhp;
    borg_init();if(borg_init_failure||borg_cfg[BORG_CHEAT_DEATH]||OPT(player,cheat_live))return 1;
    borg_equipment();borg_inventory();borg_notice(true);borg.c=player->grid;borg.avoidance=player->chp;
    mode=getenv("CBUG0061_MODE");if(!mode)mode="long-gap";
    event_add_handler(EVENT_MESSAGE,message,NULL);
    printf("SETUP seed=%d class=Warrior level=%d depth=%d max_hp=%d potions=%d\n",initial_seed,player->lev,player->depth,player->mhp,potion_count());
    return 0;
}
int teardown_tests(void *state) {cleanup_angband();term_nuke(&screen);angband_term[0]=NULL;return 0;}
static int scenario(void *state) {
    bool fresh=streq(mode,"first-use")||streq(mode,"first-turn");
    bool bypass=streq(mode,"bypass-control");
    int elapsed=streq(mode,"recent")||bypass?1:streq(mode,"boundary-four")?4:streq(mode,"same-turn")?0:5;
    borg.time.now=100;take_hit(player,120,"controlled setup injury");borg_notice(true);
    if(!fresh) {printf("PRIME ");require(execute(true));}
    else if(streq(mode,"first-turn"))borg.time.now=1;
    if(!fresh) {borg.time.now+=elapsed;take_hit(player,40,"controlled second injury");}
    if(streq(mode,"reserve-control")) {potions->number=1;calc_inventory(player);borg_inventory();borg_notice(true);}
    if(streq(mode,"empty-control")) {bool none_left; struct object *discard=gear_object_for_use(player,potions,potions->number,false,&none_left);object_delete(cave,player->cave,&discard);calc_inventory(player);borg_inventory();borg_notice(true);}
    player_inc_timed(player,TMD_CONFUSED,10,true,true,true);borg_notice_player();
    printf("MODE %s time=%d elapsed=%d available=%d\n",mode,borg.time.now,fresh?-1:elapsed,borg.trait[BI_ACCW]);
    frame("start");int hp=player->chp,n=potion_count();
    bool decision=execute(bypass);frame("potion");
    printf("OUTCOME used=%d hp_before=%d hp_after=%d healed=%d potions_spent=%d confused=%d\n",decision,hp,player->chp,player->chp-hp,n-potion_count(),player->timed[TMD_CONFUSED]);
    bool expected=!(streq(mode,"recent")||streq(mode,"boundary-four")||streq(mode,"reserve-control")||streq(mode,"empty-control"));
    eq(decision,expected);
    if(expected) {eq(n-potion_count(),1);eq(player->chp-hp,60);eq(player->timed[TMD_CONFUSED],0);}
    else {eq(n,potion_count());eq(hp,player->chp);require(player->timed[TMD_CONFUSED]>0);}
    ok;
}
const char *suite_name="native-c/CBUG-0061";
struct test tests[]={{"potion_timing_respects_recent_use",scenario},{NULL,NULL}};
