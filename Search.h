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

#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <map>
#include "Game.h"

// should be a prime between 2^{SIZE1-LOCKSIZE} and 2^32, e.g.
// 4194301,8306069,8388593,15999961,33554393,67108859,134217689
// 268435399,982451653,1040187403,2147483647 (8th Mersenne primes 2^31-1)
#ifndef TRANSIZE
#define TRANSIZE 8306069
#endif
#ifndef LOCKSIZE
#define LOCKSIZE 50
#endif
#ifndef SYMMREC 
#define SYMMREC 10    // symmetry normalize first SYMMREC moves
#endif
#define BBYTES ((SIZE1+7)/8)

typedef enum {UNKNOWN,LOSS,DRAWLOSS,DRAW,DRAWWIN,WIN,LOSSWIN,NSCORES=LOSSWIN} score;

#if (LOCKSIZE<=32)
  typedef unsigned locktype;
#else
  typedef uint64_t locktype;
#endif
typedef uint64_t proofnrtype;

class Trans {
public:
  // limit waste in hashentry size to less than 4 byte
  #pragma pack(4)
  struct Hashentry {
    locktype biglock:LOCKSIZE;
    unsigned bigwork:6;
    locktype newlock:LOCKSIZE;
    score newscore:3;
    score bigscore:3;

    void store(locktype lock, score sc, int work);
  };
  #pragma pack()

  Hashentry *ht;
  
  Trans();
  ~Trans();
  void clear();
  void store(bitboard bb, score sc, int work);
  struct Stats {
    uint64_t typecnt[NSCORES];
    uint64_t total;

    Stats(const Trans &tt);
  };
};

class Hash {
public:
  static uint64_t nhashed;
  Trans::Hashentry *he;
  locktype lock;
  uint64_t oldhashed;

  Hash(Trans &tt, const Game *game);
  score transpose();
  int store(score sc);
};

class PNTree {
public:
  // limit waste in hashentry size to less than 4 byte
  #pragma pack(4)
  struct PN {
    locktype lock:LOCKSIZE;
    proofnrtype proofnr;
    proofnrtype disproofnr;

    void store(locktype lock);
  };
  #pragma pack()

  PN *pnt;
  
  PNTree();
  ~PNTree();
  void clear();
  void store(bitboard bb, score sc, int work);
  struct Stats {
    uint64_t typecnt[NSCORES];
    uint64_t total;

    Stats(const Trans &tt);
  };
};

class History {
  int min(int x, int y) { return x<y ? x : y; }
  int max(int x, int y) { return x>y ? x : y; }

public:
  int hist[SIZE1];

  History();
  void clear();
  inline int ordermoves(int *av, int i) {
    int bi = --i, hval = hist[av[bi]];
    while (i--) {
      int v = hist[av[i]];
      if (v >= hval) {
        hval = v;
        bi = i;
      }
    }
    for (i = av[bi]; bi; bi--)
      av[bi] = av[bi-1];
    return av[0] = i;
  }
  inline void bestmove(int *av, int besti) {
    if (besti > 0) {
      for (int i = 0; i < besti; i++)
        hist[av[i]]--; // punish bad histories
      hist[av[besti]] += besti;
    }
  }
};

#pragma pack(2)
struct Result {
  score sc : 3;
  unsigned work : 6;
  unsigned best : 3;

  Result();
  Result(score s, int w, int b);
};
#pragma pack()

class Book {
  typedef std::map<bitboard,Result> BookMap;
  const char *bookfile;
  BookMap bookmap;
  int loadsize;
  int bd;

public:
  int ndupes;
  Book(Trans &tt, const char *filename);
  void store(Game *game, score sc, int work, int bestmove);
  int loaded() const;
  int added() const;
  Result find(Game *game);
  void bopen();
  void bclose();
private:
  bool _store(bitboard hsh, Result rslt);
  static bitboard hash(Game *game);
};

#ifndef BOOKWORK 
#define BOOKWORK 24 // report on best move and value for this much work
#endif

#ifndef BRUTEPLY 
#define BRUTEPLY 0     // additional plies to be searched full-width
#endif

class Window; 

class Search {
  Window *parent;
  Game *game;
  History hist[2];

public:
  Trans tt;
  Book book;
  uint64_t nodes, msecs;
  int work;

  Search(Window *win, Game &g, const char *bookfile);
  uint64_t millisecs();
  score ab(int alpha, int beta);
  void clear();
  score solve();
  score dab(int depth, int alpha, int beta);
};
