#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include "defs.h"

int menu_main(struct state *state){
	//vibratedevice(&state->jni_info,30);
	const char *aboottext=
	"-- Shadow Sprint --\nProgrammed by Josh Winter\n-- Fonts --\nCorbel, BAUHAUS93\n-- Music --\nCoffee-Break - Legionella\n"
	"Coffee-Break.newgrounds.com";
	const float bleftoffset=5.8f;
	const float btopoffset=-4.175f;
	struct button playbutton={{bleftoffset,btopoffset,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Play",false};
	struct button abootbutton={{bleftoffset,btopoffset+((BUTTON_HEIGHT+0.1f)),BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Aboot",false};
	struct button configbutton={{bleftoffset,btopoffset+((BUTTON_HEIGHT+0.1f)*2.0f),BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Settings",false};
	struct button quitbutton={{bleftoffset,btopoffset+((BUTTON_HEIGHT+0.1f)*3.0f),BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Quit",false};
	state->showmenu=false;
	int play=false;
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
		uidraw(state,&state->background,0);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONFRAME].object);
		uidraw(state,&state->buttonframe,0);
		dustroutine(state);
		dustrender(state);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&playbutton)==BUTTON_ACTIVATE){
			state->enablewhiteout=true;
			play=true;
		}
		if(buttondraw(state,&abootbutton)==BUTTON_ACTIVATE){
			if(!menu_message(state,"Aboot",aboottext,NULL))return false;
			continue;
		}
		if(buttondraw(state,&configbutton)==BUTTON_ACTIVATE){
			if(!menu_conf(state))return false;
			continue;
		}
		if(buttondraw(state,&quitbutton)==BUTTON_ACTIVATE||state->back){
			ANativeActivity_finish(state->app->activity);
		}
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		buttondrawtext(state->font.main,&playbutton);
		buttondrawtext(state->font.main,&abootbutton);
		buttondrawtext(state->font.main,&configbutton);
		buttondrawtext(state->font.main,&quitbutton);
		drawtextcentered(state->font.main,-2.0f,-2.0f,"");
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,-2.0f,-3.0f,"SHADOW SPRINT");
		if(state->enablewhiteout||state->whiteout>0.0f)whiteout(state);
		eglSwapBuffers(state->display,state->surface);
		if(!state->enablewhiteout&&play)return true;
	}
	return false;
}

int menu_pause(struct state *state){
	struct button backbutton={{5.8f,-BUTTON_HEIGHT/2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Resume",false};
	struct button resetbutton={{-7.0f,-1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Reset",false};
	struct button tutorialbutton={{-5.0f,-1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Tut.",false};
	struct button mainmenubutton={{-1.0f,-1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Menu",false};
	struct button quitbutton={{1.0f,-1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Quit",false};
	struct button confmenubutton={{-3.0f,-1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Settings",false};
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
		uidraw(state,&state->background,0.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONFRAME].object);
		uidraw(state,&state->buttonframe,0.0f);
		dustroutine(state);
		dustrender(state);
		
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&resetbutton)==BUTTON_ACTIVATE){
			reset(state);
			return true;
		}
		if(buttondraw(state,&tutorialbutton)==BUTTON_ACTIVATE){
			// tutorial
		}
		if(buttondraw(state,&mainmenubutton)==BUTTON_ACTIVATE){
			state->showmenu=true;
			return true;
		}
		if(buttondraw(state,&confmenubutton)==BUTTON_ACTIVATE){
			if(!menu_conf(state))return false;
		}
		if(buttondraw(state,&quitbutton)==BUTTON_ACTIVATE){
			ANativeActivity_finish(state->app->activity);
		}
		if(buttondraw(state,&backbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		buttondrawtext(state->font.main,&resetbutton);
		buttondrawtext(state->font.main,&tutorialbutton);
		buttondrawtext(state->font.main,&mainmenubutton);
		buttondrawtext(state->font.main,&confmenubutton);
		buttondrawtext(state->font.main,&quitbutton);
		buttondrawtext(state->font.main,&backbutton);
		
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,-2.0f,-3.0,"Paused");
		
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_conf(struct state *state){
	struct button musicbutton={{-6.0f,1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Music",false};
	struct button soundsbutton={{-4.0f,1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Sound",false};
	struct button vibbutton={{-2.0f,1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Vib.",false};
	struct button showtutbutton={{0.0f,1.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Tut.",false};
	struct button backbutton={{5.8f,-BUTTON_HEIGHT/2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Back",false};
	int changed=false;
	char settings[121];
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
		uidraw(state,&state->background,0.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONFRAME].object);
		uidraw(state,&state->buttonframe,0.0f);
		dustroutine(state);
		dustrender(state);
		
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&musicbutton)==BUTTON_ACTIVATE){
			if(state->musicenabled=!state->musicenabled){
				playsound(state->soundengine,state->aassets.sound+SID_THEME,true);
			}
			else{
				stopallsounds(state->soundengine);
			}
			changed=true;
		}
		if(buttondraw(state,&soundsbutton)==BUTTON_ACTIVATE){
			state->soundenabled=!state->soundenabled;
			changed=true;
		}
		if(buttondraw(state,&vibbutton)==BUTTON_ACTIVATE){
			state->vibenabled=!state->vibenabled;
			changed=true;
		}
		if(buttondraw(state,&showtutbutton)==BUTTON_ACTIVATE){
			state->showtut=!state->showtut;
		}
		if(buttondraw(state,&backbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			if(changed)saveconf(state);
			return true;
		}
		
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		buttondrawtext(state->font.main,&musicbutton);
		buttondrawtext(state->font.main,&soundsbutton);
		buttondrawtext(state->font.main,&vibbutton);
		buttondrawtext(state->font.main,&showtutbutton);
		buttondrawtext(state->font.main,&backbutton);
		
		sprintf(settings,"Music is %s\nSounds are %s\nVibration is %s\nShow Tutorial is %s",
		state->musicenabled?"enabled":"disabled",
		state->soundenabled?"enabled":"disabled",
		state->vibenabled?"enabled":"disabled",
		state->showtut?"enabled":"disabled");
		drawtextcentered(state->font.main,-2.0f,-1.6f,settings);
		
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,-2.0f,-3.0f,"Configuration");
		
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_gameover(struct state *state){
	struct base base={state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f};
	int timer=70;
	int active=false;
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_GAMEOVER].object);
		uidraw(state,&base,0.0f);
		if(timer)--timer;
		else{
			glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
			drawtext(state->font.main,state->rect.left+0.2f,state->rect.bottom-0.2f-state->font.main->fontsize,"Tap anywhere to continue...");
			if((active&&!state->pointer[0].active)||state->back){
				state->back=false;
				return true;
			}
			active=state->pointer[0].active;
		}
		whiteout(state);
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_victory(struct state *state){
	struct base base={state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f};
	int timer=70;
	int active=false;
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_VICTORY].object);
		uidraw(state,&base,0.0f);
		if(timer)--timer;
		else{
			glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
			drawtext(state->font.main,state->rect.left+0.2f,state->rect.bottom-0.2f-state->font.main->fontsize,"Tap anywhere to continue...");
			if((active&&!state->pointer[0].active)||state->back){
				state->back=false;
				return true;
			}
			active=state->pointer[0].active;
		}
		whiteout(state);
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_message(struct state *state,const char *caption,const char *msg,int *yesno){
	struct button okbutton={{5.8f,-BUTTON_HEIGHT/2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Mmkay",false};
	struct button yesbutton={{5.8f,-2.0,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Yes",false};
	struct button nobutton={{4.5f,0.25f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"No",false};
	while(process(state->app)){
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
		uidraw(state,&state->background,0);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONFRAME].object);
		uidraw(state,&state->buttonframe,0);
		dustroutine(state);
		dustrender(state);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(yesno){
			if(buttondraw(state,&yesbutton)==BUTTON_ACTIVATE){
				*yesno=true;
				return true;
			}
			if(buttondraw(state,&nobutton)==BUTTON_ACTIVATE||state->back){
				state->back=false;
				*yesno=false;
				return true;
			}
		}
		else if(buttondraw(state,&okbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		if(yesno){
			buttondrawtext(state->font.main,&yesbutton);
			buttondrawtext(state->font.main,&nobutton);
		}
		else buttondrawtext(state->font.main,&okbutton);
		drawtextcentered(state->font.main,-2.0f,-2.75f,msg);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		drawtextcentered(state->font.header,-2.0f,-4.0f,caption);
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}