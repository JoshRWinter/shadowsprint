#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <time.h>
#include "defs.h"

extern const char *vertexshader,*fragmentshader;
void init_display(struct state *state){
	state->running=true;
	initextensions();
	getdims(&state->device,state->app->window,DIMS_LAND);
	struct device screenres=state->device;
	if(screenres.w>1920)screenres.w=1920;
	if(screenres.h>1080)screenres.h=1080;
	state->display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(state->display,NULL,NULL);
	EGLConfig config;
	int configcount;
	eglChooseConfig(state->display,(int[]){EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE},&config,1,&configcount);
	ANativeWindow_setBuffersGeometry(state->app->window,screenres.w,screenres.h,0);
	state->surface=eglCreateWindowSurface(state->display,config,state->app->window,NULL);
	state->context=eglCreateContext(state->display,config,NULL,(int[]){EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE});
	eglMakeCurrent(state->display,state->surface,state->surface,state->context);
	if(!loadpack(&state->assets,state->app->activity->assetManager,"assets",TEX_MODE))logcat("texture init error");
	if(!loadpack(&state->uiassets,state->app->activity->assetManager,"uiassets",NULL))logcat("ui texture init error");
	state->program=initshaders(vertexshader,fragmentshader);
	glUseProgram(state->program);
	state->uniform.vector=glGetUniformLocation(state->program,"vector");
	state->uniform.size=glGetUniformLocation(state->program,"size");
	state->uniform.texcoords=glGetUniformLocation(state->program,"texcoords");
	state->uniform.rot=glGetUniformLocation(state->program,"rot");
	state->uniform.rgba=glGetUniformLocation(state->program,"rgba");
	state->uniform.projection=glGetUniformLocation(state->program,"projection");
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glUniform1f(state->uniform.rot,0.0f);
	float matrix[16];
	initortho(matrix,state->rect.left,state->rect.right,state->rect.bottom,state->rect.top,-1.0f,1.0f);
	glUniformMatrix4fv(state->uniform.projection,1,false,matrix);
	glGenVertexArrays(1,&state->vao);
	glGenBuffers(1,&state->vbo);
	glBindVertexArray(state->vao);
	glBindBuffer(GL_ARRAY_BUFFER,state->vbo);
	float verts[]={-0.5f,-0.5f,-0.5f,0.5f,0.5f,-0.5f,0.5f,0.5f};
	glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,verts,GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,2,GL_FLOAT,false,0,NULL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	if(state->level==1)glClearColor(COLOR_WHITE);
	else if(state->level==2)glClearColor(COLOR_NIGHT);
	else glClearColor(COLOR_MORNIN);
	set_ftfont_params(screenres.w,screenres.h,state->rect.right*2.0f,state->rect.bottom*2.0f,state->uniform.vector,state->uniform.size,state->uniform.texcoords);
	state->font.main=create_ftfont(state->app->activity->assetManager,0.425f,"corbel.ttf");
	state->font.dialog=create_ftfont(state->app->activity->assetManager,0.575f,"corbel.ttf");
	state->font.header=create_ftfont(state->app->activity->assetManager,0.9f,"BAUHS93.TTF");
	loadapack(&state->aassets,state->app->activity->assetManager,"aassets");
	state->soundengine=initOpenSL();
	if(state->musicenabled)playsound(state->soundengine,state->aassets.sound+SID_THEME,true);
	else playsound(state->soundengine,state->aassets.sound+SID_SILENCE,true);
}
void term_display(struct state *state){
	state->running=false;
	destroy_ftfont(state->font.main);
	destroy_ftfont(state->font.header);
	destroy_ftfont(state->font.dialog);
	destroypack(&state->assets);
	destroypack(&state->uiassets);
	glDeleteProgram(state->program);
	eglMakeCurrent(state->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
	eglDestroySurface(state->display,state->surface);
	eglDestroyContext(state->display,state->context);
	eglTerminate(state->display);
	termOpenSL(state->soundengine);
	destroyapack(&state->aassets);
}

int32_t inputproc(struct android_app *app,AInputEvent *event){
	struct state *state=app->userData;
	int32_t type=AInputEvent_getType(event);
	if(type==AINPUT_EVENT_TYPE_MOTION){
		state->gamepad=false;
		logcat("touchinput");
		return retrieve_touchscreen_input(event,state->pointer,state->device.w,state->device.h,state->rect.right*2.0f,state->rect.bottom*2.0f);
	}
	else if(type==AINPUT_EVENT_TYPE_KEY){
		int32_t key=AKeyEvent_getKeyCode(event);
		int32_t action=AKeyEvent_getAction(event);
		if(key==AKEYCODE_BACK&&action==AKEY_EVENT_ACTION_UP){
			state->back=true;
			return true;
		}
		else if(key==AKEYCODE_MENU&&action==AKEY_EVENT_ACTION_UP){
			reset(state);
			return true;
		}
		else if(key==AKEYCODE_DPAD_UP){
			state->gamepad=true;
			if(action==AKEY_EVENT_ACTION_DOWN)state->dpad_right=true;
			else if(action==AKEY_EVENT_ACTION_UP)state->dpad_right=false;
		}
		else if(key==AKEYCODE_DPAD_DOWN){
			state->gamepad=true;
			if(action==AKEY_EVENT_ACTION_DOWN)state->dpad_left=true;
			else if(action==AKEY_EVENT_ACTION_UP)state->dpad_left=false;
		}
		else if(key==AKEYCODE_DPAD_LEFT){
			state->gamepad=true;
			if(action==AKEY_EVENT_ACTION_DOWN)state->dpad_up=true;
			else if(action==AKEY_EVENT_ACTION_UP)state->dpad_up=false;
		}
		else if(key==AKEYCODE_DPAD_CENTER){
			state->gamepad=true;
			if(action==AKEY_EVENT_ACTION_DOWN)state->dpad_center=true;
			else if(action==AKEY_EVENT_ACTION_UP)state->dpad_center=false;
		}
	}
	return false;
}
void cmdproc(struct android_app *app,int32_t cmd){
	struct state *state=app->userData;
	switch(cmd){
		case APP_CMD_RESUME:
			hidenavbars(&state->jni_info);
			break;
		case APP_CMD_INIT_WINDOW:
			init_display(app->userData);
			break;
		case APP_CMD_TERM_WINDOW:
			term_display(app->userData);
			break;
		case APP_CMD_DESTROY:
			reset(app->userData);
			break;
	}
}

int process(struct android_app *app){
	int ident,events;
	struct android_poll_source *source;
	while((ident=ALooper_pollAll(((struct state*)app->userData)->running?0:-1,NULL,&events,(void**)&source))>=0){
		if(source)source->process(app,source);
		if(app->destroyRequested)return false;
	}
	return true;
}

void android_main(struct android_app *app){
	logcat("--- BEGIN NEW LOG ---");
	app_dummy();
	srand48(time(NULL));
	struct state state;
	state.running=false;
	state.app=app;
	app->userData=&state;
	app->onAppCmd=cmdproc;
	app->onInputEvent=inputproc;
	init(&state);
	reset(&state);
	init_jni(app,&state.jni_info);
	while(process(app)&&core(&state)){
		render(&state);
		eglSwapBuffers(state.display,state.surface);
	}
	term_jni(&state.jni_info);
	logcat("--- END LOG ---");
}
