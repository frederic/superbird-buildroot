/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

GLuint bs_gl_shader_create(GLenum type, const GLchar *src)
{
	assert(src);
	GLuint shader = glCreateShader(type);
	if (!shader) {
		bs_debug_error("failed call to glCreateShader(%d)", type);
		return 0;
	}

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint log_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		char *shader_log = calloc(log_len, sizeof(char));
		assert(shader_log);
		glGetShaderInfoLog(shader, log_len, NULL, shader_log);
		bs_debug_error("failed to compile shader: %s", shader_log);
		free(shader_log);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint bs_gl_program_create_vert_frag_bind(const GLchar *vert_src, const GLchar *frag_src,
					   struct bs_gl_program_create_binding *bindings)
{
	assert(vert_src);
	assert(frag_src);
	GLuint program = glCreateProgram();
	if (!program) {
		bs_debug_error("failed to create program");
		return 0;
	}

	GLuint vert_shader = bs_gl_shader_create(GL_VERTEX_SHADER, vert_src);
	if (!vert_shader) {
		bs_debug_error("failed to create vertex shader");
		glDeleteProgram(program);
		return 0;
	}

	GLuint frag_shader = bs_gl_shader_create(GL_FRAGMENT_SHADER, frag_src);
	if (!frag_shader) {
		bs_debug_error("failed to create fragment shader");
		glDeleteShader(vert_shader);
		glDeleteProgram(program);
		return 0;
	}

	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	if (bindings) {
		for (size_t binding_index = 0; bindings[binding_index].name != NULL;
		     binding_index++) {
			const struct bs_gl_program_create_binding *binding =
			    &bindings[binding_index];
			glBindAttribLocation(program, binding->index, binding->name);
		}
	}
	glLinkProgram(program);
	glDetachShader(program, vert_shader);
	glDetachShader(program, frag_shader);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		GLint log_len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
		char *program_log = calloc(log_len, sizeof(char));
		assert(program_log);
		glGetProgramInfoLog(program, log_len, NULL, program_log);
		bs_debug_error("failed to link program: %s", program_log);
		free(program_log);
		glDeleteProgram(program);
		return 0;
	}

	return program;
}
