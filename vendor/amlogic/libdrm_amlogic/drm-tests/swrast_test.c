/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

const char *get_gl_error()
{
	switch (glGetError()) {
		case GL_NO_ERROR:
			return "No error has been recorded.";
		case GL_INVALID_ENUM:
			return "An unacceptable value is specified for an enumerated argument. The "
			       "offending command is ignored and has no other side effect than to "
			       "set the error flag.";
		case GL_INVALID_VALUE:
			return "A numeric argument is out of range. The offending command is "
			       "ignored and has no other side effect than to set the error flag.";
		case GL_INVALID_OPERATION:
			return "The specified operation is not allowed in the current state. The "
			       "offending command is ignored and has no other side effect than to "
			       "set the error flag.";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "The command is trying to render to or read from the framebuffer "
			       "while the currently bound framebuffer is not framebuffer complete "
			       "(i.e. the return value from glCheckFramebufferStatus is not "
			       "GL_FRAMEBUFFER_COMPLETE). The offending command is ignored and has "
			       "no other side effect than to set the error flag.";
		case GL_OUT_OF_MEMORY:
			return "There is not enough memory left to execute the command. The state "
			       "of the GL is undefined, except for the state of the error flags, "
			       "after this error is recorded.";
		default:
			return "Unknown error";
	}
}

const char *get_egl_error()
{
	switch (eglGetError()) {
		case EGL_SUCCESS:
			return "The last function succeeded without error.";
		case EGL_NOT_INITIALIZED:
			return "EGL is not initialized, or could not be initialized, for the "
			       "specified EGL display connection.";
		case EGL_BAD_ACCESS:
			return "EGL cannot access a requested resource (for example a context is "
			       "bound in another thread).";
		case EGL_BAD_ALLOC:
			return "EGL failed to allocate resources for the requested operation.";
		case EGL_BAD_ATTRIBUTE:
			return "An unrecognized attribute or attribute value was passed in the "
			       "attribute list.";
		case EGL_BAD_CONTEXT:
			return "An EGLContext argument does not name a valid EGL rendering "
			       "context.";
		case EGL_BAD_CONFIG:
			return "An EGLConfig argument does not name a valid EGL frame buffer "
			       "configuration.";
		case EGL_BAD_CURRENT_SURFACE:
			return "The current surface of the calling thread is a window, pixel "
			       "buffer or pixmap that is no longer valid.";
		case EGL_BAD_DISPLAY:
			return "An EGLDisplay argument does not name a valid EGL display "
			       "connection.";
		case EGL_BAD_SURFACE:
			return "An EGLSurface argument does not name a valid surface (window, "
			       "pixel buffer or pixmap) configured for GL rendering.";
		case EGL_BAD_MATCH:
			return "Arguments are inconsistent (for example, a valid context requires "
			       "buffers not supplied by a valid surface).";
		case EGL_BAD_PARAMETER:
			return "One or more argument values are invalid.";
		case EGL_BAD_NATIVE_PIXMAP:
			return "A NativePixmapType argument does not refer to a valid native "
			       "pixmap.";
		case EGL_BAD_NATIVE_WINDOW:
			return "A NativeWindowType argument does not refer to a valid native "
			       "window.";
		case EGL_CONTEXT_LOST:
			return "A power management event has occurred. The application must "
			       "destroy all contexts and reinitialise OpenGL ES state and objects "
			       "to continue rendering.";
		default:
			return "Unknown error";
	}
}

struct context {
	unsigned width;
	unsigned height;
	EGLDisplay egl_display;
	EGLContext egl_ctx;

	unsigned gl_fb;
	unsigned gl_rb;
};

float f(int i)
{
	int a = i % 40;
	int b = (i / 40) % 6;
	switch (b) {
		case 0:
		case 1:
			return 0.0f;
		case 3:
		case 4:
			return 1.0f;
		case 2:
			return (a / 40.0f);
		case 5:
			return 1.0f - (a / 40.0f);
		default:
			return 0.0f;
	}
}

void draw(struct context *ctx)
{
	int i;
	const GLchar *vertexShaderStr =
	    "attribute vec4 vPosition;\n"
	    "attribute vec4 vColor;\n"
	    "varying vec4 vFillColor;\n"
	    "void main() {\n"
	    "  gl_Position = vPosition;\n"
	    "  vFillColor = vColor;\n"
	    "}\n";
	const GLchar *fragmentShaderStr =
	    "precision mediump float;\n"
	    "varying vec4 vFillColor;\n"
	    "void main() {\n"
	    "  gl_FragColor = vFillColor;\n"
	    "}\n";
	GLint vertexShader, fragmentShader, program, status;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) {
		fprintf(stderr, "Failed to create vertex shader. Error=0x%x\n", glGetError());
		return;
	}
	glShaderSource(vertexShader, 1, &vertexShaderStr, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		fprintf(stderr, "Failed to compile vertex shader. Error=0x%x\n", glGetError());
		return;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragmentShader) {
		fprintf(stderr, "Failed to create fragment shader. Error=0x%x\n", glGetError());
		return;
	}
	glShaderSource(fragmentShader, 1, &fragmentShaderStr, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		fprintf(stderr, "Failed to compile fragment shader. Error=0x%x\n", glGetError());
		return;
	}

	program = glCreateProgram();
	if (!program) {
		fprintf(stderr, "Failed to create program.\n");
		return;
	}
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glBindAttribLocation(program, 0, "vPosition");
	glBindAttribLocation(program, 1, "vColor");
	glLinkProgram(program);
	glGetShaderiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		fprintf(stderr, "Failed to link program.\n");
		return;
	}

	glViewport(0, 0, (GLint)ctx->width, (GLint)ctx->height);

	for (i = 0; i <= 500; i++) {
		GLfloat verts[] = { 0.0f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f };
		GLfloat colors[] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
				     0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };

		glClearColor(f(i), f(i + 80), f(i + 160), 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, verts);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, colors);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		usleep(1e6 / 120); /* 120 Hz */
		glFinish();

		unsigned char pixels[4];
		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		printf("color = %hhu %hhu %hhu %hhu\n", pixels[0], pixels[1], pixels[2], pixels[3]);
	}

	glDeleteProgram(program);
}

int main(int argc, char **argv)
{
	int ret = 0;
	struct context ctx;
	EGLint egl_major, egl_minor;
	const char *extensions;
	EGLint num_configs;
	EGLConfig egl_config;

	ctx.width = 800;
	ctx.height = 600;

	const EGLint config_attribs[] = { EGL_RED_SIZE,
					  1,
					  EGL_GREEN_SIZE,
					  1,
					  EGL_BLUE_SIZE,
					  1,
					  EGL_DEPTH_SIZE,
					  1,
					  EGL_RENDERABLE_TYPE,
					  EGL_OPENGL_ES2_BIT,
					  EGL_SURFACE_TYPE,
					  EGL_DONT_CARE,
					  EGL_NONE };
	const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	ctx.egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (ctx.egl_display == EGL_NO_DISPLAY) {
		fprintf(stderr, "failed to get egl display\n");
		ret = 1;
		goto fail;
	}

	if (!eglInitialize(ctx.egl_display, &egl_major, &egl_minor)) {
		fprintf(stderr, "failed to initialize egl: %s\n", get_egl_error());
		ret = 1;
		goto terminate_display;
	}

	printf("EGL %d.%d\n", egl_major, egl_minor);
	printf("EGL %s\n", eglQueryString(ctx.egl_display, EGL_VERSION));

	extensions = eglQueryString(ctx.egl_display, EGL_EXTENSIONS);
	printf("EGL Extensions: %s\n", extensions);

	if (!eglChooseConfig(ctx.egl_display, config_attribs, NULL, 0, &num_configs)) {
		fprintf(stderr, "eglChooseConfig() failed with error: %x\n", eglGetError());
		goto terminate_display;
	}
	if (!eglChooseConfig(ctx.egl_display, config_attribs, &egl_config, 1, &num_configs)) {
		fprintf(stderr, "eglChooseConfig() failed with error: %x\n", eglGetError());
		goto terminate_display;
	}

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		fprintf(stderr, "failed to bind OpenGL ES: %s\n", get_egl_error());
		ret = 1;
		goto terminate_display;
	}

	if (bs_egl_has_extension("EGL_KHR_no_config_context", extensions)) {
		ctx.egl_ctx =
		    eglCreateContext(ctx.egl_display, NULL /* No Config */,
				     EGL_NO_CONTEXT /* No shared context */, context_attribs);
	} else {
		ctx.egl_ctx =
		    eglCreateContext(ctx.egl_display, egl_config,
				     EGL_NO_CONTEXT /* No shared context */, context_attribs);
	}

	if (ctx.egl_ctx == EGL_NO_CONTEXT) {
		fprintf(stderr, "failed to create OpenGL ES Context: %s\n", get_egl_error());
		ret = 1;
		goto terminate_display;
	}

	if (!eglMakeCurrent(ctx.egl_display, EGL_NO_SURFACE /* No default draw surface */,
			    EGL_NO_SURFACE /* No default draw read */, ctx.egl_ctx)) {
		fprintf(stderr, "failed to make the OpenGL ES Context current: %s\n",
			get_egl_error());
		ret = 1;
		goto destroy_context;
	}

	printf("GL extensions: %s\n", glGetString(GL_EXTENSIONS));

	glGenFramebuffers(1, &ctx.gl_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, ctx.gl_fb);
	glGenRenderbuffers(1, &ctx.gl_rb);
	glBindRenderbuffer(GL_RENDERBUFFER, ctx.gl_rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, ctx.width, ctx.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, ctx.gl_rb);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "failed to create framebuffer: %s\n", get_gl_error());
		ret = 1;
		goto delete_gl_buffers;
	}

	draw(&ctx);

delete_gl_buffers:
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &ctx.gl_fb);
	glDeleteRenderbuffers(1, &ctx.gl_rb);
destroy_context:
	eglMakeCurrent(ctx.egl_display, NULL, NULL, NULL);
	eglDestroyContext(ctx.egl_display, ctx.egl_ctx);
terminate_display:
	eglTerminate(ctx.egl_display);
fail:
	return ret;
}
