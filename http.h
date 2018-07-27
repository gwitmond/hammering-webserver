// Hammering-webserver suite
//
// Generic HTTP request and response parsers
//
// Copyright 2018, Guido Witmond <guido@witmond.nl>
// Licensed under AGPL v3 or later. See LICENSE


//----------------------------------
// Trivials
HParser *sp();
HParser *tab();
HParser *crlf();
HParser *cr();
HParser *lf();
HParser *ascii();
HParser *lws();


//----------------------------------
// Status codes
HParser *any_status_code(void);
HParser *status_code_200(void);
HParser *status_code_201(void);
HParser *status_code_400(void);
HParser *status_code_403(void);
HParser *status_code_404(void);
HParser *status_code_409(void);
HParser *status_code_500(void);
HParser *status_code(uint8_t*);


//----------------------------------
// Headers
//
HParser *header_name(uint8_t *name);
HParser *header(HParser* name_p, HParser *value_p);
HParser *any_header_name(void);
HParser *any_header_value(void);
HParser *named_header(uint8_t *name);
HParser *general_header(void);
HParser *response_header(void);
HParser *entity_header(void);
HParser *message_body(void);
HParser *http_response(void);
HParser *headers(HParser *parser);


//----------------------------------
// Request
//
HParser *get_method(void);
HParser *post_method(void);
HParser *any_method(void);
HParser *post_url_chars(void);
HParser *request_line(HParser *method, HParser *url);
HParser *any_request_line(void);
HParser *generic_http_request(void);
HParser *post(uint8_t* url, HParser *header_p, HParser *body);
HParser *request_uri(void);
HParser *path(void);


//----------------------------------
// Response
HParser *http_version(void);
HParser *reason_phrase(void);
HParser *reason_text(void);
HParser *status_line(HParser *status_code);
HParser *http_response(void); // TODO: rename to: any_http_response

//----------------------------------
// Helpers
HParsedToken *sequence_to_bytes(const HParseResult *p, void *user_data);
HParsedToken *token_to_bytes(HArena *arena, const HParsedToken *t, void *user_data);
