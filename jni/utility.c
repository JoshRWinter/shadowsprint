#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <math.h>
#include "defs.h"

static const char *enemyphrase[]={
	"I See You!!",
	"FREEZE!",
	"STOP RIGHT THERE",
	"AHHHH",
	"GET 'EM!",
	"Boo!"
};
const char *getenemyphrase(){
	return enemyphrase[randomint(0,5)];
}

static const char *playerblastphrase[]={
	"Gotcha!",
	"Bull's eye!",
	"Headshot.",
	"Noice",
	"Good night."
};
const char *getplayerblastphrase(){
	return playerblastphrase[randomint(0,4)];
}

static const char *playerstompphrase[]={
	"Pow!",
	"Crunch.",
	"Take that!"
};
const char *getplayerstompphrase(){
	return playerstompphrase[randomint(0,2)];
}

static const char *playerresphrase[]={
	"Death cannot stop me!",
	"I WILL HAVE MY REVENGE",
	"I LIVE AGAIN",
	"I'm back, baby.",
	"ARRRGGGHHH"
};
const char *getplayerresphrase(){
	return playerresphrase[randomint(0,4)];
}

void setbuttoncolor(struct state *state){
	switch(state->level){
		case 1:
			glUniform4f(state->uniform.rgba,COLOR_WHITE);
			break;
		case 2:
			glUniform4f(state->uniform.rgba,COLOR_NIGHT);
			break;
		case 3:
			glUniform4f(state->uniform.rgba,COLOR_MORNIN);
			break;
	}
}

void whiteout(struct state *state){
	const float FADE=0.015f;
	struct base base={state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f};
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,state->whiteout);
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLOCK].object);
	uidraw(state,&base,0.0f);
	if(state->enablewhiteout){
		if((state->whiteout+=FADE)>1.0f){
			state->whiteout=1.0f;
			state->enablewhiteout=false;
		}
	}
	else{
		zerof(&state->whiteout,FADE);
	}
}

int buttonprocess0(struct state *state,struct button *button){
	if(state->pointer[0].x>button->base.x&&state->pointer[0].x<button->base.x+button->base.w&&
	state->pointer[0].y>button->base.y&&state->pointer[0].y<button->base.y+button->base.h){
		if(!state->pointer[0].active){
			if(button->active){
				button->active=false;
				return BUTTON_ACTIVATE;
			}
		}
		else{
			button->active=true;
			return BUTTON_PRESS;
		}
	}
	return 0;
}
int buttonprocess1(struct state *state,struct button *button){
	if(state->pointer[1].x>button->base.x&&state->pointer[1].x<button->base.x+button->base.w&&
	state->pointer[1].y>button->base.y&&state->pointer[1].y<button->base.y+button->base.h){
		if(!state->pointer[1].active){
			if(button->active){
				button->active=false;
				return BUTTON_ACTIVATE;
			}
		}
		else{
			button->active=true;
			return BUTTON_PRESS;
		}
	}
	button->active=false;
	return 0;
}
int buttonprocess(struct state *state,struct button *button){
	int bstate0=buttonprocess0(state,button);
	if(bstate0)return bstate0;
	int bstate1=buttonprocess1(state,button);
	if(bstate1)return bstate1;
	return 0;
}
int buttondraw(struct state *state,struct button *button){
	int bstate=buttonprocess(state,button);
	uidraw(state,&button->base,bstate==BUTTON_PRESS?1.0f:0.0f);
	return bstate;
}
void buttondrawtext(ftfont* font,struct button *button){
	drawtextcentered(font,button->base.x+(button->base.w/2.0f),button->base.y+(button->base.h/2.0f)-(font->fontsize/2.0f)+(button->active?0.0f:-0.1f),button->label);
}

void saveconf(struct state *state){
	FILE *file=fopen(DATAPATH"d00","wb");
	if(!file){
		logcat("error: could not open "DATAPATH"d00 for writing.");
		return;
	}
	fwrite(&state->musicenabled,sizeof(int),1,file);
	fwrite(&state->soundenabled,sizeof(int),1,file);
	fwrite(&state->vibenabled,sizeof(int),1,file);
	fclose(file);
}
int readconf(struct state *state){
	FILE *file=fopen(DATAPATH"d00","rb");
	if(!file)return false;
	fread(&state->musicenabled,sizeof(int),1,file);
	fread(&state->soundenabled,sizeof(int),1,file);
	fread(&state->vibenabled,sizeof(int),1,file);
	fclose(file);
	return true;
}

int pointing0(struct crosshair *pointer,struct base *base){
	return pointer[0].active&&pointer[0].x>base->x&&pointer[0].x<base->x+base->w&&pointer[0].y>base->y&&pointer[0].y<base->y+base->h;
}
int pointing1(struct crosshair *pointer,struct base *base){
	return pointer[1].active&&pointer[1].x>base->x&&pointer[1].x<base->x+base->w&&pointer[1].y>base->y&&pointer[1].y<base->y+base->h;
}
int pointing(struct crosshair *pointer,struct base *base){
	return pointing0(pointer,base)||pointing1(pointer,base);
}
int inrange(struct base *a,struct base *b,float range){
	return fabs((a->x+(a->w/2.0f))-(b->x+(b->w/2.0f)))<range;
}

int collide(struct base *a,struct base *b){
	return a->x+a->w>b->x&&a->x<b->x+b->w&&a->y+a->h>b->y&&a->y<b->y+b->h;
}
int correct(struct base *a,struct base *b){
	if(!collide(a,b))return false;
	float ldiff,rdiff,tdiff;
	ldiff=fabs((a->x+a->w)-b->x);
	rdiff=fabs(a->x-(b->x+b->w));
	tdiff=fabs((a->y+a->h)-b->y)/2.0f;
	float smallest=ldiff;
	if(rdiff<smallest)smallest=rdiff;
	if(tdiff<smallest)smallest=tdiff;
	if(smallest==tdiff)return COLLIDE_TOP;
	else if(smallest==ldiff)return COLLIDE_LEFT;
	else return COLLIDE_RIGHT;
}
void uidraw(struct state *state,struct base *target,float sprite){
	float size=1.0f/target->count;
	float pos=size*sprite;
	glUniform4f(state->uniform.texcoords,pos,pos+size,0.0f,1.0f);
	glUniform2f(state->uniform.vector,target->x,target->y);
	glUniform2f(state->uniform.size,target->w,target->h);
	glUniform1f(state->uniform.rot,target->rot);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}
void draw(struct state *state,struct base *target,float sprite,int xinvert){
	float size=1.0f/target->count;
	float pos=size*sprite;
	if(!xinvert)
		glUniform4f(state->uniform.texcoords,pos,pos+size,0.0f,1.0f);
	else
		glUniform4f(state->uniform.texcoords,pos+size,pos,0.0f,1.0f);
	glUniform2f(state->uniform.vector,target->x-((state->player.base.x)+(PLAYER_WIDTH/2.0f))-2.5f,target->y);
	glUniform2f(state->uniform.size,target->w,target->h);
	glUniform1f(state->uniform.rot,target->rot);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}