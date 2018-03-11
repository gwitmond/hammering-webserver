// Hammering-webserver suite
//
// JSON parsers
// Heavily modified from: https://github.com/sergeybratus/HammerPrimer/blob/master/lecture_13/json.c
// (Stolen, as there is no license specified in that repository).
// (I hope Sergey can forgive me :-)
//
// Copyright 2018, Guido Witmond <guido@witmond.nl>
// Licensed under AGPL v3 or later. See LICENSE


#include <hammer/hammer.h>
#include <hammer/glue.h>
#include "parser-helpers.h"
#include "json.h"
#include "test_suite.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

// global json parser
HParser *json;

// JSON sub-structures (exported)
HParser *json_any_number;
HParser *json_any_string;
HParser *json_any_array;
HParser *json_any_object;

HParser *lit_true;
HParser *lit_false;
HParser *lit_null;

HParser *colon;
HParser *ws;
HParser *any_name_value_pair;

// JSON sub-parsers (non-exported)
HParser *quote;
HParser *comma;
HParser *left_curly_bracket;
HParser *right_curly_bracket;


enum JSONTokenType {
    TT_json_object_t = TT_USER,
    TT_json_array_t,
    TT_json_string_t
};

typedef HParsedToken* json_object_t;
typedef HParsedToken* json_array_t;
typedef HParsedToken* json_string_t;

HParsedToken *act_json_any_object(const HParseResult *p, void *user_data) {
    const HParsedToken *tok = p->ast;
    return H_MAKE(json_object_t, tok->seq);
}

HParsedToken *act_json_any_array(const HParseResult *p, void *user_data) {
    return H_MAKE(json_array_t, p->ast->seq);
}

HParsedToken *act_json_any_string(const HParseResult *p, void *user_data) {
    return H_MAKE(json_string_t, p->ast->seq);
}


void init_json_parser() {
    /* Whitespace */
    EH_RULE(ws, h_in((uint8_t*)" \r\n\t", 4));

    /* Structural tokens */
    H_RULE( left_square_bracket,  h_ignore(h_sequence(h_many(ws), h_ch('['), h_many(ws), NULL)));
    H_RULE(right_square_bracket,  h_ignore(h_sequence(h_many(ws), h_ch(']'), h_many(ws), NULL)));
    EH_RULE( left_curly_bracket,  h_ignore(h_sequence(h_many(ws), h_ch('{'), h_many(ws), NULL)));
    EH_RULE(right_curly_bracket,  h_ignore(h_sequence(h_many(ws), h_ch('}'), h_many(ws), NULL)));
    EH_RULE(colon,                h_ignore(h_sequence(h_many(ws), h_ch(':'), h_many(ws), NULL)));
    EH_RULE(comma,                h_ignore(h_sequence(h_many(ws), h_ch(','), h_many(ws), NULL)));

    /* Literal tokens */
    EH_RULE(lit_true,  h_literal("true"));
    EH_RULE(lit_false, h_literal("false"));
    EH_RULE(lit_null,  h_literal("null"));

    /* Forward declarations */
    HParser *value = h_indirect();
    
    /* Numbers */
    H_RULE(minus, h_ch('-'));
    H_RULE(dot,   h_ch('.'));
    H_RULE(digit, h_ch_range(0x30, 0x39));
    H_RULE(non_zero_digit, h_ch_range(0x31, 0x39));
    H_RULE(zero, h_ch('0'));
    H_RULE(exp, h_choice(h_ch('E'), h_ch('e'), NULL));    
    H_RULE(plus, h_ch('+'));

    json_any_number = h_sequence(h_optional(minus),
				 h_choice(zero,
					  h_sequence(non_zero_digit,
						     h_many(digit),
						     NULL),
					  NULL),
				 h_optional(h_sequence(dot,
						       h_many1(digit),
						       NULL)),
				 h_optional(h_sequence(exp,
						       h_optional(h_choice(plus,
									   minus,
									   NULL)),
						       h_many1(digit),
						       NULL)),
				 NULL);
    
    /* Strings */
    EH_RULE(quote,        h_ch('"'));
    H_RULE(backslash,     h_ch('\\'));
    H_RULE(esc_quote,     h_sequence(backslash, quote, NULL));
    H_RULE(esc_backslash, h_sequence(backslash, backslash, NULL));
    H_RULE(esc_slash,     h_sequence(backslash, h_ch('/'), NULL));
    H_RULE(esc_backspace, h_sequence(backslash, h_ch('b'), NULL));
    H_RULE(esc_ff,        h_sequence(backslash, h_ch('f'), NULL));
    H_RULE(esc_lf,        h_sequence(backslash, h_ch('n'), NULL));
    H_RULE(esc_cr,        h_sequence(backslash, h_ch('r'), NULL));
    H_RULE(esc_tab,       h_sequence(backslash, h_ch('t'), NULL));
    H_RULE(escaped,       h_choice(esc_quote, esc_backslash, esc_slash, esc_backspace,
			     esc_ff, esc_lf, esc_cr, esc_tab, NULL));
    H_RULE(unescaped, h_not_in((uint8_t*)"\"\\\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F", 34));
    EH_RULE(json_char, h_choice(escaped, unescaped, NULL));

    json_any_string = h_action(h_middle(quote,
					h_many(json_char),
					quote),
			       act_json_any_string, NULL);
    
    /* Arrays */
    json_any_array = h_action(h_middle(left_square_bracket,
				       h_sepBy(value, comma),
				       right_square_bracket),
			      act_json_any_array, NULL);

    /* Objects */
    EH_RULE(any_name_value_pair, h_sequence(json_any_string,
					    colon,
					    value,
					    NULL));

    json_any_object = h_action(h_middle(left_curly_bracket,
					h_sepBy(any_name_value_pair, comma),
					right_curly_bracket),
			       act_json_any_object, NULL);
    
    h_bind_indirect(value, h_choice(json_any_object,
				    json_any_array,
				    json_any_number,
				    json_any_string,
				    lit_true,
				    lit_false,
				    lit_null,
				    NULL));

    // the main json parser, it parses any value.
    json = h_sequence(value, NULL);
}


// Parse a specific name-value-pair
HParser *json_name_value_pair(uint8_t* name, HParser* value_p) {
  assert(NULL != h_parse(h_sequence(h_many(json_char), h_end_p(), NULL), name, strlen(name)));
  return h_sequence(h_ignore(quote),
		    h_token(name, strlen(name)),
		    h_ignore(quote),
		    h_ignore(colon),
		    value_p,
		    NULL);
}



/* Parse a single JSON object,
 * Parameter:
 * A parser to match a name-value pair,
 * Or a h_permutation of (NV-pair1, comma, NV-pair2, comma, etc)
 * (explicitly add the commas.
 * Returns what your parser returns
 */
HParser *json_object(HParser *parser) {
  return h_middle(left_curly_bracket,
                  parser,
		  right_curly_bracket);
}


/* Parse the idiotic json body prefix */
PF_RULE(json_prefix, h_literal(")]}'\r\n"));
