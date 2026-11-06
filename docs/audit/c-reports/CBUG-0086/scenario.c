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
#include "mon-desc.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-defend.h"
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
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
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
static struct object *potions;
static struct monster *enemy;
static char name[160];
static const char *mode;
static int known;
static int count_potions(void) {int n=0;for(struct object*o=player->gear;o;o=o->next)if(o->tval==TV_POTION&&o->sval==sv_potion_detect_invis)n+=o->number;return n;}
static void message(game_event_type type,game_event_data*data,void*user) {if(data->message.msg)printf("ENGINE %s\n",data->message.msg);}
static void frame(const char*stage) {printf("FRAME {\"stage\":\"%s\",\"hp\":%d,\"potions\":%d,\"see_invisible\":%d}\n",stage,player->chp,count_potions(),player->timed[TMD_SINVIS]);}
int setup_tests(void **state) {
 const char *profile=getenv("CBUG0086_PROFILE");if(!profile)return 1;set_file_paths();
 string_free(ANGBAND_DIR_USER);ANGBAND_DIR_USER=string_make(profile);
 string_free(ANGBAND_DIR_SAVE);ANGBAND_DIR_SAVE=string_make(format("%s/save",profile));
 string_free(ANGBAND_DIR_PANIC);ANGBAND_DIR_PANIC=string_make(format("%s/panic",profile));
 string_free(ANGBAND_DIR_SCORES);ANGBAND_DIR_SCORES=string_make(format("%s/scores",profile));
 string_free(ANGBAND_DIR_ARCHIVE);ANGBAND_DIR_ARCHIVE=string_make(format("%s/archive",profile));
 if(!init_angband())return 1;
 create_needed_dirs();Rand_quick=false;Rand_state_init(1000);
 if(!player_make_simple("Human","Warrior","NativeNamedDragon"))return 1;
 term_init(&screen,160,60,256);Term_activate(&screen);angband_term[0]=&screen;
 cave=t_build_arena(40,40);player->cave=cave_new(40,40);player_place(cave,player,loc(20,20));character_dungeon=true;
 player->depth=player->max_depth=cave->depth=player->cave->depth=52;
 for(int y=1;y<39;y++)for(int x=1;x<39;x++)sqinfo_on(square(cave,loc(x,y))->info,SQUARE_GLOW);
 player_exp_gain(player,player_exp[38]*player->expfact/100);
 potions=object_new();object_prep(potions,lookup_kind(TV_POTION,lookup_sval(TV_POTION,"True Seeing")),50,MAXIMISE);
 potions->number=2;potions->known=object_new();object_set_base_known(player,potions);object_flavor_aware(player,potions);
 inven_carry(player,potions,false,false);object_touch(player,potions);player_know_object(player,potions);struct object *ring=object_new();object_prep(ring,lookup_kind(TV_RING,lookup_sval(TV_RING,"Flames")),50,MAXIMISE);
 ring->known=object_new();object_set_base_known(player,ring);object_flavor_aware(player,ring);inven_carry(player,ring,false,false);object_touch(player,ring);player_know_object(player,ring);inven_wield(ring,slot_by_name(player,"right hand"));
 player_inc_timed(player,TMD_OPP_FIRE,50,true,true,true);
 player_learn_all_runes(player);calc_inventory(player);
 player->upkeep->update|=PU_BONUS|PU_HP|PU_MANA;update_stuff(player);player->chp=player->mhp;
 borg_init();if(borg_init_failure||borg_cfg[BORG_CHEAT_DEATH]||OPT(player,cheat_live))return 1;
 borg_equipment();borg_inventory();borg_notice(true);borg.c=player->grid;borg.avoidance=player->chp;borg.time.now=100;
 for(int y=0;y<40;y++)for(int x=0;x<40;x++) {borg_grids[y][x].feat=square(cave,loc(x,y))->feat;borg_grids[y][x].info|=BORG_GLOW;}
 mode=getenv("CBUG0086_MODE");if(!mode)mode="named-dragon";
 const char *race=streq(mode,"ogre-control")?"Ogre":(streq(mode,"unseen-control")||streq(mode,"it-control")||streq(mode,"lower-it-control"))?"ghost":"Itangast the Fire Drake";
 enemy=t_add_monster(cave,loc(20,14),race);update_view(cave,player);update_mon(enemy,cave,true);
 if((streq(mode,"unseen-control")||streq(mode,"it-control")||streq(mode,"lower-it-control"))) {if(mflag_has(enemy->mflag,MFLAG_VISIBLE))return 1;}
 else {
  if(!mflag_has(enemy->mflag,MFLAG_VISIBLE))return 1;
  if(!observe_kill_diff(enemy->grid.y,enemy->grid.x,0,0))return 1;
  known=borg_grids[enemy->grid.y][enemy->grid.x].kill;if(!known)return 1;
 }
 monster_desc(name,sizeof(name),enemy,streq(mode,"it-control")?(MDESC_DEFAULT|MDESC_CAPITAL):streq(mode,"lower-it-control")?MDESC_DEFAULT:MDESC_STANDARD);
 event_add_handler(EVENT_MESSAGE,message,NULL);
 printf("SETUP seed=1000 class=Warrior level=%d depth=%d hp=%d monster=%s visible=%d known_index=%d\n",player->lev,player->depth,player->chp,enemy->race->name,mflag_has(enemy->mflag,MFLAG_VISIBLE),known);
 printf("NATIVE_NAME %s\n",name);
 return 0;
}
int teardown_tests(void*state) {cleanup_angband();term_nuke(&screen);angband_term[0]=NULL;return 0;}
static int scenario(void*state) {
 printf("MODE %s\n",mode);frame("start");
 int stamp=borg.need_see_invis;
 int located=borg_locate_kill(name,player->grid,z_info->max_range);
 int p=borg_danger(borg.c.y,borg.c.x,1,true,false);
 printf("MESSAGE located=%d stamp_before=%d stamp_after=%d danger=%d limit=%d\n",located,stamp,borg.need_see_invis,p,borg.avoidance*2);
 int n=count_potions();bool defend=borg_defend(p);
 keycode_t keys[128],key;int len=0;while((key=borg_inkey(true))!=0&&len<128)keys[len++]=key;
 printf("DECISION defend=%d keys=%d sequence=",defend,len);for(int i=0;i<len;i++){if(keys[i]==ESCAPE)printf("<Escape>");else printf("%c",(char)keys[i]);}putchar('\n');
 if(len) {
  require(len==2&&keys[0]=='q');struct object*obj=NULL;
  for(int i=0;i<z_info->pack_size;i++)if((keycode_t)all_letters_nohjkl[i]==keys[1])obj=player->upkeep->inven[i];
  require(obj&&obj->tval==TV_POTION&&obj->sval==sv_potion_detect_invis);
  struct command cmd={.code=CMD_QUAFF,.context=CTX_GAME};cmd_set_arg_item(&cmd,"item",obj);do_cmd_quaff_potion(&cmd);cmd_release(&cmd);calc_inventory(player);
 }
 frame("defense");
 printf("OUTCOME located=%d defend=%d potions_spent=%d see_invisible=%d stamp=%d hp=%d\n",located,defend,n-count_potions(),player->timed[TMD_SINVIS],borg.need_see_invis,player->chp);
 bool unseen=(streq(mode,"unseen-control")||streq(mode,"it-control")||streq(mode,"lower-it-control"));
 if(unseen){eq(located,0);require(defend);eq(n-count_potions(),1);require(player->timed[TMD_SINVIS]>0);}
 else {eq(located,known);eq(defend,false);eq(n,count_potions());eq(player->timed[TMD_SINVIS],0);eq(borg.need_see_invis,stamp);}
 ok;
}
const char *suite_name="native-c/CBUG-0086";
struct test tests[]={{"visible_name_does_not_request_invisible_search",scenario},{NULL,NULL}};
