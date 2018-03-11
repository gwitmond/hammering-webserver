// Hammering-webserver suite
//
// Generic HTTP request and response parsers
// Losely following RFC 2616.
//
// Copyright 2018, Guido Witmond <guido@witmond.nl>
// Licensed under AGPL v3 or later. See LICENSE


// Helper macro copied from hammer src.
#define h_literal(s) h_token(s, sizeof(s)-1)


// Parser Functions macro
//   behave like H_RULE but return a function instead of a variable.
// Like this:
//   PF_RULE(sp, h_ch(' '));
// returns:
//   HParser *sp() {
//     return h_ch(' ');
//   }
#define PF_RULE(rule, def) HParser *rule() { return def; }


// E.._RULE s define like the hammer-ones except the don't declare the variable.
// Add one in the desires scope (i.e. file scope, header file, etc)
// #define H_RULE(rule, def)  HParser *rule = def
#define   EH_RULE(rule, def)           rule = def


// Get the parse input string lengths correct, all the time
#define LEN(str) str, strlen(str)

// Parse a parser and match end of input
#define END(parser) h_left(parser, h_end_p())


