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

#include "Window.h"

void Trans::Hashentry::store(locktype lock, score sc, int work) {
      if (biglock == lock || work >= bigwork) {
        biglock = lock;
        bigscore = sc;
        bigwork = work;
      } else {
        newlock = lock;
        newscore = sc;
      }
    }

  Trans::Trans() {
    assert(ht = (Hashentry *)calloc(TRANSIZE, sizeof(Hashentry)));
  }
  Trans::~Trans() {
    free(ht);
  }
  void Trans::clear() {
    for (int i=0; i<TRANSIZE; i++) {
      ht[i].biglock = ht[i].newlock = 0;
      ht[i].bigwork = 0;
      ht[i].bigscore = ht[i].newscore = UNKNOWN;
    }
  }
  void Trans::store(bitboard bb, score sc, int work) {
    locktype lock = (locktype)(SIZE1>LOCKSIZE ? bb >> (SIZE1-LOCKSIZE) : bb);
    ht[(uint32_t)(bb % TRANSIZE)].store(lock, sc, work);
  }

    Trans::Stats::Stats(const Trans &tt) : total(0) {
    for (int i=0; i<NSCORES; i++)
      typecnt[i] = 0;
    for (int i=0; i<TRANSIZE; i++) {
      Hashentry he = tt.ht[i];
      if (he.biglock != 0)
        typecnt[he.bigscore]++;
      if (he.newlock != 0)
        typecnt[he.newscore]++;
    }
    for (int i=LOSS; i<=WIN; i++)
      total += typecnt[i];
    }

  uint64_t Hash::nhashed = 0;

  Hash::Hash(Trans &tt, const Game *game) {
    bitboard htemp = game->positioncode();
    if (game->nplies < SYMMREC) { // try symmetry recognition by reversing columns
      bitboard htemp2 = 0;
      for (bitboard htmp=htemp; htmp!=0; htmp>>=HEIGHT1)
        htemp2 = htemp2<<HEIGHT1 | (htmp & COL1);
      if (htemp2 < htemp)
        htemp = htemp2;
    }
    lock = (locktype)(SIZE1>LOCKSIZE ? htemp >> (SIZE1-LOCKSIZE) : htemp);
    he = &tt.ht[(uint32_t)(htemp % TRANSIZE)];
    oldhashed = nhashed;
  }
  score Hash::transpose() {
    return he->biglock == lock ? he->bigscore : he->newlock == lock ? he->newscore : UNKNOWN;
  }
  int Hash::store(score sc) {
    // work = log #positions stored
    int work = 0;
    for (uint64_t poscnt = nhashed++ - oldhashed; (poscnt>>=1) != 0; work++) ;
    he->store(lock, sc, work);
    return work;
  }

  History::History() {
    clear();
  }
  void History::clear() {
    for (int i=0; i<(WIDTH+1)/2; i++)
      for (int h=0; h<HEIGHT1/2; h++)
        hist[HEIGHT1*i+h] = hist[HEIGHT1*(WIDTH-1-i)+HEIGHT-1-h] =
        hist[HEIGHT1*i+HEIGHT-1-h] = hist[HEIGHT1*(WIDTH-1-i)+h] =
         4+min(3,i) + max(-1,min(3,h)-max(0,3-i)) + min(3,min(i,h)) + min(3,h);
  }

  Result::Result() : sc(UNKNOWN), work(0), best(0) { }
  Result::Result(score s, int w, int b) : sc(s), work(w), best(b) { }

  Book::Book(Trans &tt, const char *filename): bookfile(filename), ndupes(0) {
    bd = open(bookfile, O_RDONLY);
    assert(bd >= 0);
    bitboard bb = 0L;
    Result rslt;
    while (read(bd, &bb, BBYTES) == BBYTES &&
           read(bd, &rslt, sizeof(short)) == sizeof(short))  {
      if (!(rslt.sc > UNKNOWN && rslt.sc < LOSSWIN)) {
        ndupes += 1000000;
        continue;
      }
      if (_store(bb, rslt))
        ndupes++;
      tt.store(bb, rslt.sc, rslt.work);
    }
    close(bd);
    loadsize = bookmap.size();
  }
  void Book::store(Game *game, score sc, int work, int bestmove) {
    assert(sc > UNKNOWN && sc < LOSSWIN);
    bitboard bb = hash(game);
    Result rslt(sc, work, bestmove);
    _store(bb, rslt);
    assert(bd >= 0);
    assert(write(bd, &bb, BBYTES) == BBYTES);
    assert(write(bd, &rslt, sizeof(short)) == sizeof(short));
  }
  int Book::loaded() const {
    return loadsize;
  }
  int Book::added() const {
    return bookmap.size() - loadsize;
  }
  Result Book::find(Game *game) {
    static Result nada;
    bitboard hsh = hash(game);
    BookMap::const_iterator it = bookmap.find(hsh);
    bool found = it != bookmap.end();
    return found ? it->second : nada;
  }
  void Book::bopen() {
    bd = open(bookfile, O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
    assert(bd >= 0);
  }
  void Book::bclose() {
    close(bd);
  }
  bool Book::_store(bitboard hsh, Result rslt) {
    BookMap::iterator it = bookmap.find(hsh);
    bool found = it != bookmap.end();
    if (found) {
      if (it->second.sc != rslt.sc) {
        if (rslt.sc & 1) { // exact
          assert(rslt.sc == it->second.sc-1 || rslt.sc == it->second.sc+1);
          it->second = rslt;
        } else if (it->second.sc & 1) {
          assert(rslt.sc == it->second.sc-1 || rslt.sc == it->second.sc+1);
        } else {
          assert((rslt.sc^it->second.sc) == (DRAWLOSS^DRAWWIN));
          it->second.sc = DRAW;
        }
      }
    } else {
      bookmap[hsh] = rslt;
    }
    return found;
  }
  bitboard Book::hash(Game *game) {
    bitboard htemp = game->positioncode(), htemp2 = 0;
    for (bitboard htmp=htemp; htmp!=0; htmp>>=HEIGHT1)
      htemp2 = htemp2<<HEIGHT1 | (htmp & COL1);
    return htemp2 < htemp ? htemp2 : htemp;
  }

  Search::Search(Window *win, Game &g, const char *bookfile) : parent(win), game(&g), tt(), book(tt, bookfile) {
  }
  uint64_t Search::millisecs() {
    struct rusage foo;
    int rc = getrusage(RUSAGE_SELF,&foo);
    assert(rc==0);
    return (uint64_t)(foo.ru_utime.tv_sec * 1000 + foo.ru_utime.tv_usec / 1000);
  }
  score Search::ab(int alpha, int beta) {
    nodes++;
    if (game->nplies == SIZE-1) // one move left
      return DRAW; // by assumption, player to move can't win
    int side = game->nplies & 1;
    int otherside = side ^ 1;
    bitboard other = game->color[otherside];
    int av[WIDTH],nav;
    for (int i = nav = 0; i < WIDTH; i++) {
      int hi = game->hight[i];
      bitboard newbrd = other | ((bitboard)1 << hi); // check opponent move
      if (!game->islegal(newbrd)) 
        continue;
      int winontop = game->islegalhaswon(other | ((bitboard)2 << hi));
      if (game->haswon(newbrd)) { // immediate threat
        if (winontop) // can't stop double threat
          return LOSS;
        nav = 0; // forced move
        av[nav++] = hi;
        while (++i < WIDTH)
          if (game->islegalhaswon(other | ((bitboard)1 << game->hight[i])))
            return LOSS;
        break;
      }
      if (!winontop)
        av[nav++] = hi;
    }
    if (nav == 0)
      return LOSS;
    if (game->nplies == SIZE-2) // two moves left
      return DRAW; // opponent has no win either
    if (nav == 1) {
      game->makemove(av[0] / HEIGHT1);
      score sc = (score)(LOSSWIN - ab(LOSSWIN-beta,LOSSWIN-alpha));
      game->backmove();
      return sc;
    }
    Hash hash(tt, game);
    score ttscore = hash.transpose();
    if (ttscore != UNKNOWN) {
      if (ttscore == DRAWLOSS) {
        if ((beta = DRAW) <= alpha)
          return ttscore;
      } else if (ttscore == DRAWWIN) {
        if ((alpha = DRAW) >= beta)
          return ttscore;
      } else return ttscore; // exact score
    }
#if HEIGHT1&1
    if (!side) { // try evens strategy for 2nd player opponent
      int xevens = game->xevens();
      if (xevens >= 0) { // strategy works
        if (xevens == 1)
          return LOSS;
        if ((beta = DRAW) <= alpha)
          return DRAWLOSS;
      }
    }
#endif
    uint64_t bestdhashed = 0;
    int besti = 0;
    score sc = LOSS;
    for (int i = 0; i < nav; i++) {
      uint64_t mvoldhashed = Hash::nhashed;
      game->makemove(hist[side].ordermoves(av+i, nav-i) / HEIGHT1);
      score val = (score)(LOSSWIN - ab(LOSSWIN-beta,LOSSWIN-alpha));
      game->backmove();
      uint64_t dhashed = Hash::nhashed - mvoldhashed;
      if (val > sc) {
        besti = i;
        bestdhashed = dhashed;
        if ((sc=val) > alpha && game->nplies >= BRUTEPLY && (alpha=val) >= beta) {
          if (sc == DRAW && i < nav-1)
            sc = DRAWWIN;
          hist[side].bestmove(av, besti);
          break;
        }
      } else if (val == sc && dhashed > bestdhashed) {
        besti = i;
        bestdhashed = dhashed;
      }
    }
    if (sc == LOSSWIN-ttscore) // combine < and >
      sc = DRAW;
    work = hash.store(sc);
    if (work >= BOOKWORK) {
      book.store(game, sc, work, 1+av[besti]);
      parent->showgamemoves();
    }
    return sc;
  }
  void Search::clear() {
    tt.clear();
    hist[0].clear();
    hist[1].clear();
  }
  score Search::solve() {
    nodes = 0;
    msecs = millisecs();
    score sc = dab(8, LOSS, WIN);
    if (sc == UNKNOWN) {
      book.bopen();
      sc = ab(LOSS, WIN);
      book.bclose();
    }
    msecs = 1L + millisecs() - msecs; // prevent division by 0
    return sc;
  }
  score Search::dab(int depth, int alpha, int beta) {
    nodes++;
    if (game->haswon(game->color[1-(game->nplies&1)]))
      return LOSS;
    if (game->nplies == SIZE) // no move left
      return DRAW;
    Result rslt = book.find(game);
    score ttscore = rslt.sc;
    if (ttscore == UNKNOWN) {
      Hash hash(tt, game);
      ttscore = hash.transpose();
    } else work = rslt.work;
    if (ttscore != UNKNOWN) {
      if (ttscore == DRAWLOSS) {
        if ((beta = DRAW) <= alpha)
          return ttscore;
      } else if (ttscore == DRAWWIN) {
        if ((alpha = DRAW) >= beta)
          return ttscore;
      } else return ttscore; // exact score
    }
    if (depth == 0)
      return UNKNOWN;
    int side = game->nplies & 1;
    bitboard me = game->color[side];
    score v, sc = LOSS;
    bool unknown = false;
    for (int i = 0; i < WIDTH; i++) {
      int hi = game->hight[i];
      bitboard newbrd = me | ((bitboard)1 << hi);
      if (!game->islegal(newbrd)) 
        continue;
      game->makemove(i);
      score val = (score)(LOSSWIN - (v=dab(depth-1,LOSSWIN-beta,LOSSWIN-alpha)));
      game->backmove();
      if (v == UNKNOWN) {
        unknown = true;
        continue;
      }
      if (val > sc) {
        if ((sc=val) > alpha && (alpha=val) >= beta) {
          if (sc == DRAW && i < WIDTH-1)
            sc = DRAWWIN;
          break;
        }
      }
    }
    if (unknown && sc <= DRAW)
      sc = UNKNOWN;
    else if (sc == LOSSWIN-ttscore) // combine < and >
      sc = DRAW;
    return sc;
  }
