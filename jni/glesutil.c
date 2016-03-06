#include <zlib.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <math.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <unistd.h>
#define GLESUTIL_DEBUG
#include "glesutil.h"

void initextensions(){
	glGenVertexArrays=(PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
	glBindVertexArray=(PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
	glDeleteVertexArrays=(PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
}

static int _loaddata(struct pack*,unsigned char*,const char*,int);
unsigned char *convert_power_of_two(unsigned char*,int*,int,int);
int loadpack(struct pack *target,AAssetManager *mgr,const char *filename,const char *mode){
	target->texture=NULL;
	AAsset *asset=AAssetManager_open(mgr,filename,AASSET_MODE_UNKNOWN);
	if(!asset)return false;
	int filesize=AAsset_seek(asset,0,SEEK_END);
	AAsset_seek(asset,0,SEEK_SET);
	Bytef *chunk=malloc(filesize);
	AAsset_read(asset,chunk,filesize);
	AAsset_close(asset);
	return _loaddata(target,chunk,mode,filesize);
}
static int _loaddata(struct pack *target,unsigned char *chunk,const char *mode,int filesize){
	uLongf size_of_uncompressed_data=*(uLongf*)&chunk[0];
	unsigned char *bytedata=malloc(size_of_uncompressed_data);
	int result=uncompress(bytedata,&size_of_uncompressed_data,&chunk[4],filesize-4);
	free(chunk);
	if(result!=Z_OK){
		free(bytedata);
		return false;
	}
	unsigned short *shortdata=(unsigned short*)&bytedata[0];// for reading the width and height
	target->count=shortdata[0];
	target->texture=malloc(sizeof(struct packtexture)*target->count);
	int shortindex=1,byteindex=2;
	for(int i=0;i<target->count;i++){
		int tempwidth=shortdata[shortindex];
		shortindex++;
		byteindex+=2;
		int tempheight=shortdata[shortindex];
		shortindex++;
		byteindex+=2;
		//unsigned char *data=convert_power_of_two(bytedata+byteindex,&target->texture[i].size,tempwidth,tempheight);
		unsigned char *tex=bytedata+byteindex; // pointer to texture in memory
		for(int i=0;i<tempwidth*tempheight*4;i+=4){
			unsigned char temp=tex[i];
			tex[i]=tex[i+2];
			tex[i+2]=temp;
		}
		byteindex+=tempwidth*tempheight*4;
		shortindex+=(tempwidth*tempheight*4)/2;
		/*if(data==NULL){
			free(target->texture);
			free(bytedata);
			return 0;
		}*/
		target->texture[i].pinch[1]=1.0f;//1.0f-((target->texture[i].size-tempwidth)/(float)target->texture[i].size);
		target->texture[i].pinch[2]=0.0f;//(target->texture[i].size-tempheight)/(float)target->texture[i].size;
		target->texture[i].pinch[3]=1.0f;
		target->texture[i].pinch[0]=0.0f;
		glGenTextures(1,&target->texture[i].object);
		glBindTexture(GL_TEXTURE_2D,target->texture[i].object);
		if(mode==NULL||mode[i]=='1'){
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		}
		else if(mode[i]=='0'){
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		}
		else return 0;
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tempwidth,tempheight,0,GL_RGBA,GL_UNSIGNED_BYTE,tex);
		//free(data);
	}
	free(bytedata);
	return true;
}
void destroypack(struct pack *target){
	for(int i=0;i<target->count;i++)
		glDeleteTextures(1,&target->texture[i].object);
	free(target->texture);
}

struct decoderdata{
	struct apack *target;
	AAssetManager *mgr;
	const char *filename;
	int started;
};
static int decodeogg(char *encoded,int filesize,short **decoded,unsigned *size,unsigned *targetsize,pthread_mutex_t mutex);
static void *_loadapack(void *v){
	struct decoderdata *dd=v;
	pthread_mutex_lock(&dd->target->mutex);
	dd->started=true;
	AAsset *asset=AAssetManager_open(dd->mgr,dd->filename,AASSET_MODE_UNKNOWN);
	if(!asset){
		logcat("unable to open file. exiting...");
		return NULL;
	}
	AAsset_read(asset,&dd->target->count,1);
	dd->target->sound=malloc(sizeof(struct apacksound)*dd->target->count);
	memset(dd->target->sound,0,sizeof(struct apacksound)*dd->target->count);
	for(int i=0;i<dd->target->count;i++)dd->target->sound[i].parent=dd->target;
	pthread_mutex_unlock(&dd->target->mutex);
	for(int i=0;i<dd->target->count;i++){
		unsigned filesize;
		AAsset_read(asset,&filesize,4);
		char *encoded=malloc(filesize);
		AAsset_read(asset,encoded,filesize);
		int success=decodeogg(encoded,filesize,&dd->target->sound[i].buffer,&dd->target->sound[i].size,&dd->target->sound[i].targetsize,dd->target->mutex);
		if(!success)logcat("file %d failed to decode",i);
		free(encoded);
	}
	AAsset_close(asset);
	free(v);
	logcat("decoding completed");
	return NULL;
}
void loadapack(struct apack *target,AAssetManager *mgr,const char *filename){
	struct decoderdata *dd=malloc(sizeof(struct decoderdata));
	dd->target=target;
	dd->mgr=mgr;
	dd->filename=filename;
	dd->started=false;
	pthread_mutex_init(&target->mutex,NULL);
	pthread_create(&target->thread,NULL,_loadapack,dd);
	int started=false;
	int count=0;
	while(!started){// wait for freshly spawned thread to actually start executing
		pthread_mutex_lock(&target->mutex);
		started=dd->started;
		pthread_mutex_unlock(&target->mutex);
		++count;
		usleep(10*1000);
	}
}
void destroyapack(struct apack *target){
	pthread_join(target->thread,NULL);
	for(int i=0;i<target->count;i++)free(target->sound[i].buffer);
	free(target->sound);
}

void getdims(struct device *device,ANativeWindow *window,int orientation){
	device->w=ANativeWindow_getWidth(window);
	device->h=ANativeWindow_getHeight(window);
	if(orientation==DIMS_LAND&&device->w<device ->h){
		int temp=device->w;
		device->w=device->h;
		device->h=temp;
		logcat("corrected device dim values. screw you android!!");
	}
	else if(orientation==DIMS_PORT&&device->h<device->w){
		int temp=device->w;
		device->w=device->h;
		device->h=temp;
		logcat("corrected device dim values. screw you android!!");
	}
}
int32_t retrieve_touchscreen_input(AInputEvent *event,struct crosshair *pointer,int iscreenwidth,int iscreenheight,float fscreenwidth,float fscreenheight){
	int32_t action=AMotionEvent_getAction(event);
	int pointerid=AMotionEvent_getPointerId(event,GET_POINTER_INDEX(action));
	if(pointerid>1)return true;
	switch(action&AMOTION_EVENT_ACTION_MASK){
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			pointer[pointerid].active=true;
		case AMOTION_EVENT_ACTION_MOVE:
			for(int i=0,pointercount=AMotionEvent_getPointerCount(event);i<pointercount;i++){
				pointerid=AMotionEvent_getPointerId(event,i);
				if(pointerid>1)return true;
				pointer[pointerid].x=((AMotionEvent_getX(event,i)/(float)iscreenwidth)*fscreenwidth)-(fscreenwidth/2.0f);
				pointer[pointerid].y=((AMotionEvent_getY(event,i)/(float)iscreenheight)*fscreenheight)-(fscreenheight/2.0f);
			}
			break;
		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
			pointer[pointerid].active=false;
			break;
		default: return false; break;
	}
	return true;
}

void initortho(float *matrix,float left,float right,float bottom,float top,float znear,float zfar){
	matrix[0]=2.0f/(right-left);
	matrix[1]=0.0f;
	matrix[2]=0.0f;
	matrix[3]=0.0f;
	matrix[4]=0.0f;
	matrix[5]=2.0f/(top-bottom);
	matrix[6]=0.0f;
	matrix[7]=0.0f;
	matrix[8]=0.0f;
	matrix[9]=0.0f;
	matrix[10]=-2.0f/(zfar-znear);
	matrix[11]=0.0f;
	matrix[12]=-((right+left)/(right-left));
	matrix[13]=-((top+bottom)/(top-bottom));
	matrix[14]=-((zfar+znear)/(zfar-znear));
	matrix[15]=1.0f;
}

unsigned initshaders(const char *Vertex,const char *Fragment){
	unsigned program,vshad=0,fshad=0;
	int status=0;
	vshad=glCreateShader(GL_VERTEX_SHADER);
	fshad=glCreateShader(GL_FRAGMENT_SHADER);
	program=glCreateProgram();
	glShaderSource(vshad,1,&Vertex,NULL);
	glShaderSource(fshad,1,&Fragment,NULL);
	glCompileShader(vshad);
	glCompileShader(fshad);
	glGetShaderiv(vshad,GL_COMPILE_STATUS,&status);
	char *buff=malloc(1000);
	if(!status){
		glGetShaderInfoLog(vshad,1000,NULL,buff);
		logcat("VERTEX: %s",buff);
	}
	glGetShaderiv(fshad,GL_COMPILE_STATUS,&status);
	if(!status){
		glGetShaderInfoLog(fshad,1000,NULL,buff);
		logcat("FRAGMENT: %s",buff);
	}
	free(buff);
	glAttachShader(program,vshad);
	glAttachShader(program,fshad);
	glLinkProgram(program);
	glDetachShader(program,vshad);
	glDetachShader(program,fshad);
	glDeleteShader(vshad);
	glDeleteShader(fshad);
	return program;
}

// static global variables for use by the ftfont functions
// set by set_ftfont_params
static int ftglobal_initialized=false;
static int ftglobal_iscreenwidth,ftglobal_iscreenheight;
static float ftglobal_fscreenwidth,ftglobal_fscreenheight;
static unsigned ftglobal_vector,ftglobal_size,ftglobal_texcoords;
static const int cols=16,rows=6;
void set_ftfont_params(int iscreenwidth,int iscreenheight,float fscreenwidth,float fscreenheight,unsigned ftvector,unsigned ftsize,unsigned fttexcoords){
	ftglobal_initialized=true;
	ftglobal_iscreenwidth=iscreenwidth;
	ftglobal_iscreenheight=iscreenheight;
	ftglobal_fscreenwidth=fscreenwidth;
	ftglobal_fscreenheight=fscreenheight;
	ftglobal_vector=ftvector;
	ftglobal_size=ftsize;
	ftglobal_texcoords=fttexcoords;
}
struct kernvector{
	float advance,bitmap_left;
};
#define isvalidchar(c) ((c>31&&c<128)||c=='\n'||c=='\r')
static unsigned create_font_atlas(ftfont *font,struct AAssetManager *mgr,const char *facename,int pixelsize){
	AAsset *asset=AAssetManager_open(mgr,facename,AASSET_MODE_UNKNOWN);
	if(!asset){
		logcat("%s: error loading asset \"%s\"",__func__,facename);
		return 0;
	}
	int filesize=AAsset_seek(asset,0,SEEK_END);
	AAsset_seek(asset,0,SEEK_SET);
	char *chunk=malloc(filesize);
	AAsset_read(asset,chunk,filesize);
	AAsset_close(asset);
	
	int error;
	FT_Library library;
	error=FT_Init_FreeType(&library);
	if(error){
		logcat("%s: error initializing freetype",__func__);
		return 0;
	}
	
	FT_Face face;
	error=FT_New_Memory_Face(library,chunk,filesize,0,&face);
	if(error){
		logcat("%s: error creating face",__func__);
		
		return 0;
	}
	error=FT_Set_Pixel_Sizes(face,0,pixelsize);
	if(error){
		logcat("%s: error setting char size",__func__);
	}
	int adjustedsize=roundf(((face->bbox.yMax-face->bbox.yMin)/2048.0f)*face->size->metrics.y_ppem);
	font->fontsize=((float)adjustedsize/ftglobal_iscreenwidth)*ftglobal_fscreenwidth;
	
	unsigned char *bitmap=malloc(adjustedsize*adjustedsize*rows*cols*4);
	if(bitmap==NULL){
		logcat("%s: error 'malloc' no memory",__func__);
		return 0;
	}
	memset(bitmap,0,adjustedsize*adjustedsize*rows*cols*4);
	for(unsigned char character=32;character<128;character++){
		error=FT_Load_Char(face,character,FT_LOAD_RENDER);
		if(error)logcat("%s: error rendering char %d (%c)",__func__,(int)character,character);
		const int glyphwidth=face->glyph->bitmap.width,glyphheight=face->glyph->bitmap.rows;
		((struct kernvector*)font->kern)[character-32].advance=((face->glyph->metrics./*metrics.horiAdvance>>6*/horiAdvance>>6)/(float)ftglobal_iscreenwidth)*ftglobal_fscreenwidth;
		((struct kernvector*)font->kern)[character-32].bitmap_left=(face->glyph->bitmap_left/(float)ftglobal_iscreenwidth)*ftglobal_fscreenwidth;
		const unsigned char *buffer=face->glyph->bitmap.buffer;
		const int xpos=((character-32)%cols)*adjustedsize;
		int bearingadjust=((face->bbox.yMax/2048.0f)*face->size->metrics.y_ppem)-(face->glyph->metrics.horiBearingY>>6);
		const int ypos=(((character-32)/cols)*adjustedsize)+bearingadjust;
		const int index=((adjustedsize*adjustedsize*rows*cols*4)-(adjustedsize*cols*4))-(ypos*adjustedsize*cols*4)+(xpos*4);
		for(int i=index,j=0;j<glyphwidth*glyphheight;){
			if(i>=adjustedsize*adjustedsize*rows*cols*4||i<0){
				if(randomint(0,10)==0)logcat("char %c out of bounds %s by %d",character,i<0?"negative":"positive",i<0?-i:i-(adjustedsize*adjustedsize*rows*cols*4));i=0;}
			int level=buffer[j];
			if(level<255&&level>0){
				bitmap[i]=255;
				bitmap[i+1]=255;
				bitmap[i+2]=255;
				bitmap[i+3]=level;
			}
			else if(level==0){
				bitmap[i]=0;
				bitmap[i+1]=0;
				bitmap[i+2]=0;
				bitmap[i+3]=0;
			}
			else{
				bitmap[i]=255;
				bitmap[i+1]=255;
				bitmap[i+2]=255;
				bitmap[i+3]=255;
			}
			j++;
			i+=4;
			if(j%glyphwidth==0){
				i-=(adjustedsize*cols*4)+(glyphwidth*4);
			}
		}
	}
	unsigned atlas;
	glGenTextures(1,&atlas);
	glBindTexture(GL_TEXTURE_2D,atlas);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,adjustedsize*cols,adjustedsize*rows,0,GL_RGBA,GL_UNSIGNED_BYTE,bitmap);
	free(bitmap);
	
	FT_Done_Face(face);
	free(chunk);
	FT_Done_FreeType(library);
	return atlas;
}
ftfont *create_ftfont(struct AAssetManager *mgr,float fontsize,const char *facename){
	if(!ftglobal_initialized){
		logcat("%s: set font parameters befor calling this function",__func__);
		return NULL;
	}
	ftfont *font=malloc(sizeof(ftfont));
	font->kern=malloc(sizeof(struct kernvector)*96);
	int pixelsize=(fontsize/ftglobal_fscreenwidth)*ftglobal_iscreenwidth;
	font->atlas=create_font_atlas(font,mgr,facename,pixelsize);
	return font;
}
void destroy_ftfont(ftfont *font){
	glDeleteTextures(1,&font->atlas);
	free(font->kern);
	free(font);
}
#define alignx(xf) ((((int)((xf/ftglobal_fscreenwidth)*ftglobal_iscreenwidth))/(float)ftglobal_iscreenwidth)*ftglobal_fscreenwidth)
#define aligny(yf) ((((int)((yf/ftglobal_fscreenheight)*ftglobal_iscreenheight))/(float)ftglobal_iscreenheight)*ftglobal_fscreenheight)
void drawtext(ftfont *font,float xpos,float ypos,const char *output){
	glUniform2f(ftglobal_size,font->fontsize,font->fontsize);
	float xoffset=0.0f,yoffset=0.0f;
	int character=0;
	while(output[character]!=0){
		while(output[character]=='\n'){
			yoffset+=font->fontsize*1.1f;
			xoffset=0.0f;
			character++;
			continue;
		}
		if(!isvalidchar(output[character]))return;
		float bitmap_left=alignx(((struct kernvector*)font->kern)[output[character]-32].bitmap_left);
		xoffset+=bitmap_left;
		const float xnormal=1.0f/cols,ynormal=1.0/rows;
		float x=(float)((output[character]-32)%16)*xnormal;
		float y=(float)((output[character]-32)/16)*ynormal;
		glUniform2f(ftglobal_vector,alignx(xpos)+xoffset,aligny(ypos)+yoffset);
		glUniform4f(ftglobal_texcoords,x,x+xnormal,(1.0f-y-ynormal),1.0f-y);
		
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		xoffset+=alignx(((struct kernvector*)font->kern)[output[character]-32].advance)-bitmap_left;
		character++;
	}
}
static float linelength(ftfont *font,const char *line){
	int character=0;
	float length=0.0f;
	while(line[character]!=0&&line[character]!='\n'){
		length+=(((struct kernvector*)font->kern)[line[character]-32].advance);//+alignx(((struct kernvector*)font->kern)[line[character]-32].bitmap_left);
		character++;
	}
	return length;
}
void drawtextcentered(ftfont *font,float xpos,float ypos,const char *output){
	glUniform2f(ftglobal_size,font->fontsize,font->fontsize);
	float xoffset=0.0f,yoffset=0.0f;
	float adjustedxpos=xpos-(linelength(font,output)/2.0f);
	int character=0;
	while(output[character]!=0){
		while(output[character]=='\n'){
			yoffset+=font->fontsize*1.1f;
			xoffset=0.0f;
			adjustedxpos=xpos-(linelength(font,&output[character+1])/2.0f);
			character++;
			continue;
		}
		if(!isvalidchar(output[character]))return;
		float bitmap_left=alignx(((struct kernvector*)font->kern)[output[character]-32].bitmap_left);
		xoffset+=bitmap_left;
		const float xnormal=1.0f/cols,ynormal=1.0/rows;
		float x=(float)((output[character]-32)%16)*xnormal;
		float y=(float)((output[character]-32)/16)*ynormal;
		glUniform2f(ftglobal_vector,alignx(adjustedxpos)+xoffset,aligny(ypos)+yoffset);
		glUniform4f(ftglobal_texcoords,x,x+xnormal,(1.0f-y-ynormal),1.0f-y);
		
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		xoffset+=alignx(((struct kernvector*)font->kern)[output[character]-32].advance)-bitmap_left;
		character++;
	}
}
float textlen(ftfont *font,const char *text){
	int character=0;
	float length=0.0f,biggest=0.0f;
	while(text[character]!=0){
		if(text[character]=='\n'){
			if(length>biggest)biggest=length;
			length=0.0f;
			character++;
			continue;
		}
		if(!isvalidchar(text[character]))return 0.0f;
		else length+=((struct kernvector*)font->kern)[text[character]-32].advance;
		character++;
	}
	if(length>biggest)biggest=length;
	return biggest;
}
float textheight(ftfont *font,const char *text){
	int character=0;
	float height=font->fontsize;
	while(text[character]!=0){
		if(text[character]=='\n')height+=font->fontsize*1.1f;
		character++;
	}
	return height;
}

int randomint(int low,int high){
	if(low>=high)return low;
	return low+(lrand48()%(high-low+1));
}

float zerof(float *val,float step){
	if(*val>0.0f){
		*val-=step;
		if(*val<0.0f)*val=0.0f;
	}
	else if(*val<0.0f){
		*val+=step;
		if(*val>0.0f)*val=0.0f;
	}
	return *val;
}
float targetf(float *val,float step,float target){
	if(*val>target){
		*val-=step;
		if(*val<target)*val=target;
	}
	else if(*val<target){
		*val+=step;
		if(*val>target)*val=target;
	}
	return *val;
}
float align(float *rot,float step,float target){
	const float PI=3.1415926f;
	if(target==*rot)return *rot;
	while(*rot>2.0f*PI)*rot-=2.0f*PI;
	while(*rot<0.0f)*rot+=2.0f*PI;
	while(target>2.0f*PI)target-=2.0f*PI;
	while(target<0.0f)target+=2.0f*PI;
	if(target>*rot){
		if(target-*rot>PI){
			*rot-=step;
			if(*rot<0.0f){
				*rot+=2.0f*PI;
				if(*rot<target)*rot=target;
			}
		}else{
			*rot+=step;
			if(*rot>target)*rot=target;
		}
	}
	else{
		if(*rot-target>PI){
			*rot+=step;
			if(*rot>2.0f*PI){
				*rot-=2.0f*PI;
				if(*rot>target)*rot=target;
			}
		}else{
			*rot-=step;
			if(*rot<target)*rot=target;
		}
	}
	return *rot;
}

unsigned screenshot(int w,int h,int darken){
	unsigned char *pixels=malloc(w*h*4);
	glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
	for(int i=0;i<w*h*4;i+=4){
		pixels[i]/=darken;
		pixels[i+1]/=darken;
		pixels[i+2]/=darken;
	}
	unsigned texture;
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
	free(pixels);
	return texture;
}

static float gaussian(int x,float deviation){
	float base=sqrt(2.0f*3.14159f*(deviation*deviation));
	float power=-(x*x)/(2.0f*deviation*deviation);
	return (1.0f/base)*powf(2.718f,power);
}
unsigned screenshotblur(int w,int h,int resize,int intensity){
	w-=w%resize;
	h-=h%resize;
	const float deviation=1.1458f;
	const int blur=4;
	float *weights=malloc(((blur*2)+1)*sizeof(float));
	int weightindex=0;
	//float total=0.0f;
	for(int i=-blur;i<=blur;i++){
		weights[weightindex]=gaussian(i,deviation);
		//total+=weights[weightindex];
		weightindex++;
	}
	/*char c[6];
	sprintf(c,"%f",total);
	MessageBox(0,c,0,0);*/
	if(resize==0)resize=1;
	int shrinkw=w/resize;
	int shrinkh=h/resize;
	unsigned char *raw=malloc(w*h*4),*source=malloc(shrinkw*shrinkh*4),*pixels=malloc(shrinkw*shrinkh*4);
	glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,raw);
	for(int i=0,j=0;j<shrinkw*shrinkh*4;i+=4*resize){
		if(i%(w*4)==0)i+=(w*4*(resize-1));//+(resize==1?0:4);
		source[j]=raw[i];
		source[j+1]=raw[i+1];
		source[j+2]=raw[i+2];
		j+=4;
	}
	free(raw);
	for(int x=0;x<intensity;x++)
	for(int a=0;a<2;a++){
		for(int i=0;i<shrinkw*shrinkh*4;i+=4){
			float sumr=0;
			float sumg=0;
			float sumb=0;
			int index;
			weightindex=0;
			for(int j=-blur;j<=blur;j++){
				int index=((a==0)?i+(4*j):(i+(j*shrinkw*4)));
				if(index<0||index>=shrinkw*shrinkh*4)index=i;
				sumr+=source[index]*weights[weightindex];
				sumg+=source[index+1]*weights[weightindex];
				sumb+=source[index+2]*weights[weightindex];
				weightindex++;
			}
			pixels[i]=roundf(sumr);
			pixels[i+1]=roundf(sumg);
			pixels[i+2]=roundf(sumb);
			pixels[i+3]=255;
		}
		memcpy(source,pixels,shrinkw*shrinkh*4);
	}
	free(source);
	free(weights);
	unsigned texture;
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,shrinkw,shrinkh,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
	free(pixels);
	return texture;
}

static int decodeogg(char *encoded,int filesize,short **decoded,unsigned *size,unsigned *targetsize,pthread_mutex_t mutex){
	long long samplecount;
	unsigned index=filesize-1;
	while(!(encoded[index]=='S'&&encoded[index-1]=='g'&&encoded[index-2]=='g'&&encoded[index-3]=='O'))index--;
	
	char array[8];
	array[0]=encoded[index+3];
	array[1]=encoded[index+4];
	array[2]=encoded[index+5];
	array[3]=encoded[index+6];
	array[4]=encoded[index+7];
	array[5]=encoded[index+8];
	array[6]=encoded[index+9];
	array[7]=encoded[index+10];
	samplecount=*(long long*)array;
	
	//log("total samples = %lld",samplecount);
	pthread_mutex_lock(&mutex);
	*targetsize=samplecount*sizeof(short);
	pthread_mutex_unlock(&mutex);
	
	ogg_sync_state state;
	ogg_stream_state streamstate;
	ogg_page page;
	ogg_packet packet;
	vorbis_info vorbisinfo;
	vorbis_comment vorbiscomment;
	vorbis_dsp_state dsp;
	vorbis_block block;
	ogg_sync_init(&state);
	char *buffer=ogg_sync_buffer(&state,4096);
	int bytesread;
	if(4096<=filesize)bytesread=4096;
	else bytesread=filesize;
	memcpy(buffer,encoded,bytesread);
	index=bytesread;
	ogg_sync_wrote(&state,bytesread);
	if(ogg_sync_pageout(&state,&page)!=1){
		logcat("data appears to be corrupt or non vorbis data.");
		return false;
	}
	ogg_stream_init(&streamstate,ogg_page_serialno(&page));
	vorbis_info_init(&vorbisinfo);
	vorbis_comment_init(&vorbiscomment);
	if(ogg_stream_pagein(&streamstate,&page)<0){
		logcat("couldn't read first page");
		return false;
	}
	if(ogg_stream_packetout(&streamstate,&packet)!=1){
		logcat("couldn't read initial header packet");
		return false;
	}
	if(vorbis_synthesis_headerin(&vorbisinfo,&vorbiscomment,&packet)<0){
		logcat("no vorbis in here");
		return false;
	}
	int i=0;
	while(i<2){
		while(i<2){
			int result=ogg_sync_pageout(&state,&page);
			if(result==0)break;
			else if(result==1){
				ogg_stream_pagein(&streamstate,&page);
				while(i<2){
					result=ogg_stream_packetout(&streamstate,&packet);
					if(result==0)break;
					else if(result<0){
						logcat("corrupt secondary header");
						return false;
					}
					result=vorbis_synthesis_headerin(&vorbisinfo,&vorbiscomment,&packet);
					if(result<0){
						logcat("corrupt secondary header");
						return false;
					}
					i++;
				}
			}
		}
		buffer=ogg_sync_buffer(&state,4096);
		if(4096<=filesize-index)bytesread=4096;
		else bytesread=filesize-index;
		memcpy(buffer,encoded+index,bytesread);
		index+=bytesread;
		if(bytesread<1&&i<2){
			logcat("eof before finding all vorbis headers");
			return false;
		}
		ogg_sync_wrote(&state,bytesread);
	}
	if(vorbisinfo.channels>1){
		logcat("NO DUAL CHANNEL SHIT IN HERE");
		return false;
	}
	int convsize=4096/vorbisinfo.channels;
	*decoded=malloc(samplecount*sizeof(short));
	ogg_int16_t *convbuffer=malloc(sizeof(short)*4096);
	int offset=0;
	if(vorbis_synthesis_init(&dsp,&vorbisinfo)==0){
		vorbis_block_init(&dsp,&block);
		int eos=0;
		while(!eos){
			while(!eos){
				int result=ogg_sync_pageout(&state,&page);
				if(result==0)break;
				else if(result<0)logcat("missing or corrupt data in bitstream");
				else{
					ogg_stream_pagein(&streamstate,&page);
					while(1){
						result=ogg_stream_packetout(&streamstate,&packet);
						if(result==0)break;
						else if(result<0){}
						else{
							float **pcm;
							int samples;
							if(vorbis_synthesis(&block,&packet)==0)
								vorbis_synthesis_blockin(&dsp,&block);
							while((samples=vorbis_synthesis_pcmout(&dsp,&pcm))>0){
								int j,clipflag=0,bout=(samples<convsize?samples:convsize);
								for(i=0;i<vorbisinfo.channels;i++){
									ogg_int16_t *ptr=convbuffer+i;
									float *mono=pcm[i];
									for(j=0;j<bout;j++){
										int val=floor(mono[j]*32767.0f+0.5f);
										if(val>32767){
											val=32767;
											clipflag=1;
										}
										else if(val<-32768){
											val=-32768;
											clipflag=1;
										}
										*ptr=val;
										ptr+=vorbisinfo.channels;
									}
								}
								memcpy((char*)(*decoded)+offset,convbuffer,bout*2*vorbisinfo.channels);
								offset+=bout*2*vorbisinfo.channels;
								pthread_mutex_lock(&mutex);
								*size=offset;
								pthread_mutex_unlock(&mutex);
								vorbis_synthesis_read(&dsp,bout);
							}
						}
					}
					if(ogg_page_eos(&page))eos=1;
				}
			}
			if(!eos){
				buffer=ogg_sync_buffer(&state,4096);
				if(4096<=filesize-index)bytesread=4096;
				else bytesread=filesize-index;
				memcpy(buffer,encoded+index,bytesread);
				index+=bytesread;
				ogg_sync_wrote(&state,bytesread);
				if(bytesread==0)eos=1;
			}
		}
		vorbis_block_clear(&block);
		vorbis_dsp_clear(&dsp);
	}
	else{
		logcat("corrupt header during playback init");
	}
	
	ogg_stream_clear(&streamstate);
	vorbis_comment_clear(&vorbiscomment);
	vorbis_info_clear(&vorbisinfo);
	ogg_sync_clear(&state);
	free(convbuffer);
	return true;
}
void playercallback(SLAndroidSimpleBufferQueueItf buffq,void *vcontext){
	struct audioplayer *context=vcontext;
	pthread_mutex_lock(&context->sound->parent->mutex);
	unsigned size=context->sound->size,targetsize=context->sound->targetsize;
	pthread_mutex_unlock(&context->sound->parent->mutex);
	//log("size: %d, targetsize: %d",size,targetsize);
	while(size==context->initial&&size<targetsize){
			pthread_mutex_lock(&context->sound->parent->mutex);
			size=context->sound->size;
			pthread_mutex_unlock(&context->sound->parent->mutex);
			usleep(10*1000);
	}
	if(context->initial<targetsize){
		(*context->playerbufferqueue)->Enqueue(context->playerbufferqueue,context->sound->buffer+(context->initial/2),size-context->initial);
		context->initial=size;
		return;
	}
	if(context->loop){
		(*context->playerbufferqueue)->Enqueue(context->playerbufferqueue,context->sound->buffer,size);
		return;
	}
	context->destroy=true;
}
void disablesound(slesenv *engine){
	engine->enabled=false;
	for(struct audioplayer *ap=engine->audioplayerlist;ap!=NULL;ap=ap->next){
		pthread_mutex_lock(&ap->sound->parent->mutex);
		(*ap->volumeinterface)->SetVolumeLevel(ap->volumeinterface,SL_MILLIBEL_MIN);
		pthread_mutex_unlock(&ap->sound->parent->mutex);
	}
}
void enablesound(slesenv *engine){
	engine->enabled=true;
	for(struct audioplayer *ap=engine->audioplayerlist;ap!=NULL;ap=ap->next){
		pthread_mutex_lock(&ap->sound->parent->mutex);
		(*ap->volumeinterface)->SetVolumeLevel(ap->volumeinterface,0);// zero is max
		pthread_mutex_unlock(&ap->sound->parent->mutex);
	}
}
void stopsound(slesenv *engine,struct audioplayer *audioplayer){
	pthread_mutex_lock(&audioplayer->sound->parent->mutex);
	(*audioplayer->playerinterface)->SetPlayState(audioplayer->playerinterface,SL_PLAYSTATE_STOPPED);
	audioplayer->loop=false;
	audioplayer->destroy=true;
	pthread_mutex_unlock(&audioplayer->sound->parent->mutex);
}
void stopallsounds(slesenv *soundengine){
	for(struct audioplayer *audioplayer=soundengine->audioplayerlist;audioplayer!=NULL;){
		(*audioplayer->playerobject)->Destroy(audioplayer->playerobject);
		void *temp=audioplayer->next;
		free(audioplayer);
		audioplayer=temp;
	}
	soundengine->audioplayerlist=NULL;
}
struct audioplayer *playsound(slesenv *engine,struct apacksound *sound,int loop){
	if(!engine->enabled)return NULL;
	pthread_mutex_lock(&sound->parent->mutex);
	unsigned size=sound->size,targetsize=sound->targetsize;
	pthread_mutex_unlock(&sound->parent->mutex);
	if(size==0){
		if(!loop)return NULL;// don't care, non loops are usually not high-priority
		while(size==0){
			pthread_mutex_lock(&sound->parent->mutex);
			size=sound->size;
			targetsize=sound->targetsize;
			pthread_mutex_unlock(&sound->parent->mutex);
			usleep(10*1000);
		}
	}
	struct audioplayer *audioplayer=malloc(sizeof(struct audioplayer));
	
	SLDataLocator_AndroidSimpleBufferQueue buffq={SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
	SLDataFormat_PCM format_pcm={SL_DATAFORMAT_PCM,1,SL_SAMPLINGRATE_44_1,SL_PCMSAMPLEFORMAT_FIXED_16,SL_PCMSAMPLEFORMAT_FIXED_16,SL_SPEAKER_FRONT_CENTER,SL_BYTEORDER_LITTLEENDIAN};
	SLDataSource source={&buffq,&format_pcm};
	SLDataLocator_OutputMix localoutputmix={SL_DATALOCATOR_OUTPUTMIX,engine->outputmix};
	SLDataSink sink={&localoutputmix,NULL};
	
	(*engine->engineinterface)->CreateAudioPlayer(engine->engineinterface,&audioplayer->playerobject,&source,&sink,2,(SLInterfaceID[]){SL_IID_ANDROIDSIMPLEBUFFERQUEUE,SL_IID_VOLUME},(SLboolean[]){SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE});
	(*audioplayer->playerobject)->Realize(audioplayer->playerobject,SL_BOOLEAN_FALSE);
	(*audioplayer->playerobject)->GetInterface(audioplayer->playerobject,SL_IID_PLAY,&audioplayer->playerinterface);
	(*audioplayer->playerobject)->GetInterface(audioplayer->playerobject,SL_IID_VOLUME,&audioplayer->volumeinterface);
	(*audioplayer->playerobject)->GetInterface(audioplayer->playerobject,SL_IID_ANDROIDSIMPLEBUFFERQUEUE,&audioplayer->playerbufferqueue);
	(*audioplayer->playerbufferqueue)->RegisterCallback(audioplayer->playerbufferqueue,playercallback,audioplayer);
	(*audioplayer->playerinterface)->SetPlayState(audioplayer->playerinterface,SL_PLAYSTATE_PLAYING);
	
	//log("size: %d, targetsize: %d",size,targetsize);
	audioplayer->initial=size;
	(*audioplayer->playerbufferqueue)->Enqueue(audioplayer->playerbufferqueue,sound->buffer,size);
	audioplayer->loop=loop;
	audioplayer->engine=engine;
	audioplayer->sound=sound;
	audioplayer->destroy=false;
	
	audioplayer->next=engine->audioplayerlist;
	engine->audioplayerlist=audioplayer;
	//#define COUNT_OBJECTS
	#ifdef COUNT_OBJECTS
	int count=0;
	#endif
	for(struct audioplayer *ap=engine->audioplayerlist,*prevap=NULL;ap!=NULL;){
		if(ap->destroy){
			(*ap->playerobject)->Destroy(ap->playerobject);
			if(prevap!=NULL)prevap->next=ap->next;
			else engine->audioplayerlist=ap->next;
			void *temp=ap->next;
			free(ap);
			ap=temp;
			continue;
		}
		#ifdef COUNT_OBJECTS
		count++;
		#endif
		prevap=ap;
		ap=ap->next;
	}
	#ifdef COUNT_OBJECTS
	logcat("count: %d",count);
	#endif
	return audioplayer;
}
slesenv *initOpenSL(){
	slesenv *soundengine=malloc(sizeof(slesenv));
	slCreateEngine(&soundengine->engine,0,NULL,0,NULL,NULL);
	(*soundengine->engine)->Realize(soundengine->engine,SL_BOOLEAN_FALSE);
	(*soundengine->engine)->GetInterface(soundengine->engine,SL_IID_ENGINE,&soundengine->engineinterface);
	(*soundengine->engineinterface)->CreateOutputMix(soundengine->engineinterface,&soundengine->outputmix,0/*1*/,NULL/*(const SLInterfaceID[]){SL_IID_VOLUME}*/,NULL/*(SLboolean[]){SL_BOOLEAN_FALSE}*/);
	(*soundengine->outputmix)->Realize(soundengine->outputmix,SL_BOOLEAN_FALSE);
	soundengine->audioplayerlist=NULL;
	soundengine->enabled=true;
	return soundengine;
}
void termOpenSL(slesenv *soundengine){
	for(struct audioplayer *audioplayer=soundengine->audioplayerlist;audioplayer!=NULL;){
		(*audioplayer->playerobject)->Destroy(audioplayer->playerobject);
		void *temp=audioplayer->next;
		free(audioplayer);
		audioplayer=temp;
	}
	(*soundengine->outputmix)->Destroy(soundengine->outputmix);
	(*soundengine->engine)->Destroy(soundengine->engine);
	free(soundengine);
}

void init_jni(struct android_app *app,struct jni_info *jni_info){
	jni_info->vm=app->activity->vm;
	jni_info->clazz=app->activity->clazz;
	(*jni_info->vm)->AttachCurrentThread(jni_info->vm,&jni_info->env,NULL);
	jni_info->sys_svc=(*jni_info->env)->GetObjectClass(jni_info->env,jni_info->clazz);
	jmethodID mid=(*jni_info->env)->GetMethodID(jni_info->env,jni_info->sys_svc,"getSystemService","(Ljava/lang/String;)Ljava/lang/Object;");
	jstring mstr=(*jni_info->env)->NewStringUTF(jni_info->env,"vibrator");
	jni_info->vb_svc=(*jni_info->env)->CallObjectMethod(jni_info->env,jni_info->clazz,mid,mstr);
	jobject vb=(*jni_info->env)->GetObjectClass(jni_info->env,jni_info->vb_svc);
	jmethodID hasvbmethod=(*jni_info->env)->GetMethodID(jni_info->env,vb,"hasVibrator","()Z");
	jni_info->hasvb=(*jni_info->env)->CallBooleanMethod(jni_info->env,jni_info->vb_svc,hasvbmethod);
	if(!jni_info->hasvb)logcat("This device cannot vibrate");
	else jni_info->vbmethod=(*jni_info->env)->GetMethodID(jni_info->env,vb,"vibrate","(J)V");
	
	jni_info-> MethodGetWindow = (*jni_info->env)->GetMethodID(jni_info->env,jni_info->sys_svc, "getWindow", "()Landroid/view/Window;");
	jni_info-> lWindow = (*jni_info->env)->CallObjectMethod(jni_info->env,jni_info-> clazz, jni_info->MethodGetWindow);
	jni_info-> cWindow = (*jni_info->env)->FindClass(jni_info->env,"android/view/Window");
	jni_info-> cView = (*jni_info->env)->FindClass(jni_info->env,"android/view/View");
	jni_info-> MethodGetDecorView = (*jni_info->env)->GetMethodID(jni_info->env, jni_info->cWindow, "getDecorView", "()Landroid/view/View;");
	jni_info-> lDecorView = (*jni_info->env)->CallObjectMethod(jni_info->env, jni_info->lWindow, jni_info->MethodGetDecorView);
	jni_info-> MethodSetSystemUiVisibility = (*jni_info->env)->GetMethodID(jni_info->env,jni_info->cView, "setSystemUiVisibility", "(I)V");
	
	(*jni_info->env)->DeleteLocalRef(jni_info->env,mstr);
	(*jni_info->env)->DeleteLocalRef(jni_info->env,vb);
}
void vibratedevice(struct jni_info *jni_info,int mills){
	if(jni_info->hasvb){
		jlong v1=mills;
		(*jni_info->env)->CallVoidMethod(jni_info->env,jni_info->vb_svc,jni_info->vbmethod,v1);
	}
}
void term_jni(struct jni_info *jni_info){
	(*jni_info->env)->DeleteLocalRef(jni_info->env,jni_info->sys_svc);
	(*jni_info->env)->DeleteLocalRef(jni_info->env,jni_info->lWindow);
	(*jni_info->env)->DeleteLocalRef(jni_info->env,jni_info->cWindow);
	(*jni_info->env)->DeleteLocalRef(jni_info->env,jni_info->cView);
	(*jni_info->env)->DeleteLocalRef(jni_info->env,jni_info->lDecorView);
	(*jni_info->env)->DeleteLocalRef(jni_info->env,jni_info->vb_svc);
	(*jni_info->vm)->DetachCurrentThread(jni_info->vm);
}
void hidenavbars(struct jni_info *jni_info){
	jint jVisibility = 256 | 512 | 1024 | 2 | 4 | 4096;
	(*jni_info->env)->CallVoidMethod(jni_info->env, jni_info->lDecorView, jni_info->MethodSetSystemUiVisibility, jVisibility);
	if((*jni_info->env)->ExceptionCheck){
		(*jni_info->env)->ExceptionClear(jni_info->env);
	}
}