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

void newblast(struct state *state){
	state->player.reload=PLAYER_RELOAD;
	struct blast *blast=malloc(sizeof(struct blast));
	blast->base.w=BLAST_WIDTH;
	blast->base.h=BLAST_HEIGHT;
	blast->base.x=state->player.xinvert?state->player.base.x-BLAST_WIDTH:state->player.base.x+PLAYER_WIDTH;
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

void newsmoke(struct state *state,struct base *base){
	if(!onein(5))return;
	struct smoke *smoke=malloc(sizeof(struct smoke));
	smoke->base.w=SMOKE_SIZE;
	smoke->base.h=SMOKE_SIZE;
	smoke->base.x=base->x;
	smoke->base.y=base->y+(randomint(0.0f,base->h*10.0f)/10.0f);
	smoke->base.rot=0.0f;
	smoke->base.count=1.0f;
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