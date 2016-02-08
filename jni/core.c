#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include "defs.h"

int core(struct state *state){
	if(state->showmenu){
		if(!menu_main(state))return false;
		state->showmenu=false;
	}
	if(state->back){
		state->back=false;
		state->showmenu=true;
		return core(state);
	}
	
	state->player.yv+=GRAVITY;
	state->player.base.y+=state->player.yv;
	state->player.base.x+=state->player.xv;
	for(int i=0;i<BLOCK_COUNT;++i){
		if(state->block[i].hidden)continue;
		int side;
		if(side=correct(&state->player.base,&state->block[i].base)){
			if(side==COLLIDE_TOP){
				state->player.yv=0.0f;
				state->player.base.y=state->block[i].base.y-PLAYER_HEIGHT;
			}
			else if(side==COLLIDE_LEFT){
				state->player.xv=0.0f;
				state->player.base.x=state->block[i].base.x-PLAYER_WIDTH;
			}
			else{
				state->player.xv=0.0f;
				state->player.base.x=state->block[i].base.x+state->block[i].base.w;
			}
		}
	}
	
	state->lbuttonstate=pointing(state->pointer,&state->lbutton);
	state->rbuttonstate=pointing(state->pointer,&state->rbutton);
	if(state->lbuttonstate){
		state->player.xv-=PLAYER_ACCELERATE;
		if(state->player.xv<-PLAYER_MAX_SPEED)state->player.xv=-PLAYER_MAX_SPEED;
	}
	else if(state->rbuttonstate){
		state->player.xv+=PLAYER_ACCELERATE;
		if(state->player.xv>PLAYER_MAX_SPEED)state->player.xv=PLAYER_MAX_SPEED;
	}
	else zerof(&state->player.xv,PLAYER_ACCELERATE);
	if(state->jbuttonstate=pointing(state->pointer,&state->jbutton)){
	}
	if(state->fbuttonstate=pointing(state->pointer,&state->fbutton)){
	}
	return true;
}

void render(struct state *state){
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
	uidraw(state,&state->background,0);
	
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLOCK].object);
	glUniform4f(state->uniform.rgba,1.0f,0.0f,0.0f,1.0f);
	uidraw(state,&state->lava,0);
	
	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	for(int i=0;i<BLOCK_COUNT;++i){
		if(!state->block[i].hidden)draw(state,&state->block[i].base,0);
		/*else{
			glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,0.3f);
			draw(state,&state->block[i].base,0);
			glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		}*/
	}
	
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON2].object);
	uidraw(state,&state->lbutton,state->lbuttonstate);
	uidraw(state,&state->rbutton,state->rbuttonstate);
	uidraw(state,&state->jbutton,state->jbuttonstate);
	uidraw(state,&state->fbutton,state->fbuttonstate);
	
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
	draw(state,&state->player.base,0);
	
	{
		static int fps,lasttime=0;
		static char fpsstring[20];
		if(lasttime!=time(NULL)){
			lasttime=time(NULL);
			sprintf(fpsstring,"Fps: %d",fps);
			fps=0;
		}
		++fps;
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtext(state->font.main,state->rect.left+0.1f,state->rect.top+0.1f,fpsstring);
	}
}

void init(struct state *state){
	state->showmenu=true;
	state->back=false;
	memset(state->pointer,0,sizeof(struct crosshair)*2);
	state->rect.left=-(state->rect.right=8.0f);
	state->rect.top=-(state->rect.bottom=4.5f);
	state->background.x=state->rect.left;
	state->background.y=state->rect.top;
	state->background.w=state->rect.right*2.0f;
	state->background.h=state->rect.bottom*2.0f;
	state->background.rot=0.0f;
	state->background.count=1.0f;
	state->lbutton=(struct base){-7.0f,2.0f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->rbutton=(struct base){-4.75f,2.0f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->jbutton=(struct base){5.75f,2.0f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->fbutton=(struct base){5.75f,-0.25f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;
	state->player.base.rot=0.0f;
	state->player.base.count=8.0f;
	state->lava.x=state->rect.left;
	state->lava.y=3.75f;
	state->lava.w=state->rect.right*2.0f;
	state->lava.h=0.75f;
	state->lava.rot=0.0f;
	state->lava.count=1.0f;
}
void reset(struct state *state){
	newblocks(state);
	state->player.base.x=0.0f;
	state->player.base.y=state->block[1].base.y-PLAYER_HEIGHT;
	state->player.xv=0.0f;
	state->player.yv=0.0f;
}