// Hammering-webserver suite
//
// Generic HTTP request and response parsers
// Losely following RFC 2616.
//
// Copyright 2018, Guido Witmond <guido@witmond.nl>
// Licensed under AGPL v3 or later. See LICENSE

#include <hammer/hammer.h>
#include <hammer/glue.h>
#include "parser-helpers.h"
#include "http.h"
#include <string.h>


//----------------------------------------
//  Basic Rules (RFC 2616 sec 2.2)
//
PF_RULE(sp,           h_ch(' '));               // ASCII space 0x32
PF_RULE(tab,          h_ch('\t'));              // TAB
PF_RULE(cr,           h_ch('\r'));              // ASCII carriage return
PF_RULE(lf,           h_ch('\n'));              // ASCII line feed
PF_RULE(crlf,         h_token("\r\n", 2));      // ASCII carriage return + line feed
PF_RULE(http_version, h_ignore(h_token(LEN("HTTP/1.1")))); // never show this token in the output


//----------------------------------------
// Linear whitespace (RFC 2616 sec 2.2)
//
HParsedToken *act_lws(const HParseResult *p, void *user_data) {
  // Replace any sequence with a single space
  return H_MAKE_UINT(' ');
}

/* Parse Linear whitespace (LWS)
 * LWS = [CRLF] 1*( SP | HT )
 */
HParser *lws() {
  return h_action(h_sequence(h_optional(crlf()),
			     h_many1(h_choice(sp(),
					      tab(),
					      NULL)),
			     NULL),
		  act_lws, NULL);
}


//------------------------------------------------
// Headers

/* Helper sequence_to_bytes
 * Translate a sequence of bytes and strings into a single string.
 */
HParsedToken *sequence_to_bytes(const HParseResult *p, void *user_data) {
  assert(TT_SEQUENCE == p->ast->token_type);
  // printf("\nunamb seq: %s\n", h_write_result_unamb(p->ast));
  HParsedToken *seq = h_seq_flatten(p->arena, p->ast);
  size_t len = h_seq_len(seq);
  uint8_t *arr = h_arena_malloc(p->arena, len +1); // +1 for \0
  for (uint8_t i=0; i < len; i++) {
    arr[i] = h_seq_index(seq, i)->uint;
  }
  arr[len] = 0; // make it a printf-able c-string

  HParsedToken *t = h_arena_malloc(p->arena, sizeof(HParsedToken));
  t->token_type = TT_BYTES;
  t->bytes.len = len; // don't mention the \0 so len == strlen(arr) 
  t->bytes.token = arr;
  // printf("unamb res: %s\n", h_write_result_unamb(t));
  return t;
}


/* Parse a header
 * It needs a parser for a name and one for a value
 * Returns: (name value) tuple
 */
HParser *header(HParser *name_p, HParser *value_p) {
  return h_sequence(name_p,
		    h_ignore(h_ch(':')),
		    h_ignore(h_optional(lws())), // eat the space after the :
		    value_p,
		    h_ignore(crlf()),
		    NULL);
}

/* Parse any header name
 * Deviate a bit from the spec in RFC 2616 sec 2.2 (token)
 */
HParser *any_header_name() {
  uint8_t *allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
  return h_action(h_many1(h_in(allowed, strlen(allowed))),
		  sequence_to_bytes, NULL);
}


/* Parse any header_value
 *  value = *( field-content | LWS )
 */
HParser *any_header_value() {
  return h_action(h_many(h_choice(reason_text(),
				  lws(),
				  NULL)),
		  sequence_to_bytes, NULL);
}

//-----------------------------------------
// General header (RFC 2616 sec 4.2)
//
//    message-header = field-name ":" [ field-value ]
//    field-name     = token
//    field-value    = *( field-content | LWS )
//    field-content  = <the OCTETs making up the field-value
//                     and consisting of either *TEXT or combinations
//                     of token, separators, and quoted-string>

PF_RULE(general_header, header(any_header_name(), any_header_value()));


//-----------------------------------------
// Named headers
// Match only a header with a specified name.
// Matches any header value.
HParser *named_header(uint8_t *name) {
  // Test the name against the allowed syntax for header names
  assert(NULL != h_parse(END(any_header_name()), name, strlen(name)));
  return header(h_token(name, strlen(name)), any_header_value());
}

//-----------------------------------------
// HTTP RESPONSE headers (RFC 2616 section 6.2)
// Notice: we ignore all the definitions for any of these, so any header value is accepted
// TODO: implement specific header with definitions in section 14.
PF_RULE(response_header, h_choice(
				  named_header("Accept-Ranges"),
				  named_header("Age"),
				  named_header("ETag"),
				  named_header("Location"),
				  named_header("Proxy-Authenticate"),
				  named_header("Retry-After"),
				  named_header("Vary"),
				  named_header("WWW-Authenticate"),
				  NULL));


//-----------------------------------------
// HTTP ENTITY headers (RFC 2616 section 7)
// Notice: we ignore all the definitions for any of these, so any header value is accepted
// TODO: implement specific header with definitions in section 14.
PF_RULE(entity_header, h_choice(
				named_header("Allow"),
				named_header("Content-Encoding"),
				named_header("Content-Language"),
				named_header("Content-Length"),
				named_header("Content-Location"),
				named_header("Content-MD5"),
				named_header("Content-Range"),
				named_header("Content-Type"),
				named_header("Expires"),
				named_header("Last-Modified"),
				NULL));

/* Parse headers.
 * Parameters: a parser
 * It parses what the parser parser and returns that.
 * TODO: make it a parser that matches all given parameters and ignores any left over headers.
 */
HParser *headers(HParser *parser) {
  // TODO: add parser to match (and ignore) all other headers.
  return parser;
}

//----------------------------------------
// HTTP REQUEST PARSING
//


// HTTP methods

PF_RULE(get_method,  h_literal("GET"));
PF_RULE(post_method, h_literal("POST"));

PF_RULE(any_method, h_choice(get_method(),
			     post_method(),
			     NULL));


PF_RULE(path, h_many1(h_ch_range(33, 126))); // TODO: make it more URL-like
PF_RULE(request_uri, path());
PF_RULE(post_url_chars, h_many1(h_in(LEN("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					 "abcdefghijklmnopqrstuvwxyz"
					 "0123456789"
					 "/."))));

/* Parse any request line.
 * Returns a two-tuple (method url)
 */
PF_RULE(any_request_line, h_sequence(any_method(),
				     h_ignore(sp()),
				     request_uri(),
				     h_ignore(sp()),
				     h_ignore(http_version()),
				     h_ignore(crlf()),
				     NULL));

/* Parse a request_line 
 * Parameters: 
 * - method-parser; i.e.: GET, HEAD, POST, CONNECT;
 * - url-parser; 
 * Returns the url that matched
 */
HParser *request_line(HParser *method, HParser *url) {
  return h_middle(h_sequence(method,
			     sp(), NULL),
		  url,
		  h_sequence(sp(),
			     http_version(),
			     crlf(),
			     NULL));
}


/* Parse a generic http request.
 * It matches any valid request and does not validate any individual parts.
 * Caller must validate all data returned.
 * Returns: three-tuple (url, headers, body)
 */
PF_RULE(generic_http_request, h_sequence(any_request_line(),
					 h_many(h_choice(response_header(),
							 entity_header(),
							 general_header(),
							 NULL)),
					 h_ignore(crlf()),
					 h_optional(message_body()),
					 NULL));

/* Parse a specific POST request.
 * Parameters:
 * - url: the literal url; must validate against post_url_chars
 * - header-parser: a parser to parse all headers; tip: use h_permutation(...)
 * - body-parser: a parser to parse the request body.
 * For json-request, use a json parser.
 * Returns a three-tuple of the url and what the header and body parsers return. 
 */
HParser *post(uint8_t* url, HParser *header_p, HParser *body) {
  //printf("url is: >>%s<<\n", url);
  assert(NULL != h_parse(END(post_url_chars()), url, strlen(url)));
  HParser *url_p = h_token(url, strlen(url));
  return h_sequence(request_line(post_method(), url_p),
		    headers(header_p),
		    h_ignore(crlf()),
		    body,
		    NULL);
}


//----------------------------------------
// HTTP RESPONSE PARSING
//

//-----------------------------------------
// HTTP RESPONSE (RFC 2616 section 6)
//
// Response = Status-Line
//            *(( general-header
//              | response-header
//              | entity-header
//              ) CRLF
//             )
//            CRLF
//            [ message-body ]

/* Parse a generic HTTP response
 * It matches any valid response
 * Caller needs to validate all data returned.
 * Returns: tuple: (status code, headers, body)
 */
PF_RULE(http_response, h_sequence(status_line(any_status_code()),
				  h_many(h_choice(response_header(),
						  entity_header(),
						  general_header(),
						  NULL)),
				  h_ignore(crlf()),
				  h_optional(message_body()),
				  NULL));

//-----------------------------------------
// HTTP Status-Line (RFC 2616 section 6.1)
//
// Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
//

PF_RULE(reason_phrase, h_many(reason_text()));

/* Parse the status line.
 * Return only the status code
 */
HParser *status_line(HParser *status_code) {
  return h_middle(h_sequence(http_version(), sp(), NULL),
		  status_code,
		  h_sequence(sp(), reason_phrase(), crlf(), NULL));
}

//-----------------------------------------
// Status codes
//
PF_RULE(status_code_200, h_token("200", 3));
PF_RULE(status_code_201, h_token("201", 3));
// ..
PF_RULE(status_code_400, h_token("400", 3));
PF_RULE(status_code_403, h_token("403", 3));
PF_RULE(status_code_404, h_token("404", 3));
PF_RULE(status_code_409, h_token("409", 3));
// ..
PF_RULE(status_code_500, h_token("500", 3));

/* Parse a specific status code
 * Returns the specified code
 */
HParser* status_code(uint8_t* code) {
  //assert(NULL != h_parse(any_status_code, code, strlen(code)));
  return h_token(code, strlen(code));
}

/* Parse any status code matches anything in range 100-599
 * Returns the parsed code
 */
HParser *any_status_code(void) {
  return h_action(h_sequence(h_ch_range('1', '5'), h_ch_range('0', '9'), h_ch_range('0', '9'), NULL),
		  sequence_to_bytes, NULL);
}

//-----------------------------------------
// Message body

/* Parse a message body.
 * it matches anything, unconditionally.
 * Use this parser only to match until EOF
 * Returns the parsed data.
 */
PF_RULE(message_body, h_many(h_ch_range(0, 255)));



//----------------------------------
// Fixmes:

// TODO: rename 'ascii' and 'text' to something more rfc-like to prevent pollution of the namespace
// ASCII characters (Deviation from RFC: No high bits, so no UTF-8)
PF_RULE(ascii, h_ch_range(32, 126));

// we deviate from the spec and accept only ascii chars tab and 32-126 for headers and such
PF_RULE(reason_text, h_choice(ascii(),
				tab(),
				NULL));

