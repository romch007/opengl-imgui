#ifndef ROTATINGCUBE_H
#define ROTATINGCUBE_H

void rotatingcube_init();
void rotatingcube_draw(float width, float height);
void rotatingcube_cleanup();

void rotatingcube_pause(bool pause);
bool rotatingcube_is_paused();

#endif