#ifndef COMMANDS_H
#define COMMANDS_H

void execute_locate(char **args, int arg_count);
void execute_reveal(char **args, int arg_count);
void execute_peek(char **args, int arg_count);
void execute_activities();
void execute_ping(char **args, int arg_count);
void execute_resume(char **args, int arg_count);
void execute_spy(char **args, int arg_count);
void execute_snoop(char **args, int arg_count);

#endif