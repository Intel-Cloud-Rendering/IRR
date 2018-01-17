/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   cmdline-option.h
 * Author: media
 *
 * Created on January 18, 2018, 1:34 PM
 */

#ifndef CMDLINE_OPTION_H
#define CMDLINE_OPTION_H

#include <string>

typedef struct {
  char* url;
  char* b;
  char* codec;
  char* gop;
  char* fr;
  char* res;
} AndroidOptions;

extern const AndroidOptions* android_cmdLineOptions;

extern int
android_parse_options( int  argc, char*  *argv, AndroidOptions*  opt );

#endif /* CMDLINE_OPTION_H */

