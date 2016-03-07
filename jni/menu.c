#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include "defs.h"

int menu_main(struct state *state){
	//vibratedevice(&state->jni_info,30);
	const char *aboottext=
	"Shadow Sprint\nProgrammed by Josh Winter\n\nFonts:\nCorbel\nBAUHAUS93";
	const float bleftoffset=5.8f;
	const float btopoffset=-4.175f;
	struct button playbutton={{bleftoffset,btopoffset,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Play",false};
	struct button abootbutton={{bleftoffset,btopoffset+((BUTTON_HEIGHT+0.1f)),BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Aboot",false};
	struct button configbutton={{bleftoffset,btopoffset+((BUTTON_HEIGHT+0.1f)*2.0f),BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Settings",false};
	struct button quitbutton={{bleftoffset,btopoffset+((BUTTON_HEIGHT+0.1f)*3.0f),BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Quit",false};
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
		uidraw(state,&state->background,0);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONFRAME].object);
		uidraw(state,&state->buttonframe,0);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&playbutton)==BUTTON_ACTIVATE){
			return true;
		}
		if(buttondraw(state,&abootbutton)==BUTTON_ACTIVATE){
			if(!menu_message(state,"Aboot",aboottext,NULL))return false;
			continue;
		}
		if(buttondraw(state,&configbutton)==BUTTON_ACTIVATE){
			//config
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
		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

int menu_pause(struct state *state){
	return true;
}

int menu_message(struct state *state,const char *caption,const char *msg,int *yesno){
	struct button okbutton={{5.8f,-BUTTON_HEIGHT/2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Mmkay",false};
	struct button yesbutton={{5.8f,-2.0,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"Yes",false};
	struct button nobutton={{4.5f,0.25f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,2.0f},"No",false};
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BACKGROUND].object);
		uidraw(state,&state->background,0);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONFRAME].object);
		uidraw(state,&state->buttonframe,0);
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