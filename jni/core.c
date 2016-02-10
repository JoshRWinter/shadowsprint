#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <math.h>
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
	
	if(state->player.reload)--state->player.reload;
	state->player.yv+=GRAVITY;
	if(state->player.xv>0.0f)state->player.xinvert=false;
	else if(state->player.xv<0.0f)state->player.xinvert=true;
	state->player.base.y+=state->player.yv;
	state->player.base.x+=state->player.xv;
	for(int i=0;i<BLOCK_COUNT;++i){
		if(state->block[i].hidden)continue;
		int side;
		if(side=correct(&state->player.base,&state->block[i].base)){
			if(side==COLLIDE_TOP){
				state->player.yv=0.0f;
				state->player.base.y=state->block[i].base.y-PLAYER_HEIGHT;
				state->player.canjump=true;
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
	if(!state->player.canjump){
		state->player.frametimer=0;
		state->player.frame=6;
	}
	else if(state->player.xv!=0.0f){
		if(state->player.frametimer++==PLAYER_FRAME_TIMER){
			state->player.frametimer=0;
			if(++state->player.frame>5)state->player.frame=1;
		}
	}
	else{
		state->player.frametimer=0;
		state->player.frame=0;
	}
	
	for(struct blast *blast=state->blastlist,*prevblast=NULL;blast!=NULL;){
		blast->base.x+=blast->xv;
		newsmoke(state,&blast->base);
		if(blast->frametimer++>4){
			blast->frametimer=0;
			if(++blast->frame>3)blast->frame=0;
		}
		if(!blast->ttl--){
			newparticle(state,blast->base.x+(BLAST_WIDTH/2.0f),blast->base.y+(BLAST_HEIGHT/2.0f),20);
			blast=deleteblast(state,blast,prevblast);
			continue;
		}
		int stop=false;
		for(int i=0;i<BLOCK_COUNT;++i){
			if(state->block[i].hidden)continue;
			if(collide(&state->block[i].base,&blast->base)){
				newparticle(state,blast->xv>0.0f?blast->base.x+BLAST_WIDTH:blast->base.x,blast->base.y+(BLAST_HEIGHT/2.0f),20);
				blast=deleteblast(state,blast,prevblast);
				stop=true;
				break;
			}
		}
		if(stop)continue;
		prevblast=blast;
		blast=blast->next;
	}
	
	for(struct particle *particle=state->particlelist,*prevparticle=NULL;particle!=NULL;){
		for(int i=1;i<BLOCK_COUNT;++i){
			if(state->block[i].hidden)continue;
			int side=correct(&particle->base,&state->block[i].base);
			if(side&&particle->ttl>0){
				if(side==COLLIDE_TOP){
					particle->yv/=-1.4f;
					particle->base.y=state->block[i].base.y-PARTICLE_SIZE;
				}
				else if(side==COLLIDE_LEFT){
					particle->xv/=-1.2f;
					particle->base.x=state->block[i].base.x-PARTICLE_SIZE;
				}
				else{
					particle->xv/=-1.2f;
					particle->base.x=state->block[i].base.x+state->block[i].base.w;
				}
			}
		}
		if(particle->ttl--<1){
			particle->base.y+=0.002f;
			if(particle->ttl<-80){
				particle=deleteparticle(state,particle,prevparticle);
				continue;
			}
		}
		else if(particle->base.y>state->rect.bottom){
			particle=deleteparticle(state,particle,prevparticle);
			continue;
		}
		particle->xv/=1.08f;
		particle->yv+=GRAVITY;
		particle->base.rot+=particle->xv*3.0f;
		particle->base.x+=particle->xv;
		if(particle->ttl>0)particle->base.y+=particle->yv;
		
		prevparticle=particle;
		particle=particle->next;
	}
	
	for(struct smoke *smoke=state->smokelist,*prevsmoke=NULL;smoke!=NULL;){
		if((smoke->alpha-=randomint(0,1)/100.0f)<0.0f){
			smoke=deletesmoke(state,smoke,prevsmoke);
			continue;
		}
		prevsmoke=smoke;
		smoke=smoke->next;
	}
	
	// buttons
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
		if(state->player.canjump){
			state->player.canjump=false;
			state->player.yv=PLAYER_JUMP;
		}
	}
	if((state->fbuttonstate=pointing(state->pointer,&state->fbutton))&&!state->player.reload){
		newblast(state);
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
		if(!state->block[i].hidden)draw(state,&state->block[i].base,0,false);
		/*else{
			glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,0.3f);
			draw(state,&state->block[i].base,0);
			glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		}*/
	}
	
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
	draw(state,&state->player.base,state->player.frame,state->player.xinvert);
	
	if(state->particlelist){
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLOCK].object);
		for(struct particle *particle=state->particlelist;particle!=NULL;particle=particle->next)
			draw(state,&particle->base,0,false);
	}
	
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=smoke->next){
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,smoke->alpha);
		draw(state,&smoke->base,0,false);
	}
	
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	if(state->blastlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLAST].object);
		for(struct blast *blast=state->blastlist;blast!=NULL;blast=blast->next)
			draw(state,&blast->base,blast->frame,blast->xv>0.0f?false:true);
	}
	
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON2].object);
	uidraw(state,&state->lbutton,state->lbuttonstate);
	uidraw(state,&state->rbutton,state->rbuttonstate);
	uidraw(state,&state->jbutton,state->jbuttonstate);
	uidraw(state,&state->fbutton,state->fbuttonstate);
	
	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->font.symbol->atlas);
	drawtextcentered(state->font.symbol,state->lbutton.x+(BUTTONSMALL_SIZE/2.0f),
	state->lbutton.y+(BUTTONSMALL_SIZE/2.0f)-(state->font.symbol->fontsize/2.0f),"W"); // wingdings3 left
	drawtextcentered(state->font.symbol,state->rbutton.x+(BUTTONSMALL_SIZE/2.0f),
	state->rbutton.y+(BUTTONSMALL_SIZE/2.0f)-(state->font.symbol->fontsize/2.0f),"X"); // wingdings3 right
	drawtextcentered(state->font.symbol,state->jbutton.x+(BUTTONSMALL_SIZE/2.0f),
	state->jbutton.y+(BUTTONSMALL_SIZE/2.0f)-(state->font.symbol->fontsize/2.0f),"S"); // wingdings3 up
	glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
	drawtextcentered(state->font.header,state->fbutton.x+(BUTTONSMALL_SIZE/2.0f),
	state->fbutton.y+(BUTTONSMALL_SIZE/2.0f)-(state->font.header->fontsize/2.0f),"X");
	
	{
		static int fps,lasttime=0;
		static char fpsstring[20];
		if(lasttime!=time(NULL)){
			lasttime=time(NULL);
			sprintf(fpsstring,"Fps: %d",fps);
			fps=0;
		}
		++fps;
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
	state->background=(struct base){state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f};
	state->lbutton=(struct base){-7.0f,2.0f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->rbutton=(struct base){-4.75f,2.0f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->jbutton=(struct base){5.75f,2.0f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->fbutton=(struct base){5.75f,-0.25f,BUTTONSMALL_SIZE,BUTTONSMALL_SIZE,0.0f,2.0f};
	state->buttonframe=(struct base){state->rect.right-3.8125f,state->rect.top,3.8125f,state->rect.bottom*2.0f,0.0f,1.0f};
	state->lava=(struct base){state->rect.left,3.75f,state->rect.right*2.0f,0.75f,0.0f,1.0f};
	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;
	state->player.base.count=8.0f;
	state->blastlist=NULL;
	state->particlelist=NULL;
	state->smokelist=NULL;
}
void reset(struct state *state){
	for(struct blast *blast=state->blastlist;blast!=NULL;blast=deleteblast(state,blast,NULL));
	for(struct particle *particle=state->particlelist;particle!=NULL;particle=deleteparticle(state,particle,NULL));
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=deletesmoke(state,smoke,NULL));
	newblocks(state);
	state->player.base.x=0.0f;
	state->player.base.y=state->block[1].base.y-PLAYER_HEIGHT;
	state->player.xv=0.0f;
	state->player.yv=0.0f;
	state->player.frame=0;
	state->player.frametimer=0;
	state->player.base.rot=0.0f;
	state->player.xinvert=false;
	state->player.lives=3;
	state->player.canjump=true;
	state->player.reload=0;
}