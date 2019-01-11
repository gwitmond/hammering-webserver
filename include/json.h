// Hammering-webserver suite
//
// JSON parsers
//
// Copyright 2018, Guido Witmond <guido@witmond.nl>
// Licensed under AGPL v3 or later. See LICENSE

#ifndef __JSON_H
#define __JSON_H

// Full general JSON parser
HParser *json;

// JSON general sub-structure parsers
HParser *json_any_number;
HParser *json_any_string;
HParser *json_any_array;
HParser *json_any_object;

// sub grammer parsers
HParser *lit_true;
HParser *lit_false;
HParser *lit_null;

HParser *ws;
HParser *comma;
HParser *json_char;
HParser *any_name_value_pair;

// Specific parser generators
HParser *json_name_value_pair(uint8_t* name, HParser* value_p);
HParser *json_object(HParser *parser);
HParser *json_prefix();

// Initialiser
void init_json_parser();

#endif
