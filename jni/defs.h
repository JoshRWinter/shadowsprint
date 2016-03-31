#include "glesutil.h"
#define DATAPATH "/data/data/joshwinter.shadow/files/"
#define PI 3.14159f
#define PI2 (2.0f*PI)
#define TEX_MODE "11011011111"
#define COLLIDE_RIGHT 1
#define COLLIDE_TOP 2
#define COLLIDE_LEFT 3
#define GRAVITY 0.02f
#define LAVA_Y 3.75f
#define COLOR_RED 1
#define COLOR_BLACK 2
#define COLOR_WHITE 1.0f,1.0f,1.0f,1.0f
#define COLOR_NIGHT 0.75f,0.0f,1.0f,1.0f
#define COLOR_MORNIN 1.0f,0.45f,0.6f,1.0f
#define PHRASE_TIMER 200
#define torad(x) (x*(PI/180.0f))

// gameplay
#define TID_PLAYER 0
#define TID_ENEMY 1
#define TID_BLOCK 2
#define TID_BLAST 3
#define TID_SHOCKWAVE 4
#define TID_CLOUD 5
#define TID_FLARE  6
#define TID_SILO 7
#define TID_MISSILE 8
#define TID_DUST 9
#define TID_TELEPORTER 10

// ui
#define TID_BACKGROUND 0
#define TID_BUTTON 1
#define TID_BUTTONFRAME 2
#define TID_SYMBOL 3

// sound
#define SID_THEME 0

struct base{
	float x,y,w,h,rot,count;
};

#define PLAYER_RELOAD 15
#define PLAYER_JUMP -0.3f
#define PLAYER_FRAME_TIMER 4
#define PLAYER_MAX_SPEED 0.1f
#define PLAYER_ACCELERATE 0.01f
#define PLAYER_WIDTH 1.0166f
#define PLAYER_HEIGHT 1.3833f
struct player{
	struct base base;
	float xv,yv;
	int lives,canjump,frame,frametimer,xinvert,reload,success,dead;
	struct{const char *phrase;int timer;}text;
};

#define TELEPORTER_WIDTH 2.0f
#define TELEPORTER_HEIGHT 3.5f
struct teleporter{
	struct base base;
	int frame;
};

#define ENEMY_WIDTH 1.0f
#define ENEMY_HEIGHT 1.0f
#define ENEMY_SPEED 0.045f
#define ENEMY_ATTACK_SPEED 0.09f
#define ENEMY_COUNT 8
struct enemy{
	struct base base;
	float xv,yv;
	int attack; // attack mode
	struct{const char *phrase;int timer;}text;
	struct enemy *next;
};

#define BLOCK_COUNT 40
struct block{
	struct base base;
	int hidden;
};

#define BLAST_WIDTH 1.1f
#define BLAST_HEIGHT 0.4f
#define BLAST_SPEED 0.2125f
struct blast{
	struct base base;
	float xv;
	int ttl,frame,frametimer;
	struct blast *next;
};

#define PARTICLE_SIZE 0.1f
struct particle{
	struct base base;
	float xv,yv;
	int ttl,color;
	struct particle *next;
};

#define SHOCKWAVE_INFLATE 0.2f
#define SHOCKWAVE_FADE 0.05f
struct shockwave{
	struct base base;
	int color;
	float alpha;
	struct shockwave *next;
};

#define SMOKE_SIZE 0.1f
struct smoke{
	struct base base;
	int color;
	float alpha;
	struct smoke *next;
};

#define FLARE_SIZE 0.5f
struct flare{
	struct base base;
	float xv,yv;
	int frame,blockparent;
	struct flare *next;
};

#define SILO_WIDTH 1.0f
#define SILO_HEIGHT 0.5f
#define SILO_RANGE 5.0f
#define SILO_COUNT 5
struct missile;
struct silo{
	struct base base;
	struct missile *missile;
	int health;
	struct silo *next;
};

#define MISSILE_WIDTH 0.7f
#define MISSILE_HEIGHT 0.5f
#define MISSILE_TTL 420
#define MISSILE_SPEED 0.06f
struct missile{
	struct base base;
	float xv,yv;
	int ttl,dead;
	struct silo *silo;
	struct missile *next;
};

#define CLOUD_WIDTH 4.5f
#define CLOUD_HEIGHT 3.0f
#define CLOUD_BOUND -12.0f
struct cloud{
	struct base base;
	int xinvert;
	float xv;
	struct cloud *next;
};

struct dust{
	struct base base;
	int sprite,xflip;
	float xv,yv,rotv;
	struct dust *next;
};

#define BUTTON_WIDTH 1.9f
#define BUTTON_HEIGHT 2.0666f
#define BUTTON_PRESS 1
#define BUTTON_ACTIVATE 2
struct button{
	struct base base;
	char *label;
	int active;
};

struct state{
	int running,showmenu,back,musicenabled,soundenabled,vibenabled,showtut,level,skycolor,enablewhiteout;
	float ensmallen,whiteout;
	unsigned vao,vbo,program;
	struct pack assets,uiassets;
	struct apack aassets;
	struct android_app *app;
	slesenv *soundengine;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	struct{int vector,size,texcoords,rot,rgba,projection;}uniform;
	struct{ftfont *main,*header,*dialog;}font;
	struct{float left,right,bottom,top;}rect;
	struct crosshair pointer[2];
	struct device device;
	struct jni_info jni_info;
	struct base background,lava,buttonframe;
	struct teleporter teleporter;
	struct block block[BLOCK_COUNT];
	struct base lbutton,rbutton,jbutton,fbutton;
	struct button pbutton;
	int lbuttonstate,rbuttonstate,jbuttonstate,fbuttonstate,pbuttonstate;
	struct player player;
	struct enemy *enemylist;
	struct blast *blastlist;
	struct particle *particlelist;
	struct shockwave *shockwavelist;
	struct smoke *smokelist;
	struct flare *flarelist;
	struct silo *silolist;
	struct missile *missilelist;
	struct cloud *cloudlist;
	struct dust *dustlist;
};

int process(struct android_app*);
void init(struct state*);
void reset(struct state*);
void reset_level(struct state*);
int core(struct state*);
void render(struct state*);

int pointing(struct crosshair*,struct base*);
int buttonprocess(struct state*,struct button*);
int buttondraw(struct state*,struct button*);
void buttondrawtext(ftfont*,struct button*);
void uidraw(struct state*,struct base*,float);
void draw(struct state*,struct base*,float,int);
int collide(struct base*,struct base*);
int correct(struct base*,struct base*);
int menu_main(struct state*);
int menu_pause(struct state*);
int menu_conf(struct state*);
int menu_message(struct state*,const char*,const char*,int*);
void saveconf(struct state*);
int readconf(struct state*);
const char *getenemyphrase();
const char *getplayerblastphrase();
const char *getplayerstompphrase();
const char *getplayerresphrase();
void whiteout(struct state*);

void newdust(struct state*);
void dustroutine(struct state*);
void dustrender(struct state*);
struct dust *deletedust(struct state*,struct dust*,struct dust*);
void newblocks(struct state*);
void newblast(struct state*);
struct blast *deleteblast(struct state*,struct blast*,struct blast*);
void newenemy(struct state*,int);
struct enemy *deleteenemy(struct state*,struct enemy*,struct enemy*);
void newparticle(struct state *state,float,float,int,int);
struct particle *deleteparticle(struct state*,struct particle*,struct particle*);
void newshockwave(struct state*,float,float,int);
struct shockwave *deleteshockwave(struct state*,struct shockwave*,struct shockwave*);
void newsmoke(struct state*,struct base*,int);
struct smoke *deletesmoke(struct state*,struct smoke*,struct smoke*);
void newflare(struct state *state,int);
struct flare *deleteflare(struct state*,struct flare*,struct flare*);
void newsilo(struct state*,int);
struct silo *deletesilo(struct state*,struct silo*,struct silo*);
void newmissile(struct state*,struct silo*);
struct missile *deletemissile(struct state*,struct missile*,struct missile*);
void newcloud(struct state*);
struct cloud *deletecloud(struct state*,struct cloud*,struct cloud*);