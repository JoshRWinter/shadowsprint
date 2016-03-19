#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <math.h>
#include "defs.h"

int core(struct state *state){
	if(state->showmenu){
		if(!menu_main(state))return false;
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
	
	for(struct enemy *enemy=state->enemylist,*prevenemy=NULL;enemy!=NULL;){
		if(enemy->text.timer)--enemy->text.timer;
		if(enemy->xv>0)enemy->base.x+=enemy->attack?ENEMY_ATTACK_SPEED:ENEMY_SPEED;
		else enemy->base.x+=enemy->attack?-ENEMY_ATTACK_SPEED:-ENEMY_SPEED;
		enemy->base.y+=enemy->yv;
		enemy->yv+=GRAVITY;
		enemy->base.rot=enemy->yv;
		int stop=false,side;
		for(int i=0;i<BLOCK_COUNT;++i){
			if(side=correct(&enemy->base,&state->block[i].base)){
				switch(side){
					case COLLIDE_TOP:
						if(enemy->yv>0.0f){
							enemy->base.y=state->block[i].base.y-ENEMY_HEIGHT;
							enemy->yv=-0.175f;
						}
						break;
					case COLLIDE_RIGHT:
						enemy->attack=false;
						enemy->text.timer=0;
						if(state->block[i].hidden)enemy->xv=-enemy->xv;
						else enemy->yv=-0.25f;
						enemy->base.x=state->block[i].base.x+state->block[i].base.w;
						break;
					case COLLIDE_LEFT:
						enemy->attack=false;
						enemy->text.timer=0;
						if(state->block[i].hidden)enemy->xv=-enemy->xv;
						else enemy->yv=-0.25f;
						enemy->base.x=state->block[i].base.x-ENEMY_WIDTH;
						break;
				}
			}
		}
		side=correct(&state->player.base,&enemy->base);
		if(side==COLLIDE_TOP){
			newparticle(state,enemy->base.x+(ENEMY_WIDTH/2.0f),enemy->base.y+(ENEMY_HEIGHT/2.0f),20,COLOR_BLACK);
			enemy=deleteenemy(state,enemy,prevenemy);
			continue;
		}
		else if(side==COLLIDE_LEFT||side==COLLIDE_RIGHT){
		}
		for(struct blast *blast=state->blastlist,*prevblast=NULL;blast!=NULL;){
			if(collide(&blast->base,&enemy->base)){
				newparticle(state,blast->base.x+(BLAST_WIDTH/2.0f),blast->base.y+(BLAST_HEIGHT/2.0f),20,COLOR_BLACK);
				blast=deleteblast(state,blast,prevblast);
				enemy=deleteenemy(state,enemy,prevenemy);
				stop=true;
				break;
			}
			prevblast=blast;
			blast=blast->next;
		}
		if(stop)continue;
		
		// can they see the player?
		if(!enemy->attack&&onein(10)&&fabs(enemy->base.x-state->player.base.x)<9.0f){
			struct base collider={enemy->base.x+(ENEMY_WIDTH/2.0f),enemy->base.y,0.1f,ENEMY_HEIGHT};
			const float ADVANCE=PLAYER_WIDTH-0.05f;
			for(int i=0;i<8;++i){
				for(int j=1;j<BLOCK_COUNT-1;++j){
					if(state->block[j].hidden){
						if(collide(&collider,&state->block[j].base)){
							stop=true;
							break;
						}
					}
				}
				if(stop)break;
				if(collide(&collider,&state->player.base)){
					enemy->text.timer=200;
					enemy->text.phrase=getenemyphrase();
					enemy->attack=true;
					break;
				}
				collider.x+=(enemy->xv>0.0f?ADVANCE:-ADVANCE);
			}
		}
		
		prevenemy=enemy;
		enemy=enemy->next;
	}
	
	for(struct blast *blast=state->blastlist,*prevblast=NULL;blast!=NULL;){
		blast->base.x+=blast->xv;
		newsmoke(state,&blast->base,COLOR_BLACK);
		if(blast->frametimer++>4){
			blast->frametimer=0;
			if(++blast->frame>3)blast->frame=0;
		}
		if(!blast->ttl--){
			newparticle(state,blast->base.x+(BLAST_WIDTH/2.0f),blast->base.y+(BLAST_HEIGHT/2.0f),20,COLOR_BLACK);
			blast=deleteblast(state,blast,prevblast);
			continue;
		}
		int stop=false;
		for(int i=0;i<BLOCK_COUNT;++i){
			if(state->block[i].hidden)continue;
			if(collide(&state->block[i].base,&blast->base)){
				newparticle(state,blast->xv>0.0f?blast->base.x+BLAST_WIDTH:blast->base.x,blast->base.y+(BLAST_HEIGHT/2.0f),20,COLOR_BLACK);
				blast=deleteblast(state,blast,prevblast);
				stop=true;
				break;
			}
		}
		if(stop)continue;
		for(struct silo *silo=state->silolist,*prevsilo=NULL;silo!=NULL;){
			if(collide(&blast->base,&silo->base)){
				if((silo->health-=randomint(40,60))<1){
					newparticle(state,blast->base.x+(BLAST_WIDTH/2.0f),blast->base.y+(BLAST_HEIGHT/2.0f),30,COLOR_BLACK);
					silo=deletesilo(state,silo,prevsilo);
				}
				else{
					newparticle(state,blast->base.x+(BLAST_WIDTH/2.0f),blast->base.y+(BLAST_HEIGHT/2.0f),10,COLOR_BLACK);
				}
				blast=deleteblast(state,blast,prevblast);
				stop=true;
				break;
			}
			prevsilo=silo;
			silo=silo->next;
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
	
	for(struct shockwave *shockwave=state->shockwavelist,*prevshockwave=NULL;shockwave!=NULL;){
		shockwave->base.w+=SHOCKWAVE_INFLATE;
		shockwave->base.h=shockwave->base.w;
		shockwave->base.x-=SHOCKWAVE_INFLATE/2.0f;
		shockwave->base.y-=SHOCKWAVE_INFLATE/2.0f;
		if((shockwave->alpha-=SHOCKWAVE_FADE)<0.0f){
			shockwave=deleteshockwave(state,shockwave,prevshockwave);
			continue;
		}
		prevshockwave=shockwave;
		shockwave=shockwave->next;
	}
	
	for(struct smoke *smoke=state->smokelist,*prevsmoke=NULL;smoke!=NULL;){
		if((smoke->alpha-=randomint(0,1)/100.0f)<0.0f){
			smoke=deletesmoke(state,smoke,prevsmoke);
			continue;
		}
		prevsmoke=smoke;
		smoke=smoke->next;
	}
	
	for(struct flare *flare=state->flarelist,*prevflare=NULL;flare!=NULL;){
		if(flare->xv>0.0f&&flare->base.y>state->rect.bottom){
			flare=deleteflare(state,flare,prevflare);
			continue;
		}
		flare->base.x+=flare->xv;
		if(flare->base.y+flare->base.h<LAVA_Y){
			flare->base.y+=flare->yv;
			if((flare->yv+=0.005f)>0.3f)flare->yv=0.3f;
		}
		else flare->base.y+=flare->yv>0.0f?0.05f:-0.05f;
		flare->base.rot+=flare->xv*1.5f;
		if(flare->base.rot>PI2)flare->base.rot=0.0f;
		else if(flare->base.rot<0.0f)flare->base.rot=PI2;
		if(++flare->frame>60)flare->frame=0;
		int side;
		int stop=false;
		if(onein(4))newsmoke(state,&flare->base,COLOR_RED);
		for(int i=0;i<BLOCK_COUNT;++i){
			if(state->block[i].hidden)continue;
			if(side=correct(&flare->base,&state->block[i].base)){
				switch(side){
					case COLLIDE_TOP:
						newparticle(state,flare->base.x+(FLARE_SIZE/2.0f),state->block[i].base.y,30,COLOR_RED);
						flare=deleteflare(state,flare,prevflare);
						stop=true;
						break;
					case COLLIDE_LEFT:
						flare->base.x=state->block[i].base.x-FLARE_SIZE;
						flare->xv=-flare->xv;
						break;
					case COLLIDE_RIGHT:
						flare->base.x=state->block[i].base.x+state->block[i].base.w;
						flare->xv=-flare->xv;
						break;
				}
				break;
			}
		}
		if(stop)continue;
		prevflare=flare;
		flare=flare->next;
	}
	
	for(struct silo *silo=state->silolist;silo!=NULL;silo=silo->next){
		if(fabs((silo->base.x+(SILO_WIDTH/2.0f))-(state->player.base.x+(PLAYER_WIDTH/2.0f)))<SILO_RANGE&&!silo->missile){
			newmissile(state,silo);
		}
	}
	
	for(struct missile *missile=state->missilelist,*prevmissile=NULL;missile!=NULL;){
		if(missile->dead){
			missile=deletemissile(state,missile,prevmissile);
			continue;
		}
		struct base temp=missile->base;
		temp.x-=missile->xv*5.0f;
		temp.w=MISSILE_WIDTH/8.0f;
		if(!onein(7))newsmoke(state,&missile->base,COLOR_BLACK);
		--missile->ttl;
		missile->base.x+=missile->xv;
		missile->base.y+=missile->yv;
		
		int stop=false;
		if(missile->ttl<MISSILE_TTL-60&&missile->ttl>0){
			if(collide(&missile->base,&state->player.base)){
				newparticle(state,missile->base.x+(MISSILE_WIDTH/2.0f),missile->base.y+(MISSILE_HEIGHT/2.0f),30,COLOR_BLACK);
				missile=deletemissile(state,missile,prevmissile);
				continue;
			}
			for(int i=0;i<BLOCK_COUNT;++i){
				if(collide(&state->block[i].base,&missile->base)&&!state->block[i].hidden){
					newparticle(state,missile->base.x+(MISSILE_WIDTH/2.0f),state->block[i].base.y,30,COLOR_BLACK);
					missile=deletemissile(state,missile,prevmissile);
					stop=true;
					break;
				}
			}
			if(stop)continue;
			float angle=-atan2f((missile->base.y+(MISSILE_HEIGHT/2.0f))-(state->player.base.y+(PLAYER_HEIGHT/1.3f)),
			(missile->base.x+(MISSILE_WIDTH/2.0f))-(state->player.base.x+(PLAYER_WIDTH/2.0f)));
			align(&missile->base.rot,0.04,-angle);
			missile->xv=-cosf(missile->base.rot)*MISSILE_SPEED;
			missile->yv=-sinf(missile->base.rot)*MISSILE_SPEED;
		}
		else if(missile->ttl<1){
			missile->yv+=GRAVITY;
			for(int i=0;i<BLOCK_COUNT;++i){
				if(collide(&state->block[i].base,&missile->base)&&!state->block[i].hidden){
					newparticle(state,missile->base.x+(MISSILE_WIDTH/2.0f),state->block[i].base.y,30,COLOR_BLACK);
					missile=deletemissile(state,missile,prevmissile);
					stop=true;
					break;
				}
			}
			if(stop)continue;
		}
		
		for(struct missile *missile2=state->missilelist,*prevmissile2=NULL;missile2!=NULL;){
			if(missile==missile2){
				prevmissile2=missile2;
				missile2=missile2->next;
				continue;
			}
			if(collide(&missile->base,&missile2->base)){
				newparticle(state,missile->base.x+(MISSILE_WIDTH/2.0f),missile->base.y+(MISSILE_HEIGHT/2.0f),30,COLOR_BLACK);
				newparticle(state,missile2->base.x+(MISSILE_WIDTH/2.0f),missile2->base.y+(MISSILE_HEIGHT/2.0f),30,COLOR_BLACK);
				missile->dead=true;
				missile2->dead=true;
				stop=true;
				break;
			}
			prevmissile2=missile2;
			missile2=missile2->next;
		}
		if(stop)continue;
		
		prevmissile=missile;
		missile=missile->next;
	}
	
	int cloudcount=0;
	for(struct cloud *cloud=state->cloudlist,*prevcloud=NULL;cloud!=NULL;){
		++cloudcount;
		cloud->base.x+=cloud->xv;
		if(cloud->base.x<CLOUD_BOUND){
			cloud=deletecloud(state,cloud,prevcloud);
			continue;
		}
		prevcloud=cloud;
		cloud=cloud->next;
	}
	if(onein(200)&&cloudcount<4)newcloud(state);
	
	for(int i=0;i<BLOCK_COUNT;++i){
		if(state->block[i].hidden&&onein(200))newflare(state,i);
	}
	
	if(state->populate){
		state->populate=false;
		int enemycount=0;
		int silocount=0;
		do{
			for(int i=5;i<BLOCK_COUNT-1;++i){
				if(onein(4)&&!state->block[i].hidden&&enemycount<ENEMY_COUNT){
					newenemy(state,i);
					++enemycount;
				}
				if(onein(8)&&!state->block[i].hidden&&silocount<SILO_COUNT){
					newsilo(state,i);
					++silocount;
				}
			}
		}while(silocount<SILO_COUNT||enemycount<ENEMY_COUNT);
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
	if((state->pbuttonstate=buttonprocess(state,&state->pbutton))==BUTTON_ACTIVATE){
		if(!menu_pause(state))return false;
		if(state->showmenu){
			reset(state);
			return core(state);
		}
	}
	return true;
}

void render(struct state *state){
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
	uidraw(state,&state->background,0);
	
	if(state->cloudlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_CLOUD].object);
		for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
			draw(state,&cloud->base,0,cloud->xinvert);
	}
	
	if(state->particlelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLOCK].object);
		for(struct particle *particle=state->particlelist;particle!=NULL;particle=particle->next){
			if(particle->color==COLOR_BLACK)glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
			else glUniform4f(state->uniform.rgba,1.0f,0.416f,0.0f,1.0f);
			draw(state,&particle->base,0,false);
		}
	}
	
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
	if(state->flarelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_FLARE].object);
		for(struct flare *flare=state->flarelist;flare!=NULL;flare=flare->next)
			draw(state,&flare->base,flare->frame>30,false);
	}
	
	if(state->missilelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_MISSILE].object);
		for(struct missile *missile=state->missilelist;missile!=NULL;missile=missile->next)
			if(missile->base.y+MISSILE_HEIGHT>state->rect.top)draw(state,&missile->base,0.0f,false);
			else{
				struct base temp={missile->base.x,state->rect.top,MISSILE_WIDTH,MISSILE_HEIGHT,0.0f,2.0f};
				draw(state,&temp,1.0f,false);
			}
	}
	
	if(state->silolist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SILO].object);
		for(struct silo *silo=state->silolist;silo!=NULL;silo=silo->next)
			draw(state,&silo->base,0.0f,false);
	}
	
	if(state->enemylist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_ENEMY].object);
		for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
			draw(state,&enemy->base,0.0f,enemy->xv>0.0f?true:false);
	}
	
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
	draw(state,&state->player.base,state->player.frame,state->player.xinvert);
	
	if(state->smokelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLOCK].object);
		for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=smoke->next){
			if(!state->particlelist)glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLOCK].object);
			if(smoke->color==COLOR_BLACK)glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,smoke->alpha);
			else glUniform4f(state->uniform.rgba,1.0f,0.416f,0.0f,smoke->alpha);
			draw(state,&smoke->base,0,false);
		}
	}
	
	if(state->shockwavelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SHOCKWAVE].object);
		for(struct shockwave *shockwave=state->shockwavelist;shockwave!=NULL;shockwave=shockwave->next){
			if(shockwave->color==COLOR_BLACK)glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,shockwave->alpha);
			else glUniform4f(state->uniform.rgba,1.0f,0.416f,0.0f,shockwave->alpha);
			draw(state,&shockwave->base,0,false);
		}
	}
	
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	if(state->blastlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BLAST].object);
		for(struct blast *blast=state->blastlist;blast!=NULL;blast=blast->next)
			draw(state,&blast->base,blast->frame,blast->xv>0.0f?false:true);
	}
	
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
	uidraw(state,&state->lbutton,state->lbuttonstate);
	uidraw(state,&state->rbutton,state->rbuttonstate);
	uidraw(state,&state->jbutton,state->jbuttonstate);
	uidraw(state,&state->fbutton,state->fbuttonstate);
	buttondraw(state,&state->pbutton);
	
	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	int bound=false;
	for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next){
		if(enemy->text.timer){
			if(!bound){
				glBindTexture(GL_TEXTURE_2D,state->font.dialog->atlas);
				bound=true;
			}
			drawtextcentered(state->font.dialog,enemy->base.x-(state->player.base.x+2.5f),enemy->base.y-1.0f,enemy->text.phrase);
		}
	}
	
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_SYMBOL].object);
	uidraw(state,&(struct base){state->lbutton.x+(BUTTON_WIDTH/2.0f)-0.5f,state->lbutton.y+(BUTTON_WIDTH/2.0f)-0.5f+(state->lbuttonstate?0.1f:0.0f),1.0f,1.0f,0.0f,5.0f},0);
	uidraw(state,&(struct base){state->rbutton.x+(BUTTON_WIDTH/2.0f)-0.5f,state->rbutton.y+(BUTTON_WIDTH/2.0f)-0.5f+(state->rbuttonstate?0.1f:0.0f),1.0f,1.0f,0.0f,5.0f},1);
	uidraw(state,&(struct base){state->jbutton.x+(BUTTON_WIDTH/2.0f)-0.5f,state->jbutton.y+(BUTTON_WIDTH/2.0f)-0.5f+(state->jbuttonstate?0.1f:0.0f),1.0f,1.0f,0.0f,5.0f},2);
	uidraw(state,&(struct base){state->fbutton.x+(BUTTON_WIDTH/2.0f)-0.5f,state->fbutton.y+(BUTTON_WIDTH/2.0f)-0.5f+(state->fbuttonstate?0.1f:0.0f),1.0f,1.0f,0.0f,5.0f},3);
	uidraw(state,&(struct base){state->pbutton.base.x+(BUTTON_WIDTH/2.0f)-0.5f,state->pbutton.base.y+(BUTTON_WIDTH/2.0f)-0.5f+(state->pbuttonstate?0.1f:0.0f),1.0f,1.0f,0.0f,5.0f},4);
	
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
		drawtext(state->font.main,state->rect.left+3.0f,state->rect.top+0.1f,fpsstring);
	}
}

void init(struct state *state){
	if(!readconf(state)){
		state->musicenabled=true;
		state->soundenabled=true;
		state->vibenabled=true;
		state->showtut=true;
	}
	else state->showtut=false;
	state->showmenu=true;
	state->back=false;
	memset(state->pointer,0,sizeof(struct crosshair)*2);
	state->rect.left=-(state->rect.right=8.0f);
	state->rect.top=-(state->rect.bottom=4.5f);
	state->background=(struct base){state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f};
	state->lbutton=(struct base){-7.5f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f};
	state->rbutton=(struct base){-5.25f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f};
	state->jbutton=(struct base){5.75f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f};
	state->fbutton=(struct base){5.75f,-0.25f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f};
	state->pbutton=(struct button){{-7.5f,-4.3f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"",false};
	state->buttonframe=(struct base){state->rect.right-1.25f,state->rect.top,1.25f,state->rect.bottom*2.0f,0.0f,1.0f};
	state->lava=(struct base){state->rect.left,LAVA_Y,state->rect.right*2.0f,state->rect.bottom-LAVA_Y,0.0f,1.0f};
	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;
	state->player.base.count=8.0f;
	state->enemylist=NULL;
	state->blastlist=NULL;
	state->particlelist=NULL;
	state->shockwavelist=NULL;
	state->smokelist=NULL;
	state->flarelist=NULL;
	state->silolist=NULL;
	state->missilelist=NULL;
	state->cloudlist=NULL;
}
void reset(struct state *state){
	for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=deleteenemy(state,enemy,NULL));
	for(struct blast *blast=state->blastlist;blast!=NULL;blast=deleteblast(state,blast,NULL));
	for(struct particle *particle=state->particlelist;particle!=NULL;particle=deleteparticle(state,particle,NULL));
	for(struct shockwave *shockwave=state->shockwavelist;shockwave!=NULL;shockwave=deleteshockwave(state,shockwave,NULL));
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=deletesmoke(state,smoke,NULL));
	for(struct flare *flare=state->flarelist;flare!=NULL;flare=deleteflare(state,flare,NULL));
	for(struct silo *silo=state->silolist;silo!=NULL;silo=deletesilo(state,silo,NULL));
	for(struct missile *missile=state->missilelist;missile!=NULL;missile=deletemissile(state,missile,NULL));
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=deletecloud(state,cloud,NULL));
	newblocks(state);
	state->populate=true;
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
	state->player.text.timer=0;
}