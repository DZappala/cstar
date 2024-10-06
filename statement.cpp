#include <cstring>

#include "cs_block.h"
#include "cs_compile.h"
#include "cs_errors.h"
#include "cs_global.h"
#include "cs_interpret.h"

namespace cs {
  extern void next_char();
  extern void var_declaration(BlockLocal*);
  extern void const_declaration(BlockLocal*);
  extern void type_declaration(BlockLocal*);
  extern auto loc(BlockLocal* block, const ALFA &ID) -> int64_t;
  extern auto skip(SymbolSet, int) -> void;
  extern auto test(SymbolSet &, SymbolSet &, int N) -> void;
  extern void call(BlockLocal*, SymbolSet &, int);
  extern void STANDPROC(BlockLocal*, int);
  extern void enter(
    BlockLocal* block_local,
    const ALFA &identifier,
    Objects objects
  );
  extern void enter_block();
  extern auto type_compatible(Item x, Item y) -> bool;
  extern void basic_expression(BlockLocal*, SymbolSet, Item &);
  extern void constant(BlockLocal*, SymbolSet &, CONREC &);

  auto expression(BlockLocal*, SymbolSet FSYS, Item &X) -> void;
  auto compound_statement(BlockLocal* /*bl*/) -> void;
  auto fix_breaks(int64_t LC1) -> void;
  auto fix_conts(int64_t LC1) -> void;
  auto switch_statement(BlockLocal* /*block_local*/) -> void;
  auto if_statement(BlockLocal* /*bl*/) -> void;
  auto do_statement(BlockLocal* /*bl*/) -> void;
  auto while_statement(BlockLocal* /*bl*/) -> void;
  auto for_statement(BlockLocal* /*bl*/) -> void;
  auto block_statement(BlockLocal* /*bl*/) -> void;
  auto forall_statement(BlockLocal* /*bl*/) -> void;
  auto fork_statement(BlockLocal* /*bl*/) -> void;
  auto join_statement(BlockLocal* /*bl*/) -> void;
  auto return_statement(BlockLocal* /*bl*/) -> void;
  auto break_statement(BlockLocal* /*bl*/) -> void;
  auto continue_statement() -> void;
  auto move_statement(BlockLocal* /*bl*/) -> void;
  auto input_statement(BlockLocal* /*bl*/) -> void;
  auto cout_methods(BlockLocal* /*bl*/) -> void;
  auto output_statement(BlockLocal* /*bl*/) -> void;

  static int CONTPNT; // range 0..LOOPMAX
  static std::array<int64_t, LOOPMAX + 1> CONTLOC;

  void statement(BlockLocal* block, SymbolSet &FSYS) {
    int64_t I = 0;
    Item X{};
    int64_t JUMPLC = 0;
    Symbol SYMSAV;
    SymbolSet su;
    SymbolSet sv;
    SYMSAV = symbol;
    // INSYMBOL();
    if (keyword_set.test(static_cast<int>(symbol)) ||
        assigners_set.test(static_cast<int>(symbol))) {
      if (symbol == Symbol::LSETBRACK) // BEGINSY ??
        compound_statement(block);
      else if (symbol == Symbol::SWITCHSY)
        switch_statement(block);
      else if (symbol == Symbol::IFSY)
        if_statement(block);
      else if (symbol == Symbol::DOSY)
        do_statement(block);
      else if (symbol == Symbol::WHILESY)
        while_statement(block);
      else if (symbol == Symbol::FORSY)
        for_statement(block);
      else if (symbol == Symbol::FORALLSY)
        forall_statement(block);
      else if (symbol == Symbol::FORKSY)
        fork_statement(block);
      else if (symbol == Symbol::JOINSY)
        join_statement(block);
      else if (symbol == Symbol::RETURNSY)
        return_statement(block);
      else if (symbol == Symbol::BREAKSY)
        break_statement(block);
      else if (symbol == Symbol::CONTSY)
        continue_statement();
      else if (symbol == Symbol::MOVESY)
        move_statement(block);
      else if (symbol == Symbol::CINSY)
        input_statement(block);
      else if (symbol == Symbol::COUTSY)
        output_statement(block);
      else if (symbol == Symbol::IDENT) {
        I = loc(block, ID);
        if (I != 0) {
          if (TAB[I].object == Objects::Procedure) {
            in_symbol();
            if (TAB[I].LEV != 0)
              call(block, FSYS, I);
            else
              STANDPROC(block, TAB[I].address);
          } else {
            expression(block, FSYS, X);
            if (X.size > 1)
              emit(112, X.size);
            emit(111);
          }
        } else
          in_symbol();
      } else if (symbol == Symbol::LPARENT || symbol == Symbol::INCREMENT ||
                 symbol == Symbol::DECREMENT || symbol == Symbol::TIMES) {
        expression(block, FSYS, X);
        emit(111);
      }
    }
    if (assigners_set.test(static_cast<int>(SYMSAV)) ||
        (SYMSAV == Symbol::DOSY || SYMSAV == Symbol::JOINSY ||
         SYMSAV == Symbol::RETURNSY || SYMSAV == Symbol::BREAKSY ||
         SYMSAV == Symbol::CONTSY || SYMSAV == Symbol::SEMICOLON ||
         SYMSAV == Symbol::MOVESY || SYMSAV == Symbol::CINSY ||
         SYMSAV == Symbol::COUTSY ||
         SYMSAV == Symbol::RSETBRACK)) // RSETBRACK added for while missing
    // statement DDE
    {
      if (symbol == Symbol::SEMICOLON)
        in_symbol();
      else
        error(14);
    }
    su = 0;
    sv = FSYS;
    sv.set(static_cast<int>(Symbol::ELSESY), true); // added to fix else DDE
    // TEST(FSYS, su, 14);
    test(sv, su, 14);
  }

  void compound_statement(BlockLocal* bl) {
    int64_t X;
    SymbolSet su, sv;
    in_symbol();
    su = declaration_set | keyword_set | assigners_set;
    // su.set(static_cast<int>(RSETBRACK), true);  ??
    sv = bl->FSYS;
    sv.set(static_cast<int>(Symbol::RSETBRACK), true);
    test(su, sv, 123);
    while (declaration_set.test(static_cast<int>(symbol)) ||
           keyword_set.test(static_cast<int>(symbol)) ||
           assigners_set.test(static_cast<int>(symbol))) {
      if (symbol == Symbol::DEFINESY)
        const_declaration(bl);
      if (symbol == Symbol::TYPESY)
        type_declaration(bl);
      if (symbol == Symbol::STRUCTSY)
        var_declaration(bl);
      if (symbol == Symbol::IDENT) {
        X = loc(bl, ID);
        if (X != 0) {
          if (TAB[X].object == Objects::Type1)
            var_declaration(bl);
          else {
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::RSETBRACK), true);
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            statement(bl, su);
          }
        } else
          in_symbol();
      } else if (keyword_set.test(static_cast<int>(symbol)) ||
                 assigners_set.test(static_cast<int>(symbol))) {
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::RSETBRACK), true);
        su.set(static_cast<int>(Symbol::SEMICOLON), true);
        statement(bl, su);
      }
      su = declaration_set | keyword_set | assigners_set;
      su.set(static_cast<int>(Symbol::RSETBRACK), true);
      test(su, bl->FSYS, 6);
    }
    if (symbol_count == 1)
      LOCATION[LINE_NUMBER] = line_count;
    if (symbol == Symbol::RSETBRACK)
      in_symbol();
    else
      error(104);
  }

  void fix_breaks(int64_t LC1) {
    int64_t LC2;
    while (BREAK_LOCATION[BREAK_POINT] > 0) {
      LC2 = CODE[BREAK_LOCATION[BREAK_POINT]].Y;
      CODE[BREAK_LOCATION[BREAK_POINT]].Y = LC1;
      BREAK_LOCATION[BREAK_POINT] = LC2;
    }
    BREAK_POINT = BREAK_POINT - 1;
  }

  void fix_conts(int64_t LC1) {
    int64_t LC2;
    while (CONTLOC[CONTPNT] > 0) {
      LC2 = CODE[CONTLOC[CONTPNT]].Y;
      CODE[CONTLOC[CONTPNT]].Y = LC1;
      CONTLOC[CONTPNT] = LC2;
    }
    CONTPNT = CONTPNT - 1;
  }

  void if_statement(BlockLocal* bl) {
    Item X = {Types::NOTYP, 0};
    int LC1, LC2;
    SymbolSet su;
    in_symbol();
    if (symbol == Symbol::LPARENT) {
      in_symbol();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      expression(bl, su, X);
      if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
            X.types == Types::NOTYP))
        error(17);
      LC1 = line_count;
      emit(11);
      if (LOCATION[LINE_NUMBER] == line_count - 1)
        LOCATION[LINE_NUMBER] = line_count;
      if (symbol == Symbol::RPARENT)
        in_symbol();
      else
        error(4);
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::ELSESY), true);
      statement(bl, su);
      if (symbol == Symbol::ELSESY) {
        LC2 = line_count;
        emit(10);
        if (symbol_count == 1)
          LOCATION[LINE_NUMBER] = line_count;
        CODE[LC1].Y = line_count;
        in_symbol();
        statement(bl, bl->FSYS);
        CODE[LC2].Y = line_count;
      } else
        CODE[LC1].Y = line_count;
    } else
      error(9);
  }

  void do_statement(BlockLocal* bl) {
    Item X;
    int LC1, LC2, LC3;
    LC1 = line_count;
    SymbolSet su;
    in_symbol();
    BREAK_POINT = BREAK_POINT + 1;
    BREAK_LOCATION[BREAK_POINT] = 0;
    CONTPNT = CONTPNT + 1;
    CONTLOC[CONTPNT] = 0;
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::SEMICOLON), true);
    su.set(static_cast<int>(Symbol::WHILESY), true);
    statement(bl, su);
    LC2 = line_count;
    if (symbol == Symbol::WHILESY) {
      if (symbol_count == 1)
        LOCATION[LINE_NUMBER] = line_count;
      in_symbol();
      if (symbol == Symbol::LPARENT)
        in_symbol();
      else
        error(9);
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      expression(bl, su, X);
      if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
            X.types == Types::NOTYP))
        error(17);
      if (symbol == Symbol::RPARENT)
        in_symbol();
      else
        error(4);
      emit(11, line_count + 2);
      emit(10, LC1);
    } else
      error(53);
    fix_breaks(line_count);
    fix_conts(LC2);
  }

  void while_statement(BlockLocal* bl) {
    Item X;
    int LC1, LC2, LC3;
    SymbolSet su;
    in_symbol();
    if (symbol == Symbol::LPARENT)
      in_symbol();
    else
      error(9);
    LC1 = line_count;
    BREAK_POINT = BREAK_POINT + 1;
    BREAK_LOCATION[BREAK_POINT] = 0;
    CONTPNT = CONTPNT + 1;
    CONTLOC[CONTPNT] = 0;
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::RPARENT), true);
    expression(bl, su, X);
    if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
          X.types == Types::NOTYP))
      error(17);
    LC2 = line_count;
    emit(11);
    if (symbol == Symbol::RPARENT)
      in_symbol();
    else
      error(4);
    statement(bl, bl->FSYS);
    emit(10, LC1);
    CODE[LC2].Y = line_count;
    if (symbol_count == 1)
      LOCATION[LINE_NUMBER] = line_count;
    fix_breaks(line_count);
    fix_conts(line_count - 1);
  }

  void comma_expression(BlockLocal* bl, Symbol TESTSY) {
    Item X;
    SymbolSet su;
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::COMMA), true);
    su.set(static_cast<int>(TESTSY), true);
    expression(bl, su, X);
    if (X.size > 1)
      emit(112, X.size);
    emit(111);
    while (symbol == Symbol::COMMA) {
      in_symbol();
      expression(bl, su, X);
      if (X.size > 1)
        emit(112, X.size);
      emit(111);
    }
  }

  void for_statement(BlockLocal* bl) {
    Item X;
    int LC1, LC2, LC3;
    SymbolSet su;
    in_symbol();
    if (symbol == Symbol::LPARENT)
      in_symbol();
    else
      error(9);
    BREAK_POINT = BREAK_POINT + 1;
    BREAK_LOCATION[BREAK_POINT] = 0;
    CONTPNT = CONTPNT + 1;
    CONTLOC[CONTPNT] = 0;
    if (symbol != Symbol::SEMICOLON)
      comma_expression(bl, Symbol::SEMICOLON);
    if (symbol == Symbol::SEMICOLON)
      in_symbol();
    else
      error(14);
    LC1 = line_count;
    if (symbol != Symbol::SEMICOLON) {
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::SEMICOLON), true);
      expression(bl, su, X);
      if (!(X.types == Types::BOOLS || X.types == Types::INTS ||
            X.types == Types::NOTYP))
        error(17);
    } else
      emit(24, 1);
    if (symbol == Symbol::SEMICOLON)
      in_symbol();
    else
      error(14);
    LC2 = line_count;
    emit(11);
    emit(10);
    LC3 = line_count;
    if (symbol != Symbol::RPARENT)
      comma_expression(bl, Symbol::RPARENT);
    emit(10, LC1);
    if (symbol == Symbol::RPARENT)
      in_symbol();
    else
      error(4);
    CODE[LC2 + 1].Y = line_count;
    statement(bl, bl->FSYS);
    emit(10, LC3);
    CODE[LC2].Y = line_count;
    if (symbol_count == 1)
      LOCATION[LINE_NUMBER] = line_count;
    fix_breaks(line_count);
    fix_conts(line_count - 1);
  }

  void block_statement(BlockLocal* bl) {
    Types CVT;
    Item X;
    SymbolSet su, sv;
    int64_t SAVEDX, SAVENUMWITH, SAVEMAXNUMWITH, PRB, PRT, I;
    bool TCRFLAG;
    // INSYMBOL();
    ID = dummy_name;
    enter(bl, ID, Objects::Procedure);
    dummy_name[13] = static_cast<char>(dummy_name[13] + 1);
    if (dummy_name[13] == '0')
      dummy_name[12] = static_cast<char>(dummy_name[12] + 1);
    TAB[tab_index].normal = true;
    SAVEDX = bl->DX;
    SAVENUMWITH = bl->NUMWITH;
    SAVEMAXNUMWITH = bl->MAXNUMWITH;
    bl->LEVEL = bl->LEVEL + 1;
    bl->DX = BASESIZE;
    PRT = tab_index;
    if (bl->LEVEL > LMAX)
      fatal(5);
    bl->NUMWITH = 0;
    bl->MAXNUMWITH = 0;
    enter_block();
    DISPLAY[bl->LEVEL] = btab_index;
    PRB = btab_index;
    TAB[PRT].types = Types::NOTYP;
    TAB[PRT].reference = PRB;
    BTAB[PRB].LASTPAR = tab_index;
    BTAB[PRB].PSIZE = bl->DX;
    su = 0;
    su.set(static_cast<int>(Symbol::LSETBRACK), true);
    sv = declaration_set | keyword_set | assigners_set;
    test(su, sv, 3);
    in_symbol();
    emit(18, PRT);
    emit(19, BTAB[PRB].PSIZE - 1);
    TAB[PRT].address = line_count;
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::RSETBRACK), true);
    // sv = DECLBEGSYS | STATBEGSYS | ASSIGNBEGSYS;
    test(sv, su, 101);
    // while (DECLBEGSYS[SY] || STATBEGSYS[SY] || ASSIGNBEGSYS[SY])
    while (sv.test(static_cast<int>(symbol))) {
      if (symbol == Symbol::DEFINESY)
        const_declaration(bl);
      else if (symbol == Symbol::TYPESY)
        type_declaration(bl);
      else if (symbol == Symbol::STRUCTSY)
        var_declaration(bl);
      else if (symbol == Symbol::IDENT) {
        I = loc(bl, ID);
        if (I != 0) {
          if (TAB[I].object == Objects::Type1)
            var_declaration(bl);
          else {
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            su.set(static_cast<int>(Symbol::RSETBRACK), true);
            statement(bl, su);
          }
        } else
          in_symbol();
      } else if (keyword_set.test(static_cast<int>(symbol)) ||
                 assigners_set.test(static_cast<int>(symbol))) {
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::SEMICOLON), true);
        su.set(static_cast<int>(Symbol::RSETBRACK), true);
        statement(bl, su);
      }
      su = declaration_set | keyword_set | assigners_set;
      su.set(static_cast<int>(Symbol::RSETBRACK), true);
      test(su, bl->FSYS, 6);
    }
    BTAB[PRB].VSIZE = bl->DX;
    BTAB[PRB].VSIZE = BTAB[PRB].VSIZE + bl->MAXNUMWITH;
    if (symbol_count == 1)
      LOCATION[LINE_NUMBER] = line_count;
    if (symbol == Symbol::RSETBRACK)
      in_symbol();
    else
      error(7);
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::PERIOD), true);
    sv = 0;
    test(su, sv, 6);
    emit(105);
    bl->DX = SAVEDX;
    bl->NUMWITH = SAVENUMWITH;
    bl->MAXNUMWITH = SAVEMAXNUMWITH;
    bl->LEVEL = bl->LEVEL - 1;
  }

  void forall_statement(BlockLocal* bl) {
    Types CVT;
    Item X;
    int64_t I, J, LC1, LC2, LC3;
    // bool DEFOUND;
    bool TCRFLAG;
    SymbolSet su;
    bl->FLEVEL = bl->FLEVEL + 1;
    in_symbol();
    if (symbol == Symbol::IDENT) {
      I = loc(bl, ID);
      TAB[I].FORLEV = bl->FLEVEL;
      in_symbol();
      if (I == 0)
        CVT = Types::INTS;
      else {
        if (TAB[I].object == Objects::Variable) {
          CVT = TAB[I].types;
          if (!TAB[I].normal)
            error(26);
          else
            emit(0, TAB[I].LEV, TAB[I].address);
          if (!(CVT == Types::NOTYP || CVT == Types::INTS ||
                CVT == Types::BOOLS || CVT == Types::CHARS))
            error(18);
        } else {
          error(37);
          CVT = Types::INTS;
        }
      }
    } else {
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::BECOMES), true);
      su.set(static_cast<int>(Symbol::TOSY), true);
      su.set(static_cast<int>(Symbol::DOSY), true);
      su.set(static_cast<int>(Symbol::GROUPINGSY), true);
      skip(su, 2);
      CVT = Types::INTS;
    }
    if (symbol == Symbol::BECOMES) {
      in_symbol();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::TOSY), true);
      su.set(static_cast<int>(Symbol::DOSY), true);
      su.set(static_cast<int>(Symbol::GROUPINGSY), true);
      expression(bl, su, X);
      if (X.types != CVT)
        error(19);
    } else {
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::TOSY), true);
      su.set(static_cast<int>(Symbol::DOSY), true);
      su.set(static_cast<int>(Symbol::GROUPINGSY), true);
      skip(su, 51);
    }
    if (symbol == Symbol::TOSY) {
      in_symbol();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::ATSY), true);
      su.set(static_cast<int>(Symbol::DOSY), true);
      su.set(static_cast<int>(Symbol::GROUPINGSY), true);
      expression(bl, su, X);
      if (X.types != CVT)
        error(19);
    } else {
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::ATSY), true);
      su.set(static_cast<int>(Symbol::DOSY), true);
      su.set(static_cast<int>(Symbol::GROUPINGSY), true);
      skip(su, 55);
    }
    if (symbol == Symbol::GROUPINGSY) {
      in_symbol();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::ATSY), true);
      su.set(static_cast<int>(Symbol::DOSY), true);
      expression(bl, su, X);
      if (X.types != Types::INTS)
        error(45);
    } else
      emit(24, 1);
    emit(4);
    LC1 = line_count;
    emit(75);
    LC2 = line_count;
    TAB[I].FORLEV = -TAB[I].FORLEV;
    if (symbol == Symbol::ATSY) {
      in_symbol();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::DOSY), true);
      expression(bl, su, X);
      if (!(X.types == Types::INTS || X.types == Types::NOTYP))
        error(126);
    } else
      emit(79);
    if (symbol == Symbol::DOSY)
      in_symbol();
    else
      error(54);
    TAB[I].FORLEV = -TAB[I].FORLEV;
    bl->CREATEFLAG = false;
    if (symbol == Symbol::IDENT) {
      J = loc(bl, ID);
      if (J != 0) {
        if (TAB[J].object == Objects::Procedure && TAB[J].LEV != 0)
          bl->CREATEFLAG = true;
      }
    }
    if (bl->CREATEFLAG)
      emit(74, 1);
    else
      emit(74, 0);
    TCRFLAG = bl->CREATEFLAG;
    LC3 = line_count;
    emit(10, 0);
    if (symbol == Symbol::LSETBRACK)
      block_statement(bl);
    else {
      su = bl->FSYS;
      statement(bl, su);
    }
    TAB[I].FORLEV = 0;
    if (TCRFLAG)
      emit(104, LC3 + 1, 1);
    else
      emit(104, LC3 + 1, 0);
    emit(70);
    CODE.at(LC3).Y = line_count;
    emit(76, LC2);
    CODE.at(LC1).Y = line_count;
    emit(5);
    bl->FLEVEL = bl->FLEVEL - 1;
    if (symbol_count == 1)
      LOCATION[LINE_NUMBER] = line_count;
  }

  void fork_statement(BlockLocal* bl) {
    Item X;
    int I; // added local (unlikely intended to use global)
    int JUMPLC;
    SymbolSet su;
    bl->CREATEFLAG = false;
    emit(106);
    in_symbol();
    if (symbol == Symbol::LPARENT) {
      in_symbol();
      if (symbol == Symbol::ATSY) {
        in_symbol();
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::RPARENT), true);
        expression(bl, su, X);
        if (!(X.types == Types::INTS || X.types == Types::NOTYP))
          error(126);
      } else
        error(127);
      if (symbol == Symbol::RPARENT)
        in_symbol();
      else
        error(4);
    } else
      emit(79);
    if (symbol == Symbol::IDENT) {
      I = loc(bl, ID);
      if (I != 0) {
        if (TAB[I].object == Objects::Procedure && TAB[I].LEV != 0)
          bl->CREATEFLAG = true;
      }
    }
    if (bl->CREATEFLAG)
      emit(67, 1);
    else
      emit(67, 0);
    JUMPLC = line_count;
    emit(7, 0);
    statement(bl, bl->FSYS);
    emit(69);
    CODE.at(JUMPLC).Y = line_count;
    if (symbol_count == 1)
      LOCATION[LINE_NUMBER] = line_count;
  }

  void join_statement(BlockLocal* bl) {
    emit(85);
    in_symbol();
  }

  void return_statement(BlockLocal* bl) {
    Item X;
    in_symbol();
    if (return_type.types != Types::VOIDS) {
      expression(bl, bl->FSYS, X);
      if (X.types == Types::RECS || X.types == Types::ARRAYS)
        emit(113, X.size);
      if (!type_compatible(return_type, X))
        error(118);
      else {
        if (return_type.types == Types::REALS && X.types == Types::INTS)
          emit(26);
        emit(33);
      }
    } else
      emit(32);
  }

  void break_statement(BlockLocal* bl) {
    emit(10, BREAK_LOCATION[BREAK_POINT]);
    BREAK_LOCATION[BREAK_POINT] = line_count - 1;
    in_symbol();
  }

  void continue_statement() {
    emit(10, CONTLOC[CONTPNT]);
    CONTLOC[CONTPNT] = line_count - 1;
    in_symbol();
  }

  void move_statement(BlockLocal* bl) {
    Item X;
    SymbolSet su;
    in_symbol();
    su = bl->FSYS;
    su.set(static_cast<int>(Symbol::TOSY), true);
    basic_expression(bl, su, X);
    if (!(X.is_address && X.types == Types::CHANS))
      error(103);
    if (symbol == Symbol::TOSY) {
      in_symbol();
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::SEMICOLON), true);
      expression(bl, su, X);
      if (!(X.types == Types::INTS || X.types == Types::NOTYP))
        error(126);
    } else
      emit(8, 19);
    emit(115);
  }

  //    void INPUTSTATEMENT(BlockLocal *bl)
  //    {
  //        ITEM X;
  //        SYMSET su;
  //        INSYMBOL();
  //        if (symbol == INSTR)
  //        {
  //            do
  //            {
  //                INSYMBOL();
  //                if (symbol == ENDLSY)
  //                {
  //                    emit(63);
  //                    INSYMBOL();
  //                } else if (symbol == STRNG)
  //                {
  //                    emit(24, STRING_LENGTH);
  //                    emit(28, INUM);
  //                    INSYMBOL();
  //                } else
  //                {
  //                    su = bl->FSYS;
  //                    sv.set(static_cast<int>(INSTR), true);
  //                    sv.set(static_cast<int>(SEMICOLON), true);
  //                    EXPRESSION(bl, su, X);
  //                    if (!X.ISADDR)
  //                    {
  //                        error(131);
  //                    } else
  //                    {
  //                        if (X.TYP == INTS || X.TYP == REALS || X.TYP == CHARS
  //                        || X.TYP == NOTYP)
  //                        {
  //                            emit(27, (int)X.TYP);
  //                        } else
  //                        {
  //                            error(40);
  //                        }
  //                    }
  //                }
  //            } while (symbol == INSTR);
  //        } else
  //        {
  //            error(129);
  //        }
  //    }
  void input_statement(BlockLocal* bl) {
    Item X;
    SymbolSet su;
    in_symbol();
    if (symbol == Symbol::INSTR) {
      do {
        in_symbol();
        su = bl->FSYS;
        su.set(static_cast<int>(Symbol::INSTR), true);
        su.set(static_cast<int>(Symbol::SEMICOLON), true);
        basic_expression(bl, su, X);
        if (!X.is_address)
          error(131);
        else {
          if (X.types == Types::INTS || X.types == Types::REALS ||
              X.types == Types::CHARS || X.types == Types::NOTYP)
            emit(27, (int)X.types);
          else
            error(40);
        }
      } while (symbol == Symbol::INSTR);
    } else
      error(129);
  }

  void cout_methods(BlockLocal* bl) {
    ALFA METHOD;
    Item X;
    SymbolSet su;
    in_symbol();
    METHOD = ID;
    if (symbol != Symbol::IDENT || (strcmp(ID.c_str(), "WIDTH         ") != 0 &&
                                    strcmp(
                                      ID.c_str(),
                                      "PRECISION     "
                                    ) != 0))
      error(133);
    else {
      in_symbol();
      if (symbol == Symbol::LPARENT)
        in_symbol();
      else
        error(9);
      su = bl->FSYS;
      su.set(static_cast<int>(Symbol::RPARENT), true);
      expression(bl, su, X);
      if (X.types != Types::INTS)
        error(134);
      if (symbol != Symbol::RPARENT)
        error(4);
      if (strcmp(METHOD.c_str(), "WIDTH         ") == 0)
        emit(30, 1);
      else
        emit(30, 2);
    }
    in_symbol();
  }

  void output_statement(BlockLocal* bl) {
    Item X;
    SymbolSet su;
    in_symbol();
    if (symbol == Symbol::PERIOD)
      cout_methods(bl);
    else {
      emit(89);
      if (symbol == Symbol::OUTSTR) {
        do {
          in_symbol();
          if (symbol == Symbol::ENDLSY) {
            emit(63);
            in_symbol();
          } else if (symbol == Symbol::STRNG) {
            emit(24, STRING_LENGTH);
            emit(28, INUM);
            in_symbol();
          } else {
            su = bl->FSYS;
            su.set(static_cast<int>(Symbol::OUTSTR), true);
            su.set(static_cast<int>(Symbol::SEMICOLON), true);
            expression(bl, su, X);
            if (!(standard_set.test(static_cast<int>(X.types))))
              error(41);
            if (X.types == Types::REALS)
              emit(37);
            else
              emit(29, static_cast<int>(X.types));
          }
        } while (symbol == Symbol::OUTSTR);
      } else
        error(128);
      emit(90);
    }
  }

  /* declared in SWITCHSTATEMENT therefore available to the contained functions
   * ala Pascal */
  struct CaseTable {
    Index VAL, line_count;
  };

  struct SwitchLocal {
    Item X;
    int I, J, K, LC1, LC2, LC3;
    bool DEFOUND;
    std::array<CaseTable, CSMAX + 1> case_table;
    std::array<int, CSMAX + 1> exit_table;
    BlockLocal* bl;
  };

  static auto case_label(SwitchLocal* /*sl*/) -> void;
  static auto one_case(SwitchLocal* /*sl*/) -> void;

  auto case_label(SwitchLocal* switch_local) -> void {
    CONREC LAB = {Types::NOTYP, 0L};
    int64_t K = 0;
    SymbolSet su;
    su = switch_local->bl->FSYS;
    su.set(static_cast<int>(Symbol::COLON), true);
    constant(switch_local->bl, su, LAB);
    if (LAB.TP != switch_local->X.types)
      error(47);
    else if (switch_local->I == CSMAX)
      fatal(6);
    else {
      switch_local->I = switch_local->I + 1;
      K = 0;
      switch_local->case_table[switch_local->I].VAL = static_cast<int>(LAB.I);
      switch_local->case_table[switch_local->I].line_count = line_count;
      do {
        K = K + 1;
      } while (switch_local->case_table[K].VAL != LAB.I);
      if (K < switch_local->I)
        error(1);
    }
  }

  void one_case(SwitchLocal* switch_local) {
    SymbolSet su;
    if (base_set.test(static_cast<int>(symbol))) {
      case_label(switch_local);
      if (symbol == Symbol::COLON)
        in_symbol();
      else
        error(5);

      su = switch_local->bl->FSYS;
      su.set(static_cast<int>(Symbol::CASESY), true);
      su.set(static_cast<int>(Symbol::SEMICOLON), true);
      su.set(static_cast<int>(Symbol::DEFAULTSY), true);
      su.set(static_cast<int>(Symbol::RSETBRACK), true);

      while (keyword_set.test(static_cast<int>(symbol)) ||
             assigners_set.test(static_cast<int>(symbol)))
        statement(switch_local->bl, su);

      su = 0;
      su.set(static_cast<int>(Symbol::CASESY), true);
      su.set(static_cast<int>(Symbol::DEFAULTSY), true);
      su.set(static_cast<int>(Symbol::RSETBRACK), true);
      test(su, switch_local->bl->FSYS, 125);
    }
  }

  void switch_statement(BlockLocal* block_local) {
    // int K;
    struct SwitchLocal switch_local{};
    memset(&switch_local, 0, sizeof(struct SwitchLocal));

    switch_local.bl = block_local;
    SymbolSet su;

    BREAK_POINT = BREAK_POINT + 1;
    BREAK_LOCATION[BREAK_POINT] = 0;
    in_symbol();

    if (symbol == Symbol::LPARENT)
      in_symbol();
    else
      error(9);

    switch_local.I = 0;
    su = block_local->FSYS;
    su.set(static_cast<int>(Symbol::RPARENT), true);
    su.set(static_cast<int>(Symbol::COLON), true);
    su.set(static_cast<int>(Symbol::LSETBRACK), true);
    su.set(static_cast<int>(Symbol::CASESY), true);
    expression(block_local, su, switch_local.X);

    if (switch_local.X.types != Types::INTS &&
        switch_local.X.types != Types::BOOLS &&
        switch_local.X.types != Types::CHARS &&
        switch_local.X.types != Types::NOTYP)
      error(23);

    if (symbol == Symbol::RPARENT)
      in_symbol();
    else
      error(4);

    switch_local.LC1 = line_count;
    emit(12);

    if (symbol == Symbol::LSETBRACK)
      in_symbol();
    else
      error(106);
    if (symbol == Symbol::CASESY)
      in_symbol();
    else
      error(124);

    one_case(&switch_local);

    while (symbol == Symbol::CASESY) {
      in_symbol();
      one_case(&switch_local);
    }

    switch_local.DEFOUND = false;
    if (symbol == Symbol::DEFAULTSY) {
      in_symbol();

      if (symbol == Symbol::COLON)
        in_symbol();
      else
        error(5);

      switch_local.LC3 = line_count;

      while (keyword_set.test(static_cast<int>(symbol)) ||
             assigners_set.test(static_cast<int>(symbol))) {
        su = block_local->FSYS;
        su.set(static_cast<int>(Symbol::SEMICOLON), true);
        su.set(static_cast<int>(Symbol::RSETBRACK), true);
        statement(block_local, su);
      }
      switch_local.DEFOUND = true;
    }

    switch_local.LC2 = line_count;
    emit(10);
    CODE.at(switch_local.LC1).Y = line_count;

    for (switch_local.K = 1; switch_local.K <= switch_local.I; switch_local.K
         ++) {
      emit(13, switch_local.case_table.at(switch_local.K).VAL);
      emit(13, switch_local.case_table.at(switch_local.K).line_count);
    }

    if (switch_local.DEFOUND) {
      emit(13, -1, 0);
      emit(13, switch_local.LC3);
    }

    emit(10, 0);
    CODE.at(switch_local.LC2).Y = line_count;
    if (symbol_count == 1)
      LOCATION[line_count] = line_count;

    if (symbol == Symbol::RSETBRACK)
      in_symbol();
    else
      error(104);

    fix_breaks(line_count);
  }
} // namespace cs
