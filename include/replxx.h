/* linenoise.h -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * See linenoise.c for more information.
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __REPLXX_H
#define __REPLXX_H

#define REPLXX_VERSION "0.0.1"
#define REPLXX_VERSION_MAJOR 0
#define REPLXX_VERSION_MINOR 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct replxx_completions replxx_completions;

typedef void(replxx_completion_callback_t)(const char*, replxx_completions*);
void replxx_set_completion_callback(replxx_completion_callback_t* fn);
void replxx_add_completion(replxx_completions* lc, const char* str);

char* replxx_input(const char* prompt);
void replxx_set_preload_buffer(const char* preloadText);
int replxx_history_add(const char* line);
void replxx_set_max_line_size(int len);
void replxx_set_double_tab_completion(int val);
void replxx_set_complete_on_empty(int val);
void replxx_set_beep_on_ambiguous_completion(int val);
int replxx_set_max_history_size(int len);
char* replxx_history_line(int index);
int replxx_history_save(const char* filename);
int replxx_history_load(const char* filename);
void replxx_history_free(void);
void replxx_clear_screen(void);
void replxx_debug_dump_print_codes(void);
/* the following is extension to the original linenoise API */
int replxx_install_window_change_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __REPLXX_H */
