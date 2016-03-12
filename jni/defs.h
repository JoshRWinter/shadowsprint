#include "glesutil.h"
#define DATAPATH "/data/data/joshwinter.shadow/files/"
#define PI 3.14159f
#define PI2 (2.0f*PI)
#define TEX_MODE "110110"
#define COLLIDE_RIGHT 1
#define COLLIDE_TOP 2
#define COLLIDE_LEFT 3
#define GRAVITY 0.02f
#define torad(x) (x*(PI/180.0f))

// gameplay
#define TID_PLAYER 0
#define TID_ENEMY 1
#define TID_BLOCK 2
#define TID_BLAST 3
#define TID_SHOCKWAVE 4
#define TID_CLOUD 5

//ui
#define TID_BACKGROUND 0
#define TID_BUTTON 1
#define TID_BUTTONFRAME 2
#define TID_SYMBOL 3

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
	int lives,canjump,frame,frametimer,xinvert,reload;
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
	int ttl;
	struct particle *next;
};

#define SHOCKWAVE_INFLATE 0.2f
#define SHOCKWAVE_FADE 0.05f
struct shockwave{
	struct base base;
	float alpha;
	struct shockwave *next;
};

#define SMOKE_SIZE 0.1f
struct smoke{
	struct base base;
	float alpha;
	struct smoke *next;
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
	int running,showmenu,back,musicenabled,soundenabled,vibenabled;
	unsigned vao,vbo,program;
	struct pack assets,uiassets;
	struct apack aassets;
	struct android_app *app;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	struct{int vector,size,texcoords,rot,rgba,projection;}uniform;
	struct{ftfont *main,*header;}font;
	struct{float left,right,bottom,top;}rect;
	struct crosshair pointer[2];
	struct device device;
	struct jni_info jni_info;
	struct base background,lava,buttonframe;
	struct block block[BLOCK_COUNT];
	struct base lbutton,rbutton,jbutton,fbutton;
	struct button pbutton;
	int lbuttonstate,rbuttonstate,jbuttonstate,fbuttonstate,pbuttonstate;
	struct player player;
	struct blast *blastlist;
	struct particle *particlelist;
	struct shockwave *shockwavelist;
	struct smoke *smokelist;
	struct cloud *cloudlist;
};

int process(struct android_app*);
void init(struct state*);
void reset(struct state*);
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
int menu_message(struct state*,const char*,const char*,int*);

void newblocks(struct state*);
void newblast(struct state*);
struct blast *deleteblast(struct state*,struct blast*,struct blast*);
void newparticle(struct state *state,float,float,int);
struct particle *deleteparticle(struct state*,struct particle*,struct particle*);
void newshockwave(struct state*,float,float);
struct shockwave *deleteshockwave(struct state*,struct shockwave*,struct shockwave*);
void newsmoke(struct state*,struct base*);
struct smoke *deletesmoke(struct state*,struct smoke*,struct smoke*);
void newcloud(struct state*);
struct cloud *deletecloud(struct state*,struct cloud*,struct cloud*);