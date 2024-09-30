//
// Created by Dan Evans on 1/1/24.
//
#include "cs_basic.h"
#include "cs_block.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"

#include <cstring>

namespace cs {
  using OptionsLocal = struct {
    Symbol MYSY;
  };
  extern auto HasIncludeFlag() -> bool;
  extern auto RESULTTYPE(Types A, Types B) -> Types;
  extern auto SUMRESULTTYPE(Types A, Types B) -> Types;
  extern auto COMPATIBLE(Item X, Item Y) -> bool;
  extern auto type_compatible(Item x, Item y) -> bool;
  extern auto ARRAY_COMPATIBLE(Item X, Item Y) -> bool;
  extern auto PNT_COMPATIBLE(Item X, Item Y) -> bool;
  extern void const_declaration(BlockLocal*);
  extern void type_declaration(BlockLocal*);
  extern void var_declaration(BlockLocal*);
  extern auto include_directive() -> bool;
  extern void statement(BlockLocal*, SymbolSet &);
  extern void CHANELEMENT(Item &);
  extern void enter_channel();
  extern void C_PNTCHANTYP(BlockLocal*, Types &, int64_t &, int64_t &);
  extern void C_ARRATYP(
    BlockLocal*,
    int64_t &,
    int64_t &,
    bool FIRSTINDEX,
    Types,
    int64_t,
    int64_t
  );
  extern void TYPF(
    BlockLocal* block_local,
    SymbolSet FSYS,
    Types &types,
    int64_t &RF,
    int64_t &SZ
  );
  void FACTOR(BasicLocal*, SymbolSet &, Item &);
  void expression(BlockLocal* /*bl*/, SymbolSet & /*FSYS*/, Item & /*X*/);
  void PARAMETERLIST(BlockLocal*);
  void ASSIGNEXPRESSION(BasicLocal* /*bx*/, SymbolSet & /*FSYS*/, Item & /*X*/);
  static const int NWORDS = 9;
  // static int F;  // decl in BASICEXPRESSION, set in FACTOR

  void enter_block() {
    if (btab_index == BMAX)
      fatal(2);
    else {
      btab_index++;
      BTAB[btab_index].LAST = 0;
      BTAB[btab_index].LASTPAR = 0;
    }
  }

  void skip(SymbolSet FSYS, int num) {
    error(num);
    while (symbol != Symbol::EOFSY && !FSYS[static_cast<int>(symbol)])
      in_symbol();
  }

  void test(SymbolSet &S1, SymbolSet &S2, int num) {
    if (!S1[static_cast<int>(symbol)]) {
      SymbolSet su;
      // std::set_union(S1.begin(), S1.end(), S2.begin(), S2.end(), su);
      su = S1 | S2;
      // SKIP(S1 + S2, N);
      skip(su, num);
    }
  }

  void TESTSEMICOLON(BlockLocal* block_local) {
    SymbolSet su;
    if (symbol == Symbol::SEMICOLON)
      in_symbol();
    else {
      error(14);
      if (symbol == Symbol::COMMA || symbol == Symbol::COLON)
        in_symbol();
    }
    su = declaration_set | keyword_set | assigners_set;
    su.set(static_cast<int>(Symbol::INCLUDESY), true);
    test(su, block_local->FSYS, 6);
  }

  void enter(BlockLocal* block_local, const ALFA &identifier, Objects objects) {
    // int ix;
    int J, L;
    if (tab_index == TMAX)
      fatal(1);
    else {
      // std::cout << ID << std::endl;
      TAB[0].name = identifier;
      if (objects == Objects::Component)
        J = block_local->RLAST;
      else
        J = BTAB[DISPLAY[block_local->LEVEL]].LAST;
      L = J;
      while (strcmp(TAB[J].name.c_str(), identifier.c_str()) != 0)
        J = TAB[J].link;
      if (J != 0) {
        if ((TAB[J].object == Objects::Function ||
             TAB[J].object == Objects::Procedure) &&
            TAB[J].address == 0)
          proto_index = J;
        else
          error(1);
      } else {
        tab_index++;
        TAB[tab_index].name = identifier;
        TAB[tab_index].link = L;
        TAB[tab_index].object = objects;
        TAB[tab_index].types = Types::NOTYP;
        TAB[tab_index].FORLEV = 0;
        TAB[tab_index].reference = 0;
        TAB[tab_index].LEV = block_local->LEVEL;
        TAB[tab_index].address = 0;
        if (objects == Objects::Component)
          block_local->RLAST = tab_index;
        else
          BTAB[DISPLAY[block_local->LEVEL]].LAST = tab_index;
      }
    }
  }

  auto loc(BlockLocal* block, const ALFA &ID) -> int64_t {
    int I, J;
    bool FOUND;
    TAB[0].name = ID;

    I = block->NUMWITH;
    FOUND = false;
    while (I > 0 && !FOUND) {
      J = WITH_TAB[I];
      while (strcmp(TAB[J].name.c_str(), ID.c_str()) != 0)
        J = TAB[J].link;
      if (J != 0) {
        FOUND = true;
        emit(1, block->LEVEL, block->DX + I - 1);
        emit(24, TAB[J].address);
        emit(15);
      }
      I = I - 1;
    }
    if (!FOUND) {
      I = block->LEVEL;
      do {
        J = BTAB[DISPLAY[I]].LAST;
        while (strcmp(TAB[J].name.c_str(), ID.c_str()) != 0)
          J = TAB[J].link;
        I = I - 1;
      } while (I >= 0 && J == 0);
    }
    //        fprintf(STANDARD_OUTPUT, "lookup %s\n", ID);
    //        if (J != 0 && strcmp(ID, "SIZEOF        ") == 0)
    //            fprintf(STANDARD_OUTPUT, "sizeof %d\n", J);
    if (J == 0 && block->UNDEFMSGFLAG)
      error(0);
    return J;
  }

  void ENTERVARIABLE(BlockLocal* bl, Objects KIND) {
    if (symbol == Symbol::IDENT) {
      enter(bl, ID, KIND);
      in_symbol();
    } else
      error(2);
  }

  void constant(BlockLocal* bl, SymbolSet &FSYS, CONREC &C) {
    CONREC NEXTC = {Types::NOTYP, 0l};
    C.TP = Types::NOTYP;
    C.I = 0;
    test(base_set, FSYS, 50);
    if (base_set[static_cast<int>(symbol)]) {
      if (symbol == Symbol::CHARCON) {
        C.TP = Types::CHARS;
        C.I = INUM;
        in_symbol();
      } else {
        int sign = 1;
        if (symbol == Symbol::PLUS || symbol == Symbol::MINUS) {
          if (symbol == Symbol::MINUS)
            sign = -1;
          in_symbol();
        }
        if (symbol == Symbol::IDENT) {
          int X = loc(bl, ID);
          if (X != 0) {
            if (TAB[X].object != Objects::Constant)
              error(25);
            else {
              C.TP = TAB[X].types;
              if (C.TP == Types::REALS) {
                C.I = CPNT;
                CONTABLE[CPNT] = sign * CONTABLE[TAB[X].address];
                if (CPNT >= RCMAX)
                  fatal(10);
                else
                  CPNT = CPNT + 1;
              } else
                C.I = sign * TAB[X].address;
            }
          }
          in_symbol();
        } else if (symbol == Symbol::INTCON) {
          C.TP = Types::INTS;
          C.I = sign * INUM;
          in_symbol();
        } else if (symbol == Symbol::REALCON) {
          C.TP = Types::REALS;
          CONTABLE[CPNT] = sign * RNUM;
          C.I = CPNT;
          if (CPNT >= RCMAX)
            fatal(10);
          else
            CPNT = CPNT + 1;
          in_symbol();
        } else
          skip(FSYS, 50);
        if (C.TP == Types::INTS &&
            (symbol == Symbol::PLUS || symbol == Symbol::MINUS ||
             symbol == Symbol::TIMES)) {
          Symbol OPSYM = symbol;
          in_symbol();
          constant(bl, FSYS, NEXTC);
          if (NEXTC.TP != Types::INTS)
            error(25);
          else {
            switch (OPSYM) {
              case Symbol::PLUS: C.I = C.I + NEXTC.I;
                break;
              case Symbol::MINUS: C.I = C.I - NEXTC.I;
                break;
              case Symbol::TIMES: C.I = C.I * NEXTC.I;
                break;
              default: break;
            }
          }
        }
      }
      SymbolSet su = 0;
      test(FSYS, su, 6);
    }
  }

  void IDENTIFY(OptionsLocal* ol, const ALFA* WORD, const Symbol* SYM) {
    int64_t i = 1;
    bool found = false;
    do {
      if (strcmp(WORD[i].c_str(), ID.c_str()) == 0)
        found = true;
      else
        i = i + 1;
    } while (!found && i <= NWORDS);
    if (i <= NWORDS)
      ol->MYSY = SYM[i];
    else
      ol->MYSY = Symbol::ERRSY;
  }

  void OPTIONS(BlockLocal* bl) {
    ALFA WORD[NWORDS + 1];
    Symbol SYM[NWORDS + 1];
    CONREC C = {Types::NOTYP, 0l};
    int64_t J, K;
    OptionsLocal ol;
    SymbolSet su;
    ol.MYSY = Symbol::INTCON; // initial value only
    topology = Symbol::SHAREDSY;
    TOPDIM = PMAX;
    if (symbol == Symbol::IDENT && strcmp(ID.c_str(), "ARCHITECTURE  ") == 0) {
      WORD[1] = "SHARED        ";
      SYM[1] = Symbol::SHAREDSY;
      WORD[2] = "FULLCONNECT   ";
      SYM[2] = Symbol::FULLCONNSY;
      WORD[3] = "HYPERCUBE     ";
      SYM[3] = Symbol::HYPERCUBESY;
      WORD[4] = "LINE          ";
      SYM[4] = Symbol::LINESY;
      WORD[5] = "MESH2         ";
      SYM[5] = Symbol::MESH2SY;
      WORD[6] = "MESH3         ";
      SYM[6] = Symbol::MESH3SY;
      WORD[7] = "RING          ";
      SYM[7] = Symbol::RINGSY;
      WORD[8] = "TORUS         ";
      SYM[8] = Symbol::TORUSSY;
      WORD[9] = "CLUSTER       ";
      SYM[9] = Symbol::CLUSTERSY;
      in_symbol();
      if (symbol == Symbol::IDENT) {
        IDENTIFY(&ol, WORD, SYM);
        if (ol.MYSY != Symbol::ERRSY) {
          in_symbol();
          topology = ol.MYSY;
          if (symbol == Symbol::LPARENT) {
            in_symbol();
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            su.set(static_cast<int>(Symbol::COMMA), true);
            su.set(static_cast<int>(Symbol::RPARENT), true);
            su.set(static_cast<int>(Symbol::IDENT), true);
            constant(bl, su, C);
            if (C.TP == Types::INTS && C.I >= 0)
              TOPDIM = C.I;
            else
              error(43);
            if (symbol == Symbol::RPARENT)
              in_symbol();
            else
              error(4);
          } else {
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::RPARENT), true);
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            su.set(static_cast<int>(Symbol::IDENT), true);
            skip(su, 9);
          }
        } else {
          in_symbol();
          su = 0;
          su.set(static_cast<int>(Symbol::SEMICOLON), true);
          skip(su | declaration_set, 42);
        }
        TESTSEMICOLON(bl);
      } else
        skip(declaration_set, 42);
    }
    switch (topology) {
      case Symbol::SHAREDSY:
      case Symbol::FULLCONNSY:
      case Symbol::CLUSTERSY: highest_processor = TOPDIM - 1;
        break;
      case Symbol::HYPERCUBESY: K = 1;
        for (J = 1; J <= TOPDIM; J++)
          K = K * 2;
        highest_processor = K - 1;
        break;
      case Symbol::LINESY:
      case Symbol::RINGSY: highest_processor = TOPDIM - 1;
        break;
      case Symbol::MESH2SY:
      case Symbol::TORUSSY: highest_processor = TOPDIM * TOPDIM - 1;
        break;
      case Symbol::MESH3SY: highest_processor = TOPDIM * TOPDIM * TOPDIM - 1;
        break;
      default: break;
    }
    if (mpi_mode && topology == Symbol::SHAREDSY)
      error(144);
  }

  void LOADVAL(Item &X) {
    if (standard_set.test(static_cast<int>(X.types)) || X.types == Types::PNTS
        ||
        X.types == Types::LOCKS)
      emit(34);
    if (X.types == Types::CHANS)
      error(143);
    X.is_address = false;
  }

  void basic_expression(BlockLocal* bl, SymbolSet FSYS, Item &X) {
    // int F;
    // ITEM Y;
    // SYMBOL OP;
    BasicLocal bx;
    bx.factor = 0;
    bx.item = {Types::NOTYP, 0, 0, false};
    bx.operation = Symbol::INTCON; // initial value only, no meaning
    bx.block_local = bl;
    SymbolSet su = FSYS;
    ASSIGNEXPRESSION(&bx, su, X);
  }

  void expression(BlockLocal* bl, SymbolSet &FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    bool ADDR_REQUIRED;
    SymbolSet NXT;
    int64_t RF, SZ;
    Types TP;
    if (symbol == Symbol::ADDRSY) {
      ADDR_REQUIRED = true;
      in_symbol();
    } else
      ADDR_REQUIRED = false;
    if (symbol == Symbol::NEWSY) {
      in_symbol();
      NXT = FSYS;
      NXT.set(static_cast<int>(Symbol::TIMES), true);
      NXT.set(static_cast<int>(Symbol::CHANSY), true);
      NXT.set(static_cast<int>(Symbol::LBRACK), true);
      TYPF(bl, NXT, TP, RF, SZ);
      C_PNTCHANTYP(bl, TP, RF, SZ);
      enter_channel();
      CTAB[ctab_index].ELTYP = X.types;
      CTAB[ctab_index].ELREF = X.reference;
      CTAB[ctab_index].ELSIZE = (int)X.size;
      X.types = Types::PNTS;
      X.size = 1;
      X.reference = ctab_index;
      emit(99, SZ);
    } else {
      basic_expression(bl, FSYS, X);
      if (ADDR_REQUIRED) {
        if (!X.is_address)
          error(115);
        else {
          enter_channel();
          CTAB[ctab_index].ELTYP = X.types;
          CTAB[ctab_index].ELREF = X.reference;
          CTAB[ctab_index].ELSIZE = (int)X.size;
          X.types = Types::PNTS;
          X.size = 1;
          X.reference = ctab_index;
        }
      } else {
        if (X.is_address)
          LOADVAL(X);
      }
    }
    X.is_address = false;
  }

  void TERM(BasicLocal* bx, SymbolSet &FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    SymbolSet NXT;
    int64_t A, B;
    SymbolSet su, sv;
    su = 0;
    su.set(static_cast<int>(Symbol::TIMES), true);
    su.set(static_cast<int>(Symbol::IMOD), true);
    su.set(static_cast<int>(Symbol::RDIV), true);
    sv = su | FSYS;
    FACTOR(bx, sv, X);
    if (su.test(static_cast<int>(symbol)) && X.is_address)
      LOADVAL(X);
    while (su.test(static_cast<int>(symbol))) {
      OP = symbol;
      in_symbol();
      FACTOR(bx, sv, Y);
      if (Y.is_address)
        LOADVAL(Y);
      if (OP == Symbol::TIMES) {
        X.types = RESULTTYPE(X.types, Y.types);
        if (X.types == Types::INTS || X.types == Types::REALS)
          emit(57);
      } else if (OP == Symbol::RDIV) {
        if (X.types == Types::INTS && Y.types == Types::INTS)
          emit(58);
        else {
          X.types = RESULTTYPE(X.types, Y.types);
          if (X.types != Types::NOTYP)
            X.types = Types::REALS;
          emit(88);
        }
      } else {
        if (X.types == Types::INTS && Y.types == Types::INTS)
          emit(59);
        else {
          if (X.types != Types::NOTYP && Y.types != Types::NOTYP)
            error(34);
          X.types = Types::NOTYP;
        }
      }
    }
  }

  void SIMPLEEXPRESSION(BasicLocal* bx, SymbolSet &FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    int64_t I = 0;
    int64_t J = 0;
    SymbolSet su, sv;
    su = 0;
    su.set(static_cast<int>(Symbol::PLUS), true);
    su.set(static_cast<int>(Symbol::MINUS), true);
    sv = FSYS | su;
    TERM(bx, sv, X);
    if ((symbol == Symbol::PLUS || symbol == Symbol::MINUS) && X.is_address)
      LOADVAL(X);
    while (symbol == Symbol::PLUS || symbol == Symbol::MINUS) {
      OP = symbol;
      in_symbol();
      TERM(bx, sv, Y);
      if (Y.is_address)
        LOADVAL(Y);
      if (X.types == Types::PNTS && Y.types == Types::INTS) {
        if (OP == Symbol::MINUS)
          emit(36);
        emit(110, CTAB[X.reference].ELSIZE, 0);
      }
      if (X.types == Types::INTS && Y.types == Types::PNTS) {
        if (OP == Symbol::MINUS) {
          error(113);
          X.types = Types::NOTYP;
        } else
          emit(110, CTAB[Y.reference].ELSIZE, 1);
      }
      if (X.types == Types::PNTS && Y.types == Types::PNTS &&
          OP == Symbol::MINUS) {
        X.types = Types::INTS;
        emit(53);
      } else {
        X.types = SUMRESULTTYPE(X.types, Y.types);
        if (X.types == Types::INTS || X.types == Types::REALS ||
            X.types == Types::CHARS) {
          if (OP == Symbol::PLUS)
            emit(52);
          else
            emit(53);
        }
      }
    }
  }

  void BOOLEXPRESSION(BasicLocal* bx, SymbolSet &FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    SymbolSet su, sv, sz;
    su = 0;
    su.set(static_cast<int>(Symbol::EQL), true);
    su.set(static_cast<int>(Symbol::NEQ), true);
    su.set(static_cast<int>(Symbol::LSS), true);
    su.set(static_cast<int>(Symbol::LEQ), true);
    su.set(static_cast<int>(Symbol::GTR), true);
    su.set(static_cast<int>(Symbol::GEQ), true);
    sv = 0;
    sv[static_cast<int>(Types::NOTYP)] = true;
    sv[static_cast<int>(Types::BOOLS)] = true;
    sv[static_cast<int>(Types::INTS)] = true;
    sv[static_cast<int>(Types::REALS)] = true;
    sv[static_cast<int>(Types::CHARS)] = true;
    sv[static_cast<int>(Types::PNTS)] = true;
    sz = su | FSYS;
    SIMPLEEXPRESSION(bx, sz, X);
    if (su.test(static_cast<int>(symbol))) {
      if (X.is_address)
        LOADVAL(X);
      OP = symbol;
      in_symbol();
      SIMPLEEXPRESSION(bx, FSYS, Y);
      if (Y.is_address)
        LOADVAL(Y);
      if (sv[static_cast<int>(X.types)]) {
        if (X.types == Y.types ||
            (X.types == Types::INTS && Y.types == Types::REALS) ||
            (X.types == Types::REALS && Y.types == Types::INTS)) {
          switch (OP) {
            case Symbol::EQL: emit(45);
              break;
            case Symbol::NEQ: emit(46);
              break;
            case Symbol::LSS: emit(47);
              break;
            case Symbol::LEQ: emit(48);
              break;
            case Symbol::GTR: emit(49);
              break;
            case Symbol::GEQ: emit(50);
              break;
            default: break;
          }
        } else
          error(35);
      }
      X.types = Types::BOOLS;
    }
  }

  void ANDEXPRESSION(BasicLocal* bx, SymbolSet &FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    SymbolSet su;
    su = FSYS;
    su.set(static_cast<int>(Symbol::ANDSY), true);
    BOOLEXPRESSION(bx, su, X);
    if (symbol == Symbol::ANDSY && X.is_address)
      LOADVAL(X);
    while (symbol == Symbol::ANDSY) {
      OP = symbol;
      in_symbol();
      BOOLEXPRESSION(bx, su, Y);
      if (Y.is_address)
        LOADVAL(Y);
      if (X.types == Types::BOOLS && Y.types == Types::BOOLS)
        emit(56);
      else {
        if (X.types != Types::NOTYP && Y.types != Types::NOTYP)
          error(32);
        X.types = Types::NOTYP;
      }
    }
  }

  void OREXPRESSION(BasicLocal* bx, SymbolSet FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    SymbolSet su;
    su = FSYS;
    su.set(static_cast<int>(Symbol::ORSY), true);
    ANDEXPRESSION(bx, su, X);
    while (symbol == Symbol::ORSY) {
      if (X.is_address)
        LOADVAL(X);
      OP = symbol;
      in_symbol();
      ANDEXPRESSION(bx, su, Y);
      if (Y.is_address)
        LOADVAL(Y);
      if (X.types == Types::BOOLS && Y.types == Types::BOOLS)
        emit(51);
      else {
        if (X.types != Types::NOTYP && Y.types != Types::NOTYP)
          error(32);
        X.types = Types::NOTYP;
      }
    }
  }

  void COMPASSIGNEXP(BasicLocal* bx, SymbolSet &FSYS, Item &X) {
    Item Y = {Types::NOTYP, 0, 0, false};
    Item Z = {Types::NOTYP, 0, 0, false};
    Symbol OP;
    int SF;
    OP = symbol;
    SF = bx->factor;
    if (!X.is_address) {
      error(114);
      X.types = Types::NOTYP;
    }
    if (X.types == Types::CHANS) {
      error(114);
      X.types = Types::NOTYP;
    }
    emit(14);
    Z = X;
    Z.is_address = false;
    in_symbol();
    expression(bx->block_local, FSYS, Y);
    switch (OP) {
      case Symbol::PLUSEQ:
      case Symbol::MINUSEQ: if (Y.is_address)
          LOADVAL(Y);
        if (Z.types == Types::PNTS && Y.types == Types::INTS) {
          if (OP == Symbol::MINUSEQ)
            emit(36);
          emit(110, CTAB[Z.reference].ELSIZE, 0);
        }
        if (Z.types == Types::INTS && Y.types == Types::PNTS) {
          if (OP == Symbol::MINUSEQ) {
            error(113);
            Z.types = Types::NOTYP;
          } else
            emit(110, CTAB[Y.reference].ELSIZE, 1);
        }
        if (Z.types == Types::PNTS && Y.types == Types::PNTS &&
            OP == Symbol::MINUSEQ) {
          Z.types = Types::INTS;
          emit(53);
        } else {
          Z.types = SUMRESULTTYPE(Z.types, Y.types);
          if (Z.types == Types::INTS || Z.types == Types::REALS ||
              Z.types == Types::CHARS) {
            if (OP == Symbol::PLUSEQ)
              emit(52);
            else
              emit(53);
          }
        }
        break;
      case Symbol::TIMESEQ:
      case Symbol::RDIVEQ:
      case Symbol::IMODEQ: if (Y.is_address)
          LOADVAL(Y);
        if (OP == Symbol::TIMESEQ) {
          Z.types = RESULTTYPE(Z.types, Y.types);
          if (Z.types == Types::INTS || Z.types == Types::REALS)
            emit(57);
        } else if (OP == Symbol::RDIVEQ) {
          if (Z.types == Types::INTS && Y.types == Types::INTS)
            emit(58);
          else {
            Z.types = RESULTTYPE(Z.types, Y.types);
            if (Z.types != Types::NOTYP)
              Z.types = Types::REALS;
            emit(88);
          }
        } else {
          if (Z.types == Types::INTS && Y.types == Types::INTS)
            emit(59);
          else {
            if (Z.types != Types::NOTYP && Y.types != Types::NOTYP)
              error(34);
            Z.types = Types::NOTYP;
          }
        }
        break;
      default: break;
    }
    if (type_compatible(X, Z))
      error(46);
    else {
      if (X.types == Types::REALS && Z.types == Types::INTS)
        emit(91, SF);
      emit(38, SF);
    }
    X.is_address = false;
  }

  void ASSIGNEXPRESSION(BasicLocal* bx, SymbolSet &FSYS, Item &X) {
    Item Y;
    Symbol OP;
    int SF, SZ;
    SymbolSet su, sv;
    su = FSYS | comparators_set;
    su.set(static_cast<int>(Symbol::BECOMES), true);
    OREXPRESSION(bx, su, X);
    if (comparators_set.test(static_cast<int>(symbol)))
      COMPASSIGNEXP(bx, FSYS, X);
    else if (symbol == Symbol::BECOMES) {
      SF = bx->factor;
      if (!X.is_address) {
        error(114);
        X.types = Types::NOTYP;
      }
      if (X.types == Types::CHANS) {
        error(114);
        X.types = Types::NOTYP;
      }
      in_symbol();
      su = FSYS;
      su.set(static_cast<int>(Symbol::BECOMES), true);
      expression(bx->block_local, su, Y);
      if (type_compatible(X, Y))
        error(46);
      else {
        if (X.types == Types::REALS && Y.types == Types::INTS)
          emit(91, SF);
        else if (standard_set[static_cast<int>(X.types)] || X.types ==
                 Types::PNTS ||
                 X.types == Types::LOCKS)
          emit(38, SF);
        else {
          if (X.types == Types::ARRAYS)
            SZ = ATAB[X.reference].SIZE;
          else
            SZ = X.size;
          emit(23, SZ, SF);
        }
      }
      X.is_address = false;
    }
  }

  void STANDPROC(BlockLocal* bl, int N) {
    Item X, Y, Z;
    int SZ;
    SymbolSet su;
    switch (N) {
      case 3: // send
        if (symbol == Symbol::LPARENT) {
          in_symbol();
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::COMMA), true);
          su.set(static_cast<int>(Symbol::RPARENT), true);
          basic_expression(bl, su, X);
          if (!X.is_address || !(X.types == Types::CHANS)) {
            error(140);
            X.types = Types::NOTYP;
          } else {
            CHANELEMENT(X);
            if (symbol == Symbol::COMMA)
              in_symbol();
            else
              error(135);
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::RPARENT), true);
            expression(bl, su, Y);
            if (type_compatible(X, Y))
              error(141);
            else {
              if (X.types == Types::REALS && Y.types == Types::INTS)
                emit(92, 1);
              else if (standard_set[static_cast<int>(X.types)] ||
                       X.types == Types::PNTS)
                emit(66, 1);
              else {
                if (X.types == Types::ARRAYS)
                  SZ = ATAB[X.reference].SIZE;
                else
                  SZ = X.size;
                emit(98, SZ);
                emit(66, SZ);
              }
            }
          }
          if (symbol == Symbol::RPARENT)
            in_symbol();
          else
            error(4);
        } else
          error(9);
        break;
      case 4: // recv
        if (symbol == Symbol::LPARENT) {
          in_symbol();
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::COMMA), true);
          su.set(static_cast<int>(Symbol::RPARENT), true);
          basic_expression(bl, su, X);
          if (!X.is_address || X.types != Types::CHANS) {
            error(140);
            X.types = Types::NOTYP;
          } else {
            emit(71);
            CHANELEMENT(X);
            if (symbol == Symbol::COMMA)
              in_symbol();
            else
              error(135);
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::RPARENT), true);
            basic_expression(bl, su, Y);
            if (!Y.is_address) {
              error(142);
              Y.types = Types::NOTYP;
            }
            if (type_compatible(X, Y))
              error(141);
            else {
              emit(20);
              Z = X;
              X = Y;
              Y = Z;
              if (X.types == Types::REALS && Y.types == Types::INTS)
                emit(91, 0);
              else if (standard_set[static_cast<int>(X.types)] ||
                       X.types == Types::PNTS) {
                emit(38, 0);
                emit(111);
              } else {
                if (X.types == Types::ARRAYS)
                  SZ = ATAB[X.reference].SIZE;
                else
                  SZ = X.size;
                emit(23, SZ, 0);
                emit(112, X.size);
                emit(111);
              }
            }
          }
          if (symbol == Symbol::RPARENT)
            in_symbol();
          else
            error(4);
        } else
          error(9);
        break;
      case 6: // delete
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::SEMICOLON), true);
        basic_expression(bl, su, X);
        if (!X.is_address)
          error(120);
        else if (X.types == Types::PNTS)
          emit(100);
        else
          error(16);
        break;
      case 7: // lock
        if (symbol == Symbol::LPARENT) {
          in_symbol();
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::RPARENT), true);
          basic_expression(bl, su, X);
          if (!X.is_address)
            error(15);
          else {
            if (X.types == Types::LOCKS)
              emit(101);
            else
              error(15);
            if (symbol == Symbol::RPARENT)
              in_symbol();
            else
              error(4);
          }
        } else
          error(9);
        break;
      case 8: // unlock
        if (symbol == Symbol::LPARENT) {
          in_symbol();
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::RPARENT), true);
          basic_expression(bl, su, X);
          if (!X.is_address)
            error(15);
          else {
            if (X.types == Types::LOCKS)
              emit(102);
            else
              error(15);
            if (symbol == Symbol::RPARENT)
              in_symbol();
            else
              error(4);
          }
        } else
          error(9);
        break;
      case 9: // duration
        if (symbol == Symbol::LPARENT) {
          in_symbol();
          su = bl->FSYS;
          su.set(static_cast<int>(Symbol::RPARENT), true);
          expression(bl, su, X);
          if (X.types != Types::INTS)
            error(34);
          emit(93);
          if (symbol == Symbol::RPARENT)
            in_symbol();
          else
            error(4);
        } else
          error(9);
        break;
      case 10: // seqon
        emit(106);
        break;
      case 11: // seqoff
        emit(107);
        break;
    }
  }

  void call(BlockLocal* block, SymbolSet &FSYS, int idx) {
    // int K;
    if (strcmp(TAB[idx].name.c_str(), "MPI_INIT\t") == 0 && !mpi_mode)
      error(146);
    SymbolSet su = FSYS;
    su.set(static_cast<int>(Symbol::COMMA), true);
    su.set(static_cast<int>(Symbol::COLON), true);
    su.set(static_cast<int>(Symbol::RPARENT), true);
    emit(18, idx);
    const int lastp = BTAB[TAB[idx].reference].LASTPAR;
    int cp = lastp - BTAB[TAB[idx].reference].PARCNT;
    if (symbol == Symbol::LPARENT)
      in_symbol();
    else
      error(9);
    if (symbol != Symbol::RPARENT) {
      Item x{};
      Item y{};
      bool done = false;
      do {
        if (cp >= lastp)
          error(39);
        else {
          cp++;
          if (TAB[cp].normal) {
            expression(block, su, x);
            y.types = TAB[cp].types;
            y.reference = TAB[cp].reference;
            y.is_address = true;
            if (type_compatible(x, y)) {
              if (x.types == Types::ARRAYS && y.types != Types::PNTS)
                emit(22, ATAB[x.reference].SIZE);
              if (x.types == Types::RECS)
                emit(22, (int)x.size);
              if (y.types == Types::REALS && x.types == Types::INTS)
                emit(26);
            } else
              error(36);
          } else if (symbol != Symbol::IDENT)
            error(2);
          else {
            const SymbolSet sv = FSYS | su;
            basic_expression(block, sv, x);
            y.types = TAB[cp].types;
            y.reference = TAB[cp].reference;
            y.is_address = true;
            if (!x.is_address)
              error(116);
            else {
              switch (y.types) {
                case Types::CHANS: if (x.types != Types::CHANS)
                    error(116);
                  else if (!COMPATIBLE(y, x))
                    error(36);
                  break;
                case Types::ARRAYS: emit(86);
                  if (ATAB[y.reference].HIGH > 0) {
                    if (!ARRAY_COMPATIBLE(y, x))
                      error(36);
                  } else if (!PNT_COMPATIBLE(y, x))
                    error(36);
                  break;
                default: break;
              }
            }
          }
        }
        su = 0;
        su.set(static_cast<int>(Symbol::COMMA), true);
        su.set(static_cast<int>(Symbol::RPARENT), true);
        test(su, FSYS, 6);
        if (symbol == Symbol::COMMA)
          in_symbol();
        else
          done = true;
      } while (!done);
    }
    if (symbol == Symbol::RPARENT)
      in_symbol();
    else
      error(4);
    if (cp < lastp)
      error(39);
    if (block->CREATEFLAG) {
      emit(78, BTAB[TAB[idx].reference].PSIZE - BASESIZE);
      block->CREATEFLAG = false;
    }
    emit(19, idx, BTAB[TAB[idx].reference].PSIZE - 1);
    if (TAB[idx].LEV < block->LEVEL && TAB[idx].address >= 0)
      emit(3, TAB[idx].LEV, block->LEVEL);
  }

  void BLOCK(
    Interpreter* interp_local,
    SymbolSet FSYS,
    bool is_function,
    int level,
    int PRT
  ) {
    // fprintf(stdout, "BLOCK not yet implemented");
    //    int DX;
    //    int FLEVEL;   shadows global
    //    int NUMWITH;
    //    int MAXNUMWITH;
    //    bool UNDEFMSGFLAG;
    BlockLocal block_local;
    block_local.blkil = interp_local;
    block_local.FSYS = FSYS;
    block_local.ISFUN = is_function;
    block_local.LEVEL = level;
    block_local.PRT = PRT;
    /* parameters above */
    SymbolSet su, sv;
    block_local.DX = BASESIZE;
    block_local.FLEVEL = 0;
    if (interp_local->LEVEL > LMAX)
      fatal(5);
    if (block_local.LEVEL > 2)
      error(122);
    block_local.NUMWITH = 0;
    block_local.MAXNUMWITH = 0;
    block_local.UNDEFMSGFLAG = true;
    block_local.RDX = 0;
    block_local.RLAST = 0;
    block_local.PCNT = 0;
    block_local.X = 0;
    block_local.V = 0;
    // bl.F = 0;   BASICEXPRESSION
    block_local.CREATEFLAG = false;
    block_local.ISDECLARATION = false;
    enter_block();

    DISPLAY[block_local.LEVEL] = btab_index;
    block_local.PRB = btab_index;
    if (block_local.LEVEL == 1)
      TAB[PRT].types = Types::NOTYP;
    else {
      su = 0;
      su.set(static_cast<int>(Symbol::LPARENT), true);
      test(su, FSYS, 9);
    }

    TAB[PRT].reference = block_local.PRB;
    if (symbol == Symbol::LPARENT && block_local.LEVEL > 1)
      PARAMETERLIST(&block_local);

    BTAB[block_local.PRB].LASTPAR = tab_index;
    BTAB[block_local.PRB].PSIZE = block_local.DX;
    BTAB[block_local.PRB].PARCNT = block_local.PCNT;
    if (block_local.LEVEL == 1) {
      su = declaration_set;
      su.set(static_cast<int>(Symbol::INCLUDESY), true);
      test(su, FSYS, 102);
      OPTIONS(&block_local);
      do {
        if (symbol == Symbol::DEFINESY)
          const_declaration(&block_local);
        else if (symbol == Symbol::TYPESY)
          type_declaration(&block_local);
        else if (type_set.test(static_cast<int>(symbol)))
          var_declaration(&block_local);
        else if (symbol == Symbol::INCLUDESY)
          include_directive();
        su = declaration_set;
        su.set(static_cast<int>(Symbol::INCLUDESY), true);
        su.set(static_cast<int>(Symbol::EOFSY), true);
        test(su, FSYS, 6);
      } while (symbol != Symbol::EOFSY);
      TAB[PRT].address = line_count;
    } else {
      if (symbol != Symbol::SEMICOLON) {
        TAB[PRT].address = line_count;
        su = declaration_set | keyword_set | assigners_set;
        sv = FSYS;
        sv.set(static_cast<int>(Symbol::RSETBRACK), true);
        test(su, sv, 101);
        block_local.CREATEFLAG = false;
        while (declaration_set.test(static_cast<int>(symbol)) ||
               keyword_set.test(static_cast<int>(symbol)) ||
               assigners_set.test(static_cast<int>(symbol))) {
          if (symbol == Symbol::DEFINESY)
            const_declaration(&block_local);
          else if (symbol == Symbol::TYPESY)
            type_declaration(&block_local);
          else if (symbol == Symbol::STRUCTSY || symbol == Symbol::CONSTSY)
            var_declaration(&block_local);
          if (symbol == Symbol::IDENT) {
            block_local.X = loc(&block_local, ID);
            if (block_local.X != 0) {
              if (TAB[block_local.X].object == Objects::Type1)
                var_declaration(&block_local);
              else {
                su = FSYS;
                su.set(static_cast<int>(Symbol::SEMICOLON), true);
                su.set(static_cast<int>(Symbol::RSETBRACK), true);
                statement(&block_local, su);
              }
            } else
              in_symbol();
          } else {
            if (keyword_set.test(static_cast<int>(symbol)) ||
                assigners_set.test(static_cast<int>(symbol))) {
              su = FSYS;
              su.set(static_cast<int>(Symbol::SEMICOLON), true);
              su.set(static_cast<int>(Symbol::RSETBRACK), true);
              statement(&block_local, su);
            }
          }
          su = declaration_set | keyword_set | assigners_set;
          su.set(static_cast<int>(Symbol::RSETBRACK), true);
          test(su, FSYS, 6);
        }
      }
    }

    BTAB[block_local.PRB].VSIZE = block_local.DX;
    BTAB[block_local.PRB].VSIZE =
      BTAB[block_local.PRB].VSIZE + block_local.MAXNUMWITH;
    if (symbol_count == 1 && !include_directive())
      LOCATION[LINE_NUMBER] = line_count;
  }
} // namespace cs
