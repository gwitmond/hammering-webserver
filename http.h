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
HParser *any_status_code();
HParser *status_code_200();
HParser *status_code_201();
HParser *status_code_400();
HParser *status_code_403();
HParser *status_code_404();
HParser *status_code_409();
HParser *status_code_500();
HParser *status_code(uint8_t*);


//----------------------------------
// Headers
//
HParser *header(HParser *name_p, HParser *value_p);
HParser *any_header_name(void);
HParser *any_header_value();
HParser *named_header();
HParser *general_header();
HParser *response_header();
HParser *entity_header();
HParser *message_body();
HParser *http_response();
HParser *headers();


//----------------------------------
// Request
//
HParser *get_method();
HParser *post_method();
HParser *any_method();
HParser *post_url_chars();
HParser *request_line(HParser *method, HParser *url);
HParser *any_request_line();
HParser *generic_http_request();
HParser *post(uint8_t* url, HParser *header_p, HParser *body);
HParser *request_uri();


//----------------------------------
// Response
HParser *http_version();
HParser *reason_phrase();
HParser *reason_text();
HParser *status_line();
