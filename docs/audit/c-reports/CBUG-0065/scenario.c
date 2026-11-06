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
#include "borg/borg-flow-misc.h"
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
static struct object *potion;
static const char *mode;
static struct loc stairs;
static bool should_use;
static int count_potions(void) {int n=0;for(struct object*o=player->gear;o;o=o->next)if(o->kind==potion->kind)n+=o->number;return n;}
static void frame(const char*stage) {printf("FRAME {\"stage\":\"%s\",\"hp\":%d,\"potions\":%d,\"stairs_known\":%d,\"stairs_feat\":%d}\n",stage,player->chp,count_potions(),square_isknown(cave,stairs),square(player->cave,stairs)->feat);}
static void message(game_event_type type,game_event_data*data,void*user) {if(data->message.msg)printf("ENGINE %s\n",data->message.msg);}
static struct object *give(const char*name,int tval,int number,bool aware) {
 struct object*obj=object_new();object_prep(obj,lookup_kind(tval,lookup_sval(tval,name)),50,MAXIMISE);obj->number=number;
 obj->known=object_new();object_set_base_known(player,obj);
 if(aware)object_flavor_aware(player,obj);
 inven_carry(player,obj,false,false);
 if(aware){object_touch(player,obj);player_know_object(player,obj);}
 return obj;
}
int setup_tests(void **state) {
 const char*profile=getenv("CBUG0065_PROFILE");if(!profile)return 1;set_file_paths();
 string_free(ANGBAND_DIR_USER);ANGBAND_DIR_USER=string_make(profile);
 string_free(ANGBAND_DIR_SAVE);ANGBAND_DIR_SAVE=string_make(format("%s/save",profile));
 string_free(ANGBAND_DIR_PANIC);ANGBAND_DIR_PANIC=string_make(format("%s/panic",profile));
 string_free(ANGBAND_DIR_SCORES);ANGBAND_DIR_SCORES=string_make(format("%s/scores",profile));
 string_free(ANGBAND_DIR_ARCHIVE);ANGBAND_DIR_ARCHIVE=string_make(format("%s/archive",profile));
 if(!init_angband())return 1;
 create_needed_dirs();Rand_quick=false;Rand_state_init(1000);
 if(!player_make_simple("Human","Warrior","NativeEnlightenment"))return 1;
 term_init(&screen,160,60,256);Term_activate(&screen);angband_term[0]=&screen;
 cave=t_build_arena(40,40);player->cave=cave_new(40,40);player_place(cave,player,loc(20,20));character_dungeon=true;
 mode=getenv("CBUG0065_MODE");if(!mode)mode="dungeon";
 bool town=streq(mode,"town-control")||streq(mode,"augmentation-town");
 player->depth=player->max_depth=cave->depth=player->cave->depth=town?0:30;
 for(int y=1;y<39;y++)for(int x=1;x<39;x++)sqinfo_on(square(cave,loc(x,y))->info,SQUARE_GLOW);
 for(int y=1;y<35;y++)square_set_feat(cave,loc(25,y),FEAT_GRANITE);
 stairs=loc(30,20);square_set_feat(cave,stairs,FEAT_MORE);
 player_exp_gain(player,player_exp[38]*player->expfact/100);
 struct object*torch=give("Wooden Torch",TV_LIGHT,1,true);torch->timeout=5000;inven_wield(torch,slot_by_name(player,"light"));
 give("Ration of Food",TV_FOOD,3,true);
 const char*name=streq(mode,"augmentation-dungeon")||streq(mode,"augmentation-town")?"Augmentation":streq(mode,"unrelated-control")?"True Seeing":"Enlightenment";
 potion=give(name,TV_POTION,2,!streq(mode,"unknown-control"));
 player_learn_all_runes(player);calc_inventory(player);player->upkeep->update|=PU_BONUS|PU_HP|PU_MANA;update_stuff(player);player->chp=player->mhp;
 update_view(cave,player);
 if(square_isseen(cave,stairs)||square_isknown(cave,stairs))return 1;
 borg_init();if(borg_init_failure||borg_cfg[BORG_CHEAT_DEATH]||OPT(player,cheat_live))return 1;
 borg_equipment();borg_inventory();borg_notice(true);borg.c=player->grid;borg.avoidance=player->chp;borg.time.now=100;
 for(int y=0;y<40;y++)for(int x=0;x<40;x++)borg_grids[y][x].feat=square(player->cave,loc(x,y))->feat;
 should_use=streq(mode,"dungeon")||streq(mode,"augmentation-dungeon")||streq(mode,"augmentation-town");
 event_add_handler(EVENT_MESSAGE,message,NULL);
 printf("SETUP seed=1000 class=Warrior level=%d depth=%d hp=%d potion=%s aware=%d light=%d food=%d safe=%d\n",player->lev,player->depth,player->chp,name,potion->kind->aware,borg.trait[BI_LIGHT],borg.trait[BI_FOOD],borg_check_rest(borg.c.y,borg.c.x));
 printf("STAIRS x=%d y=%d native_feat=%d visible=%d known=%d\n",stairs.x,stairs.y,square(cave,stairs)->feat,square_isseen(cave,stairs),square_isknown(cave,stairs));
 return 0;
}
int teardown_tests(void*state) {cleanup_angband();term_nuke(&screen);angband_term[0]=NULL;return 0;}
static int scenario(void*state) {
 printf("MODE %s\n",mode);frame("start");int n=count_potions();
 bool used=borg_use_things();keycode_t keys[16],key;int len=0;while((key=borg_inkey(true))!=0&&len<16)keys[len++]=key;
 printf("DECISION used=%d keys=%d sequence=",used,len);for(int i=0;i<len;i++)printf("%c",(char)keys[i]);putchar('\n');
 if(used){require(len==2&&keys[0]=='q');struct object*obj=NULL;for(int i=0;i<z_info->pack_size;i++)if((keycode_t)all_letters_nohjkl[i]==keys[1])obj=player->upkeep->inven[i];require(obj==potion);struct command cmd={.code=CMD_QUAFF,.context=CTX_GAME};cmd_set_arg_item(&cmd,"item",obj);do_cmd_quaff_potion(&cmd);cmd_release(&cmd);calc_inventory(player);}
 else{eq(len,0);}
 frame("use");printf("OUTCOME used=%d potions_spent=%d stairs_known=%d stairs_feat=%d native_stairs=%d hp=%d\n",used,n-count_potions(),square_isknown(cave,stairs),square(player->cave,stairs)->feat,square(cave,stairs)->feat,player->chp);
 eq(used,should_use);
 if(should_use){eq(n-count_potions(),1);}else{eq(n,count_potions());}
 if(streq(mode,"dungeon")){require(square_isknown(cave,stairs));eq(square(player->cave,stairs)->feat,FEAT_MORE);}
 else{eq(square_isknown(cave,stairs),false);}
 ok;
}
const char*suite_name="native-c/CBUG-0065";
struct test tests[]={{"use_enlightenment_in_dungeon_only",scenario},{NULL,NULL}};
