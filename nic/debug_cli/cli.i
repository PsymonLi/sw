/*
{C} Copyright 2017 Pensando Systems Inc. All rights reserved
*/

%module cli
%{
    extern int cli_init(char *args);
%}
int cli_init(char *args);
