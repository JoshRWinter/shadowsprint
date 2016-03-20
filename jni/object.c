#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

void newblocks(struct state *state){
	float offset=-1.5f,lasth,lasthidden;
	const float BLOCK_CEILING=6.2f;
	const float BLOCK_FLOOR=2.0f;
	const float BLOCK_SPACING=0.45f;
	for(int i=0;i<BLOCK_COUNT;++i){
		state->block[i].hidden=(lasthidden?false:onein(3))||i==0||i==BLOCK_COUNT-1;
		if(state->block[i].hidden){
			state->block[i].base.h=state->rect.bottom*2.0f;
			state->block[i].base.w=1.45f;
		}
		else{
			if(i==1)state->block[i].base.h=randomint(BLOCK_FLOOR*10.0f,BLOCK_CEILING*10.0f)/10.0f;
			else{
				do{
					state->block[i].base.h=lasth+(randomint(-100,100)/100.0f);
				}while((state->block[i].base.h>BLOCK_CEILING||state->block[i].base.h<BLOCK_FLOOR)||fabs(state->block[i].base.h-lasth)<BLOCK_SPACING);
			}
			state->block[i].base.w=randomint(275,450)/100.0f;
		}
		state->block[i].base.x=offset-0.05f;
		state->block[i].base.y=state->rect.bottom-state->block[i].base.h;
		state->block[i].base.rot=0.0f;
		offset=state->block[i].base.x+state->block[i].base.w;
		if(!state->block[i].hidden)lasth=state->block[i].base.h;
		lasthidden=state->block[i].hidden;
	}
}

void newenemy(struct state *state,int index){
	struct enemy *enemy=malloc(sizeof(struct enemy));
	enemy->base.w=ENEMY_WIDTH;
	enemy->base.h=ENEMY_HEIGHT;
	enemy->base.x=state->block[index].base.x;
	enemy->base.y=state->block[index].base.y-ENEMY_HEIGHT;
	enemy->base.rot=0.0f;
	enemy->base.count=1.0f;
	enemy->xv=onein(2)?ENEMY_SPEED:-ENEMY_SPEED;
	enemy->yv=0.0f;
	enemy->attack=false;
	enemy->text.timer=0;
	enemy->next=state->enemylist;
	state->enemylist=enemy;
}
struct enemy *deleteenemy(struct state *state,struct enemy *enemy,struct enemy *prev){
	if(prev!=NULL)prev->next=enemy->next;
	else state->enemylist=enemy->next;
	void *temp=enemy->next;
	free(enemy);
	return temp;
}

void newblast(struct state *state){
	state->player.reload=PLAYER_RELOAD;
	struct blast *blast=malloc(sizeof(struct blast));
	blast->base.w=BLAST_WIDTH;
	blast->base.h=BLAST_HEIGHT;
	blast->base.x=state->player.base.x+(PLAYER_WIDTH/2.0f)-(BLAST_WIDTH/2.0f);
	blast->base.y=state->player.base.y+(PLAYER_HEIGHT/2.0f)-(BLAST_HEIGHT/2.0f);
	blast->base.rot=0.0f;
	blast->base.count=4.0f;
	blast->ttl=70;
	blast->frame=0;
	blast->frametimer=0;
	blast->xv=state->player.xinvert?-BLAST_SPEED:BLAST_SPEED;
	blast->next=state->blastlist;
	state->blastlist=blast;
}
struct blast *deleteblast(struct state *state,struct blast *blast,struct blast *prev){
	if(prev!=NULL)prev->next=blast->next;
	else state->blastlist=blast->next;
	void *temp=blast->next;
	free(blast);
	return temp;
}

void newparticle(struct state *state,float x,float y,int count,int color){
	const float PARTICLE_SPEED=3.8f;
	newshockwave(state,x,y,color);
	for(int i=0;i<count;++i){
		float angle=torad(randomint(1,360));
		struct particle *particle=malloc(sizeof(struct particle));
		particle->base.w=PARTICLE_SIZE;
		particle->base.h=PARTICLE_SIZE;
		particle->base.x=x-(PARTICLE_SIZE/2.0f);
		particle->base.y=y-(PARTICLE_SIZE/2.0f);
		particle->base.rot=torad(randomint(1,360));
		particle->base.count=1.0f;
		particle->xv=cosf(angle)/PARTICLE_SPEED;
		particle->yv=sinf(angle)/PARTICLE_SPEED;
		particle->color=color;
		particle->ttl=randomint(120,145);
		particle->next=state->particlelist;
		state->particlelist=particle;
	}
}
struct particle *deleteparticle(struct state *state,struct particle *particle,struct particle *prev){
	if(prev!=NULL)prev->next=particle->next;
	else state->particlelist=particle->next;
	void *temp=particle->next;
	free(particle);
	return temp;
}

void newshockwave(struct state *state,float x,float y,int color){
	struct shockwave *shockwave=malloc(sizeof(struct shockwave));
	shockwave->base.w=0.0f;
	shockwave->base.h=0.0f;
	shockwave->base.x=x;
	shockwave->base.y=y;
	shockwave->base.rot=0.0f;
	shockwave->base.count=1.0f;
	shockwave->color=color;
	shockwave->alpha=1.0f;
	shockwave->next=state->shockwavelist;
	state->shockwavelist=shockwave;
}
struct shockwave *deleteshockwave(struct state *state,struct shockwave *shockwave,struct shockwave *prev){
	if(prev!=NULL)prev->next=shockwave->next;
	else state->shockwavelist=shockwave->next;
	void *temp=shockwave->next;
	free(shockwave);
	return temp;
}

void newsmoke(struct state *state,struct base *base,int color){
	if(!onein(5))return;
	struct smoke *smoke=malloc(sizeof(struct smoke));
	smoke->base.w=SMOKE_SIZE;
	smoke->base.h=SMOKE_SIZE;
	smoke->base.x=base->x+(base->w/2.0f)+(randomint(-4,4)/50.0f);
	smoke->base.y=base->y+(base->h/2.0f)+(randomint(-4,4)/50.0f);
	smoke->base.rot=0.0f;
	smoke->base.count=1.0f;
	smoke->color=color;
	smoke->alpha=randomint(60,80)/100.0f;
	smoke->next=state->smokelist;
	state->smokelist=smoke;
}
struct smoke *deletesmoke(struct state *state,struct smoke *smoke,struct smoke *prev){
	if(prev!=NULL)prev->next=smoke->next;
	else state->smokelist=smoke->next;
	void *temp=smoke->next;
	free(smoke);
	return temp;
}

void newflare(struct state *state,int index){
	struct flare *flare=malloc(sizeof(struct flare));
	flare->base.w=FLARE_SIZE;
	flare->base.h=FLARE_SIZE;
	flare->base.x=randomint(state->block[index].base.x*10.0f,(state->block[index].base.x+state->block[index].base.w-FLARE_SIZE)*10.0f)/10.0f;
	flare->base.y=state->rect.bottom;
	flare->base.rot=torad(randomint(1,360));
	flare->base.count=2.0f;
	flare->xv=randomint(-3,3)/100.0f;
	flare->yv=-0.225f;
	flare->frame=0;
	flare->next=state->flarelist;
	state->flarelist=flare;
}
struct flare *deleteflare(struct state *state,struct flare *flare,struct flare *prev){
	if(prev!=NULL)prev->next=flare->next;
	else state->flarelist=flare->next;
	void *temp=flare->next;
	free(flare);
	return temp;
}

void newsilo(struct state *state,int index){
	struct silo *silo=malloc(sizeof(struct silo));
	silo->base.w=SILO_WIDTH;
	silo->base.h=SILO_HEIGHT;
	silo->base.x=randomint(state->block[index].base.x*100.0f,(state->block[index].base.x+state->block[index].base.w-SILO_WIDTH)*100.0f)/100.0f;
	silo->base.y=state->block[index].base.y-SILO_HEIGHT;
	silo->base.rot=0.0f;
	silo->base.count=1.0f;
	silo->health=100;
	silo->missile=NULL;
	silo->next=state->silolist;
	state->silolist=silo;
}
struct silo *deletesilo(struct state *state,struct silo *silo,struct silo *prev){
	if(silo->missile)silo->missile->silo=NULL;
	if(prev!=NULL)prev->next=silo->next;
	else state->silolist=silo->next;
	void *temp=silo->next;
	free(silo);
	return temp;
}

void newmissile(struct state *state,struct silo *silo){
	struct missile *missile=malloc(sizeof(struct missile));
	missile->base.w=MISSILE_WIDTH;
	missile->base.h=MISSILE_HEIGHT;
	missile->base.x=silo->base.x+(SILO_WIDTH/2.0f)-(MISSILE_WIDTH/2.0f);
	missile->base.y=silo->base.y+SILO_HEIGHT;
	missile->base.rot=PI/2.0f;
	missile->base.count=2.0f;
	missile->xv=0.0f;
	missile->yv=-0.05f;
	missile->ttl=MISSILE_TTL;
	missile->silo=silo;
	silo->missile=missile;
	missile->dead=false;
	missile->next=state->missilelist;
	state->missilelist=missile;
}
struct missile *deletemissile(struct state *state,struct missile *missile,struct missile *prev){
	if(missile->silo)missile->silo->missile=NULL;
	if(prev!=NULL)prev->next=missile->next;
	else state->missilelist=missile->next;
	void *temp=missile->next;
	free(missile);
	return temp;
}

void newdust(struct state *state){
	struct dust *dust=malloc(sizeof(struct dust));
	dust->base.w=randomint(5,90)/100.0f;
	dust->base.h=dust->base.w;
	dust->base.x=randomint((state->player.base.x+state->rect.left)*10.0f,(state->player.base.x+state->rect.right+15.0f)*10.0f)/10.0f;
	dust->base.y=state->rect.top-dust->base.h;
	dust->base.rot=torad(randomint(1,360));
	dust->base.count=5.0f;
	dust->xv-=0.1f*dust->base.w;
	dust->yv=-dust->xv;
	dust->rotv=randomint(-30,30)/1000.0f;
	dust->sprite=randomint(0,4);
	dust->xflip=onein(2);
	dust->next=state->dustlist;
	state->dustlist=dust;
	if(state->dustlist->next==NULL){
		for(int i=0;i<400;++i)dustroutine(state);
	}
}
void dustroutine(struct state *state){
	if(onein(4)||onein(5)||!state->dustlist)newdust(state);
	for(struct dust *dust=state->dustlist,*prevdust=NULL;dust!=NULL;){
		dust->base.x+=dust->xv;
		dust->base.y+=dust->yv;
		dust->base.rot+=dust->rotv;
		if(dust->base.x+dust->base.w<state->rect.left||dust->base.y>state->rect.bottom){
			dust=deletedust(state,dust,prevdust);
			continue;
		}
		prevdust=dust;
		dust=dust->next;
	}
}
void dustrender(struct state *state){
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_DUST].object);
	for(struct dust *dust=state->dustlist;dust!=NULL;dust=dust->next){
		draw(state,&dust->base,dust->sprite,dust->xflip);
	}
}
struct dust *deletedust(struct state *state,struct dust *dust,struct dust *prev){
	if(prev!=NULL)prev->next=dust->next;
	else state->dustlist=dust->next;
	void *temp=dust->next;
	free(dust);
	return temp;
}

void newcloud(struct state *state){
	struct cloud *cloud=malloc(sizeof(struct cloud));
	cloud->base.w=CLOUD_WIDTH;
	cloud->base.h=CLOUD_HEIGHT;
	cloud->base.y=randomint(state->rect.top*10.0f,0.0f)/10.0f;
	cloud->base.x=randomint((state->player.base.x+state->rect.right+2.0f)*10.0f,(state->player.base.x+state->rect.right+7.0f)*10.0f)/10.0f;
	cloud->base.rot=0.0f;
	cloud->base.count=1.0f;
	cloud->xinvert=onein(2);
	cloud->xv=-randomint(20,40)/1000.0f;
	cloud->next=state->cloudlist;
	state->cloudlist=cloud;
}
struct cloud *deletecloud(struct state *state,struct cloud *cloud,struct cloud *prev){
	if(prev!=NULL)prev->next=cloud->next;
	else state->cloudlist=cloud->next;
	void *temp=cloud->next;
	free(cloud);
	return temp;
}