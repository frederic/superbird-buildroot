#ifndef _CT_H_
#define _CT_H_


int player_init(void);
void player_delinit(void);
int start_play(void);
int stop_play(void);
int pause_play(void);
int next(void);
int previous(void);
int volume_up();
int volume_down();
void connect_call_back(gboolean connected);
void play_call_back(char *status);

#endif
