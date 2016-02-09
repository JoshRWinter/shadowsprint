#include "glesutil.h"
#define DATAPATH "/data/data/joshwinter.shadow/files/"
#define PI 3.14159f
#define PI2 (2.0f*PI)
#define TEX_MODE "110"
#define COLLIDE_RIGHT 1
#define COLLIDE_TOP 2
#define COLLIDE_LEFT 3
#define GRAVITY 0.02f

// gameplay
#define TID_PLAYER 0
#define TID_ENEMY 1
#define TID_BLOCK 2

//ui
#define TID_BACKGROUND 0
#define TID_BUTTON 1
#define TID_BUTTON2 2
#define TID_BUTTONFRAME 3

struct base{
	float x,y,w,h,rot,count;
};

#define PLAYER_JUMP -0.3f
#define PLAYER_FRAME_TIMER 4
#define PLAYER_MAX_SPEED 0.1f
#define PLAYER_ACCELERATE 0.01f
#define PLAYER_WIDTH 1.0125f
#define PLAYER_HEIGHT 1.3875f
struct player{
	struct base base;
	float xv,yv;
	int lives,canjump,frame,frametimer;
};

#define BLOCK_COUNT 40
struct block{
	struct base base;
	int hidden;
};

#define BUTTON_WIDTH 3.3f
#define BUTTON_HEIGHT 1.75f
#define BUTTONSMALL_SIZE 2.0f
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
	struct vibrate vibrate;
	struct base background,lava,buttonframe;
	struct block block[BLOCK_COUNT];
	struct base lbutton,rbutton,jbutton,fbutton;
	int lbuttonstate,rbuttonstate,jbuttonstate,fbuttonstate;
	struct player player;
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
void draw(struct state*,struct base*,float);
int menu_main(struct state*);
int menu_message(struct state*,const char*,const char*,int*);

int collide(struct base*,struct base*);
int correct(struct base*,struct base*);
void newblocks(struct state*);