// This software is copyright (c) 1996-2015 by
//      John Tromp
//      600 Route 25A
//      East Setauket
//      NY 11733
// E-mail: john.tromp@gmail.com
//
// This notice must not be removed.
// This software must not be sold for profit.
// You may redistribute if your distributees have the
// same rights and restrictions.

#include "Search.h"

class Window {
  Game game;
  Search search;
  static const char *optns[];
  int moves[SIZE],nmoves, movenr;

public:
  Window(const char *bookfile);
  ~Window();
  void refresh();
  bool back();
  bool forward();
  bool play(int n);
  void showgamemoves();
  bool reset();
  bool solve();
  bool evals();
  bool stats();
  bool chglvl(int l);
  void showevals();
};
