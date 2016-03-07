#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <math.h>
#include "defs.h"

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
	uidraw(state,&button->base,bstate==BUTTON_PRESS?1:0);
	return bstate;
}
void buttondrawtext(ftfont* font,struct button *button){
	drawtextcentered(font,button->base.x+(button->base.w/2.0f),button->base.y+(button->base.h/2.0f)-(font->fontsize/2.0f)+(button->active?0.0f:-0.1f),button->label);
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