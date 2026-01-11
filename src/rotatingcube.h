#ifndef ROTATINGCUBE_H
#define ROTATINGCUBE_H

#include <SDL3/SDL.h>

void rotatingcube_init();
void rotatingcube_draw(float width, float height);
void rotatingcube_cleanup();

void rotatingcube_handle_event(const SDL_Event *event, float scale);
void rotatingcube_pause(bool pause);
bool rotatingcube_is_paused();

#endif