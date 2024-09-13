//
// Created by Dan Evans on 1/11/24.
//
#include "cs_compile.h"
#include "cs_global.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <print>
#include <string>

#define EXPORT_CS_ERRORS
#include "cs_errors.h"
namespace cs {
using std::cin;
using std::cout;
using std::exit;
using std::fputc;
using std::print;
using std::println;
using std::string;

extern auto inCLUDEFLAG() -> bool;
extern FILE *LIS;

string MSG[] = {"Undefined Identifier     ",
                "Multiple Definitions     ",
                "Identifier Expected      ",
                "{                        ",
                ")                        ",
                ":                        ",
                "Syntax Error             ",
                "}                        ",
                "Expression Error         ",
                "(                        ",
                "Identifier or Struct     ",
                "Stream Error             ",
                "]                        ",
                "Pointer Expected         ",
                ";                        ",
                "Lock Variable required   ",
                "Pointer Required         ",
                "Boolean Required         ",
                "Forall Index Type        ",
                "Wrong Type               ",
                "",
                "Number too big           ",
                "Improper Termination     ",
                "Type error after Switch  ",
                "Illegal character        ",
                "Invalid constant defn    ",
                "Index Type Mismatch      ",
                "Illegal Index Bound      ",
                "Not an Array             ",
                "Type Identifier Expected   ",
                "Undefined Type Identifier",
                "Not a Structure          ",
                "Boolean Type Required    ",
                "Expression Type          ",
                "Integer Type Required    ",
                "Types                    ",
                "Parameter Type           ",
                "Variable Identifier      ",
                "String Empty             ",
                "Number of Parameters     ",
                "Type Error               ",
                "Type Error               ",
                "Topology Not Found       ",
                "Type Error               ",
                "Variable or Constant     ",
                "Integer Required         ",
                "Types Incompatible (=)   ",
                "Struct Type Error        ",
                "Parameter Type           ",
                "Storage Overflow         ",
                "Constant Expected        ",
                "=                        ",
                "",
                "While Expected           ",
                "Do Expected              ",
                "To Expected              ",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "No Main Function         ",
                "Function Body            ",
                "Program Start Error      ",
                "Stream                   ",
                "}                        ",
                "Parameter Type Error     ",
                "{                        ",
                "Identifier               ",
                "Struct Type Expected     ",
                "Component Declaration    ",
                "Increment Error          ",
                "Decrement Error          ",
                "Pointer Expected         ",
                "Pointer Arithmetic       ",
                "Assignment Error         ",
                "Improper Type            ",
                "Parameter Error          ",
                "Array Parameter          ",
                "Return Type              ",
                "Function Return Type     ",
                "Pointer Required         ",
                "Type Cast                ",
                "Function within Function ",
                "Statement Start          ",
                "Case Expected            ",
                "Case End                 ",
                "Integer Required         ",
                "@                        ",
                "<<                       ",
                ">>                       ",
                "'                       ",
                "Input Expression Invalid ",
                "\\                        ",
                "Invalid Cout Method      ",
                "Integer Required         ",
                ",                        ",
                "Initializer Not Allowed  ",
                "Array Type               ",
                "Initializer Type         ",
                "Too Many Values          ",
                "Stream Type Expected     ",
                "Types Incompatible       ",
                "Recv Parameter Invalid   ",
                "Stream Error             ",
                "Shared Memory Not Allowed",
                "MPI Errror               ",
                "MPI Mode Requried        ",
                "Communicator Type        ",
                "MPI Parameter Type       ",
                "File Error               ",
                "Include File Error       ",
                "Improper Use of Void     ",
                "Funct Prototype Mismatch ",
                "Prototype Duplication    ",
                "No Union Type Allowed    ",
                "No Enumeration Allowed   ",
                "No Bitwise Ops Allowed   ",
                "No Cond Expressions      "};

void error_message() {
  int K;
  int ei;
  //        MSG[0] = "Undefined Identifier     ";
  //        MSG[1] = "Multiple Definitions     ";
  //        MSG[2] = "Identifier Expected      ";
  //        MSG[3] = "{                        ";
  //        MSG[4] = ")                        ";
  //        MSG[5] = ":                        ";
  //        MSG[6] = "Syntax Error             ";
  //        MSG[7] = "}                        ";
  //        MSG[8] = "Expression Error         ";
  //        MSG[9] = "(                        ";
  //        MSG[10] = "Identifier or Struct     ";
  //        MSG[11] = "Stream Error             ";
  //        MSG[12] = "]                        ";
  //        MSG[13] = "Pointer Expected         ";
  //        MSG[14] = ";                        ";
  //        MSG[15] = "Lock Variable required   ";
  //        MSG[16] = "Pointer Required         ";
  //        MSG[17] = "Boolean Required         ";
  //        MSG[18] = "Forall Index Type        ";
  //        MSG[19] = "Wrong Type               ";
  //        MSG[21] = "Number too big           ";
  //        MSG[22] = "Improper Termination     ";
  //        MSG[23] = "Type error after Switch  ";
  //        MSG[24] = "Illegal character        ";
  //        MSG[25] = "Invalid constant defn    ";
  //        MSG[26] = "Index Type Mismatch      ";
  //        MSG[27] = "Illegal Index Bound      ";
  //        MSG[28] = "Not an Array             ";
  //        MSG[29] = "Type Identifier Expected   ";
  //        MSG[30] = "Undefined Type Identifier";
  //        MSG[31] = "Not a Structure          ";
  //        MSG[32] = "Boolean Type Required    ";
  //        MSG[33] = "Expression Type          ";
  //        MSG[34] = "Integer Type Required    ";
  //        MSG[35] = "Types                    ";
  //        MSG[36] = "Parameter Type           ";
  //        MSG[37] = "Variable Identifier      ";
  //        MSG[38] = "String Empty             ";
  //        MSG[39] = "Number of Parameters     ";
  //        MSG[40] = "Type Error               ";
  //        MSG[41] = "Type Error               ";
  //        MSG[42] = "Topology Not Found       ";
  //        MSG[43] = "Type Error               ";
  //        MSG[44] = "Variable or Constant     ";
  //        MSG[45] = "Integer Required         ";
  //        MSG[46] = "Types Incompatible (=)   ";
  //        MSG[47] = "Struct Type Error        ";
  //        MSG[48] = "Parameter Type           ";
  //        MSG[49] = "Storage Overflow         ";
  //        MSG[50] = "Constant Expected        ";
  //        MSG[51] = "=                        ";
  //        MSG[53] = "While Expected           ";
  //        MSG[54] = "Do Expected              ";
  //        MSG[55] = "To Expected              ";
  //        MSG[100] = "No Main Function         ";
  //        MSG[101] = "Function Body            ";
  //        MSG[102] = "Program Start Error      ";
  //        MSG[103] = "Stream                   ";
  //        MSG[104] = "}                        ";
  //        MSG[105] = "Parameter Type Error     ";
  //        MSG[106] = "{                        ";
  //        MSG[107] = "Identifier               ";
  //        MSG[108] = "Struct Type Expected     ";
  //        MSG[109] = "Component Declaration    ";
  //        MSG[110] = "Increment Error          ";
  //        MSG[111] = "Decrement Error          ";
  //        MSG[112] = "Pointer Expected         ";
  //        MSG[113] = "Pointer Arithmetic       ";
  //        MSG[114] = "Assignment Error         ";
  //        MSG[115] = "Improper Type            ";
  //        MSG[116] = "Parameter Error          ";
  //        MSG[117] = "Array Parameter          ";
  //        MSG[118] = "Return Type              ";
  //        MSG[119] = "Function Return Type     ";
  //        MSG[120] = "Pointer Required         ";
  //        MSG[121] = "Type Cast                ";
  //        MSG[122] = "Function within Function ";
  //        MSG[123] = "Statement Start          ";
  //        MSG[124] = "Case Expected            ";
  //        MSG[125] = "Case End                 ";
  //        MSG[126] = "Integer Required         ";
  //        MSG[127] = "@                        ";
  //        MSG[128] = "<<                       ";
  //        MSG[129] = ">>                       ";
  //        MSG[130] = "'                       ";
  //        MSG[131] = "Input Expression Invalid ";
  //        MSG[132] = "\\                        ";
  //        MSG[133] = "Invalid Cout Method      ";
  //        MSG[134] = "Integer Required         ";
  //        MSG[135] = ",                        ";
  //        MSG[136] = "Initializer Not Allowed  ";
  //        MSG[137] = "Array Type               ";
  //        MSG[138] = "Initializer Type         ";
  //        MSG[139] = "Too Many Values          ";
  //        MSG[140] = "Stream Type Expected     ";
  //        MSG[141] = "Types Incompatible       ";
  //        MSG[142] = "Recv Parameter Invalid   ";
  //        MSG[143] = "Stream Error             ";
  //        MSG[144] = "Shared Memory Not Allowed";
  //        MSG[145] = "MPI Errror               ";
  //        MSG[146] = "MPI Mode Requried        ";
  //        MSG[147] = "Communicator Type        ";
  //        MSG[148] = "MPI Parameter Type       ";
  //        MSG[149] = "File Error               ";
  //        MSG[150] = "Include File Error       ";
  //        MSG[151] = "Improper Use of Void     ";
  //        MSG[152] = "Funct Prototype Mismatch ";
  //        MSG[153] = "Prototype Duplication    ";
  //        MSG[154] = "No Union Type Allowed    ";
  //        MSG[155] = "No Enumeration Allowed   ";
  //        MSG[156] = "No Bitwise Ops Allowed   ";
  //        MSG[157] = "No Cond Expressions      ";

  // WRITELN(LIS); WRITELN(LIS, ' COMPILATION ERRORS');
  println(LIS, "\n COMPILATION ERRORS");
  // WRITELN(LIS); WRITELN(LIS, ' ERROR CODES');
  println(LIS, "\n ERROR CODES");
  // WRITELN; WRITELN(' COMPILATION ERRORS');
  // WRITELN; WRITELN(' ERROR CODES');
  println(stdout, "\n COMPILATION ERRORS");
  println(stdout, "\n ERROR CODES");
  for (K = 0; K < errors.size(); ++K) {
    if (!errors[K]) {
      continue;
    }
    // WRITE(LIS, K);
    print(LIS, "{}", K);
    if (K < 10) {
      fputc(' ', LIS);
    }
    if (K < 100)
      fputc(' ', LIS);
    print(LIS, "  {}", MSG[K]);
    print(stdout, "{}", K);
    if (K < 10) {
      fputc(' ', stdout);
    }
    if (K < 100) {
      fputc(' ', stdout);
    }
    println(stdout, "  {}", MSG[K]);
  }
}

void error_exit() {
  float A = NAN;
  float B = NAN;
  cout << "PROGRAM SOURCE FILE IS NOW CLOSED TO ALLOW EDITING" << '\n';
  cout << '\n';
  cout << "To continue, press ENTER key, then Restart the C* Software System"
       << '\n';
  cin.ignore(1);
  exit(1);
  //        A = 0;
  //        B = B / A;
}

void error(int N) {
  int I;
  if (inCLUDEFLAG()) {
    if (error_count == 0) {
      println(stdout, "");
      print(stdout, "{:5} ", LNUM);
      for (I = 1; I <= LL; ++I) {
        fputc(line.str().at(I), stdout);
      }
      println(stdout, "");
      println(stdout, " ****                   ^150");
      println(LIS, " ****                   ^150");
      errors[150] = true;
    }
  } else {
    if (error_position == 0) {
      println(stdout, "");
      print(stdout, "{:5} ", LNUM);
      for (I = 1; I <= LL; ++I) {
        fputc(line.str().at(I), stdout);
      }
      println(stdout, "");
    }
    if (error_position == 0) {
      print(stdout, " ****");
      print(LIS, " ****");
    }
    if (CC > error_position && error_position == 0) {
      for (I = 0; I < CC - error_position; ++I) {
        fputc(' ', stdout);
        fputc(' ', LIS);
      }
      println(stdout, "^{:2}", N);
      println(LIS, "^{:2}", N);
      error_position = CC + 3;
      errors[N] = true;
    }
  }
  error_count += 1;
  // if (error_count > 1000)
  if (error_count > 10) {
    println(stdout, "");
    error_message();
    fatal_error = true;
    error_exit();
  }
}

static char MSGF[][11] = {
    "unused    ", "IDENTIFIER", "FUNCTIONS ", "STRINGS   ",
    "ARRAYS    ", "LEVELS    ", "CODE      ", "STRUCTS   ",
    "WITHS     ", "STREAMS   ", "FLOATS    ",
};

void fatal(int N) {
  cout << '\n';
  fatal_error = true;
  // cout << " COMPILER TABLE FOR %s IS TOO SMALL" << MSGF[N];
  print(stdout, " COMPILER TABLE FOR {} IS TOO SMALL", MSGF[N]);
  error_exit();
}
} // namespace Cstar
