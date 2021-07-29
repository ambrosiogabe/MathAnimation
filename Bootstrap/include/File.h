#ifndef BOOTSTRAP_FILE_H
#define BOOTSTRAP_FILE_H


bool manim_remove_dir(const char* directoryName);
bool manim_is_dir(const char* directoryName);
bool manim_is_file(const char* directoryName);
bool manim_move_file(const char* from, const char* to);
bool manim_create_dir_if_not_exists(const char* directoryName);

#endif 
