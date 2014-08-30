#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#define LOG_TAG ("gles2ndtest")
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))


struct engine {
	android_app* app;
	EGLDisplay display;
	EGLSurface surface;
};


const char gVertexShader[] =
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";


const char gFragmentShader[] =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";


GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &pSource, nullptr);
	glCompileShader(shader);
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    return shader;
}


GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, pixelShader);
	glLinkProgram(program);
	GLint linkStatus = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    return program;
}

int init(engine* e) {
	const EGLint attribs[] = {
		EGL_BLUE_SIZE,       8,
		EGL_GREEN_SIZE,      8,
		EGL_RED_SIZE,        8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	EGLConfig config;
	EGLint numConfigs;
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	EGLint format;
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(e->app->window, 0, 0, format);

	EGLSurface surface = eglCreateWindowSurface(display, config, e->app->window, nullptr);
	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		return -1;
	}

	EGLint w, h;
	eglQuerySurface(display, surface, EGL_WIDTH,  &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	e->display = display;
	e->surface = surface;

	glViewport(0, 0, w, h);

}

// 描画
void draw(engine* e) {
    GLuint gProgram = createProgram(gVertexShader, gFragmentShader);
    GLuint gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");

	const GLfloat vertices[] = {
		 0.0f,  0.5f,
		-0.5f, -0.5f,
		 0.5f, -0.5f
	};

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(gProgram);
	glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(gvPositionHandle);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	eglSwapBuffers(e->display, e->surface);
}


void android_main(android_app* state) {
	app_dummy();

	engine e;
	state->userData = &e;
	state->onAppCmd = [](android_app* app, int32_t cmd) {
		auto e = static_cast<engine*>(app->userData);
		switch (cmd) {
			case APP_CMD_INIT_WINDOW:
				init(e);
				draw(e);
				break;
		}
	};
	e.app = state;

	while (1) {
		int ident, events;
		android_poll_source* source;
		while ((ident=ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) {
			if (source != nullptr) {
				source->process(state, source);
			}
			if (state->destroyRequested != 0) {
				return;
			}
		}
	}
}

