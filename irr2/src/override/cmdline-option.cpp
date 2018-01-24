/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "override/cmdline-option.h"
#include <string>
#include <cstring>

const AndroidOptions* android_cmdLineOptions = NULL;

int
android_parse_options(int  argc, char*  *argv, AndroidOptions*  opt )
{
  int i = 0;

  argc -= 2;
  i += 2;

  memset( opt, 0, sizeof *opt );

  while (argc >= 2) {
    const char* opt_str = argv[i];
    if (!strcmp(opt_str, "-url")) {
      opt->url = argv[i+1];

    } else if (!strcmp(opt_str, "-b")) {
      opt->b = argv[i+1];

    } else if (!strcmp(opt_str, "-codec")) {
      opt->codec = argv[i+1];

    } else if (!strcmp(opt_str, "-gop")) {
      opt->gop = argv[i+1];

    } else if (!strcmp(opt_str, "-fr")) {
      opt->fr = argv[i+1];

    } else if (!strcmp(opt_str, "-res")) {
      opt->res = argv[i+1];

    }
    argc -= 2;
    i += 2;
  }

  if (argc == 0) {
    return 0;
  } else {
    return -1;
  }
}
