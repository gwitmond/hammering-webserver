// Hammering-webserver suite
//
// Test suite.
//
// Copyright 2018, Guido Witmond <guido@witmond.nl>
// Licensed under AGPL v3 or later. See LICENSE


#include <hammer/hammer.h>
#include <glib.h>
#include <string.h>
#include "test_suite.h"

#include "parser-helpers.h"
#include "http.h"
#include "json.h"

// Don't care about leaking memory at every other test

HParserBackend pr = PB_PACKRAT;

static void test_sp(void) {
  g_assert(NULL != h_parse(sp(), " ", 1));
  g_assert(NULL == h_parse(sp(), "!", 1));
  g_check_parse_match(sp(), pr, " ", 1, "u0x20");
}


static void test_tab(void) {
  g_assert(NULL == h_parse(tab(), " ", 1));
  g_assert(NULL != h_parse(tab(), "\t", 1));
  g_check_parse_match(tab(), pr, "\t", 1, "u0x9");
}


static void test_status_code_200(void) {
  g_assert(NULL != h_parse(status_code_200(), "200", 3));
  g_assert(NULL == h_parse(END(status_code_200()), "2000", 4)); // no extra digits allowd
  g_assert(NULL == h_parse(END(status_code_200()), "200 ", 4)); // don't match space behind it
  g_assert(NULL == h_parse(END(status_code_200()), " 200", 4)); // don't match in front either
  g_assert(NULL == h_parse(END(status_code_200()), "201", 3));  // get the number correct
  g_check_parse_match(END(status_code_200()), pr, "200", 3, "<32.30.30>");
  // assume the other status_code_NNN go the same way. So don't write test for them
}


static void test_status_code(void) {
  g_assert(NULL != h_parse(any_status_code(), "200", 3));
  g_assert(NULL != h_parse(any_status_code(), "400", 3));
  g_assert(NULL != h_parse(any_status_code(), LEN("400")));

  // expect the status code returned.
  g_check_parse_match(END(any_status_code()), pr, "200", 3, "<32.30.30>");
  g_assert_cmpmem("200", 3, h_parse(any_status_code(), LEN("200"))->ast->bytes.token, 3);
  g_assert_cmpmem("404", 3, h_parse(any_status_code(), LEN("404"))->ast->bytes.token, 3);
}


static void test_http_version_1_1(void) {
  g_assert(NULL != h_parse(http_version(), LEN("HTTP/1.1")));
  g_assert(NULL == h_parse(http_version(), LEN("HTTP/1.0")));
  g_check_parse_match(END(http_version()), pr, "HTTP/1.1", 8, "NULL"); // no output
}


static void test_status_line(void) {
  g_assert(NULL == h_parse(status_line(any_status_code()), LEN("HTTP/1.1")));
  g_assert(NULL == h_parse(status_line(any_status_code()), LEN("HTTP/1.1 ")));
  g_assert(NULL == h_parse(status_line(any_status_code()), LEN("HTTP/1.1 200")));
  g_assert(NULL == h_parse(status_line(any_status_code()), LEN("HTTP/1.1 200\r\n")));  // missing sp after 200 is required by the spec
  g_assert(NULL != h_parse(status_line(any_status_code()), LEN("HTTP/1.1 200 \r\n"))); // missing reason is ok
  g_assert(NULL != h_parse(status_line(any_status_code()), LEN("HTTP/1.1 200 OK\r\n"))); // full header is ok
  g_assert(NULL != h_parse(status_line(any_status_code()), LEN("HTTP/1.1 300 OK\r\n")));
  g_assert(NULL == h_parse(status_line(any_status_code()), LEN("HTTP/1.1 3000 OK\r\n"))); // Only 3 digits
  g_assert(NULL == h_parse(status_line(any_status_code()), LEN("HTTP/1.1 666 Evil\r\n"))); // Only 100-599
  g_assert(NULL != h_parse(status_line(any_status_code()), LEN("HTTP/1.1 404 FooBar\r\n"))); // OK, as reason is igored

  // expect the status code returned, ignore the reason.
  g_assert_cmpmem("200", 3, h_parse(status_line(any_status_code()), LEN("HTTP/1.1 200 OK\r\n"))->ast->bytes.token, 3);
  g_assert_cmpmem("409", 3, h_parse(status_line(any_status_code()), LEN("HTTP/1.1 409 Foo\r\n"))->ast->bytes.token, 3);
  g_check_parse_match(END(status_line(any_status_code())), pr, "HTTP/1.1 200 XXX\r\n", 18, "<32.30.30>");
}


void test_lws(void) {
  g_assert(NULL != h_parse(END(lws()), LEN(" ")));
  g_assert(NULL != h_parse(END(lws()), LEN("  ")));
  g_assert(NULL != h_parse(END(lws()), LEN(" \t")));
  g_assert(NULL != h_parse(END(lws()), LEN("\t")));
  g_assert(NULL != h_parse(END(lws()), LEN("\t ")));
  g_assert(NULL != h_parse(END(lws()), LEN("\t\t")));
  g_assert(NULL != h_parse(END(lws()), LEN("\r\n ")));
  g_assert(NULL != h_parse(END(lws()), LEN("\r\n\t")));
  g_assert(NULL != h_parse(END(lws()), LEN("\r\n\t\t \t  ")));
  g_assert(NULL == h_parse(END(lws()), LEN("\r\n"))); // no trailing space or tab
  g_assert(NULL == h_parse(END(lws()), LEN(" \r\n "))); // no heading space
  g_assert(NULL == h_parse(END(lws()), LEN("\t\r\n "))); // no heading tab

  // test that lws always returns a single space
  g_check_parse_match(END(lws()), pr, "\r\n\t ", 4, "u0x20");
}


void test_header_names(void) {
  g_assert(NULL != h_parse(END(any_header_name()), LEN("AAA")));
  g_assert(NULL == h_parse(END(any_header_name()), LEN("AAA:"))); // : is not part of the name

  g_assert(TT_BYTES == h_parse(any_header_name(), LEN("AAA"))->ast->token_type);
  g_assert_cmpmem("ABC", 3, h_parse(any_header_name(), LEN("ABC"))->ast->bytes.token, 3);
  g_assert_cmpmem("BBB", 3, h_parse(any_header_name(), LEN("BBB"))->ast->bytes.token, 3);
  g_check_parse_match(END(any_header_name()), pr, "ABC", 3, "<41.42.43>");
}


void test_header_values(void) {
  g_assert(NULL != h_parse(END(any_header_value()), LEN("AAA")));
  g_assert(NULL != h_parse(END(any_header_value()), LEN("AAA:"))); // : is allowed in the value
  g_assert(NULL != h_parse(END(any_header_value()), LEN("::::"))); // strangly this is allowed too
  g_assert(NULL != h_parse(END(any_header_value()), LEN("AAA\r\n BBB"))); // header continuation
  g_assert(NULL != h_parse(END(any_header_value()), LEN("AAA\r\n\tBBB"))); // header continuation
  g_assert(NULL == h_parse(END(any_header_value()), LEN("AAA\r\nBBB"))); // header continuation require a space or tab at the next line
  g_assert(NULL == h_parse(END(any_header_value()), LEN("AAA\r\nBBB: CCC"))); // typical header injection

  g_assert(TT_BYTES == h_parse(any_header_value(), LEN("AAA"))->ast->token_type);
  g_assert_cmpmem("ABC", 3, h_parse(any_header_value(), LEN("ABC"))->ast->bytes.token, 3);
  g_assert_cmpmem("BBB", 3, h_parse(any_header_value(), LEN("BBB"))->ast->bytes.token, 3);
  g_assert_cmpmem("BB:", 3, h_parse(any_header_value(), LEN("BB:"))->ast->bytes.token, 3);
  g_assert_cmpmem(":::", 3, h_parse(any_header_value(), LEN(":::"))->ast->bytes.token, 3);
  g_assert_cmpmem("AAA BBB", 7, h_parse(any_header_value(), LEN("AAA\r\n BBB"))->ast->bytes.token, 7);
  g_assert_cmpmem("AAA BBB", 7, h_parse(any_header_value(), LEN("AAA\r\n\tBBB"))->ast->bytes.token, 7);
  g_assert_cmpmem("AAA BBB", 7, h_parse(any_header_value(), LEN("AAA\r\n\t  BBB"))->ast->bytes.token, 7);
  g_assert_cmpmem("AAA BBB CCC", 11, h_parse(any_header_value(), LEN("AAA\r\n\t  BBB\r\n  CCC"))->ast->bytes.token, 11);  
  g_check_parse_match(END(any_header_value()), pr, "ABC", 3, "<41.42.43>");
}


void test_general_header(void) {
  g_assert(NULL != h_parse(END(general_header()), LEN("AAA:BBB\r\n")));
  g_assert(NULL != h_parse(END(general_header()), LEN("AAA: BBB\r\n")));
  g_assert(NULL == h_parse(END(general_header()), LEN("AAA:BBB"))); // no crlf at the end
  g_assert(NULL != h_parse(END(general_header()), LEN("AAA:BBB \r\n"))); // space at end of line is allowed.
  g_assert(NULL != h_parse(END(general_header()), LEN("AAA:BBB\r\n \r\n"))); // single space on new line is allowed.
  g_assert(NULL != h_parse(END(general_header()), LEN("AAA:BBB\r\n\t\r\n"))); // single tab on new line is allowed.
  g_assert(NULL == h_parse(END(general_header()), LEN("AAA:BBB\r\nCCC:DDD\r\n"))); // typical header injection is rejected
  g_assert(NULL != h_parse(END(general_header()), LEN("AAA:BBB\r\n CCC:DDD\r\n"))); // header continutation

  // test return values
  g_check_parse_match(    general_header() , pr, "A:B\r\n", 5,  "(<41> <42>)");  // header returns (Name Value)
  g_check_parse_match(END(general_header()), pr, "A:B\r\n", 5, "(<41> <42>)"); // END is an extra sequence

  g_check_parse_match(END(general_header()), pr, "A:B \r\n", 6, "(<41> <42.20>)"); // allow trailing ws
  g_check_parse_match(END(general_header()), pr, "A: B\r\n", 6, "(<41> <42>)");  // ws after : is ignored
}


void test_named_header(void) {
  g_assert(NULL != h_parse(END(named_header("AAA")), LEN("AAA:BBB\r\n")));
  g_assert(NULL != h_parse(END(named_header("AAA")), LEN("AAA: BBB\r\n")));
  g_assert(NULL == h_parse(END(named_header("AAA")), LEN("AAA\r\n"))); // incomplete, missing :
  g_assert(NULL != h_parse(END(named_header("AAA")), LEN("AAA:\r\n"))); // empty value is actually allowed by the spec
  g_assert(NULL == h_parse(END(named_header("BBB")), LEN("AAA:BBB\r\n"))); // don't match on value
  g_assert(NULL == h_parse(END(named_header("CCC")), LEN("AAA:BBB\r\nCCC:DDD\r\n"))); // don't allow injection
  g_assert(NULL == h_parse(END(named_header("CCC")), LEN("\r\n CCC:DDD\r\n"))); // don't allow injection
  g_assert(NULL != h_parse(END(named_header("AAA")), LEN("AAA:BBB\r\n CCC:DDD\r\n"))); // valid continuation

  // test return values
  g_check_parse_match(END(named_header("A")), pr, "A:B\r\n", 5, "(<41> <42>)");
}


void test_response_header(void) {
  g_assert(NULL != h_parse(END(response_header()), LEN("Age: foo\r\n")));
}


void test_entity_header(void) {
  g_assert(NULL != h_parse(END(entity_header()), LEN("Content-Type: foo\r\n")));
}


void test_full_header(void) {
  uint8_t *full_response =
    "HTTP/1.1 200 FOO\r\n"
    "Host: 127.0.0.1.2.3.4\r\n"
    "Content-Type: application/json ; charset = utf-666\r\n"
    "X-general_header::::Foo:Bar\r\n"
    " A continuation\r\n"
    "\r\n";
  g_assert(NULL != h_parse(END(http_response()), LEN(full_response)));
  g_assert(NULL == h_parse(END(http_response()), full_response, strlen(full_response) -1)); // skip last character -> FAIL
  g_assert(NULL == h_parse(END(http_response()), full_response, strlen(full_response) -2)); // skip last 2 chars -> FAIL
}

void test_message_body(void) {
  g_assert(NULL != h_parse(END(json), LEN("42")));
  g_assert(NULL != h_parse(END(json), LEN("[42 ] ")));
  g_assert(NULL != h_parse(END(json), LEN("\"42\"")));
  g_assert(NULL != h_parse(END(json), LEN("{\"answer\": 42}")));
  uint8_t *body =
    "{ \"Image\": { \"Width\":  800, \"Title\":  \"View from 15'th Floor\", \n\r"
    " \"Thumbnail\": {\"Url\": \"http://www.example.com/image/481989943\", \n\r"
    " \"Height\": 125, \"Width\":  \"100\" }, \"IDs\": [116, 943, 234, 38793] }}";
  g_assert(NULL != h_parse(END(json), LEN(body)));
}


void test_full_request(void) {
  uint8_t *full_request =
    "HTTP/1.1 200 OK\r\n"
    "Host: 127.0.0.1.2.3.4\r\n"
    "Content-Type: application/json ; charset = utf-666\r\n"
    "X-general_header::::Foo:Bar\r\n"
    " A header continuation\r\n"
    "\r\n"
    "{ \"Image\": "
    " { \"Width\":  800, "
    "   \"Height\":  600, "
    "   \"Title\":  \"View from 15'th Floor\", \n\r"
    "   \"Thumbnail\": "
    "      {\"Url\": \"http://www.example.com/image/481989943\", \n\r"
    "       \"Height\": 125, "
    "       \"Width\":  \"100\" }, "
    "   \"IDs\": [116, 943, 234, 38793] }}";
  g_assert(NULL != h_parse(END(http_response()), LEN(full_request)));
}


void test_json_any_object(void) {
  uint8_t *test =
    "{ \"x\": true, \"y\":\"42\" }";
  g_assert(NULL != h_parse(END(json_any_object), LEN(test)));
}


void test_json_any_string(void) {
  g_assert(NULL == h_parse(END(json_char), LEN("")));     // empty string fails
  g_assert(NULL == h_parse(END(json_char), LEN("\"")));   // single " fails
  g_assert(NULL == h_parse(END(json_char), LEN("\\")));   // single \ fails
  g_assert(NULL != h_parse(END(json_char), LEN(" ")));    // ok, trivial
  g_assert(NULL != h_parse(END(json_char), LEN("A")));    // ok, trivial
  g_assert(NULL != h_parse(END(json_char), LEN("\\\""))); // \" ok
  g_assert(NULL != h_parse(END(json_char), LEN("\\\\"))); // \\ ok

  g_assert(NULL == h_parse(END(json_any_string), LEN("")));         // empty string fails, need quotes
  g_assert(NULL != h_parse(END(json_any_string), LEN("\"\"")));     // "" ok
  g_assert(NULL == h_parse(END(json_any_string), LEN("\\\"\\\""))); // \"\" fails
  g_assert(NULL != h_parse(END(json_any_string), LEN("\"\\\"\""))); // "\"" ok
  g_assert(NULL == h_parse(END(json_any_string), LEN("foo")));      // foo without quotes fails
  g_assert(NULL != h_parse(END(json_any_string), LEN("\"foo\"")));  // "foo" ok
}


void test_json_name_value_pair_assert(void) {
  if (g_test_subprocess()) {
    // must fail assertion for " as that is an illegal name
    HParser *p = json_name_value_pair("\"", h_ch('A'));
  }
  g_test_trap_subprocess(NULL, 0, 0);
  g_test_trap_assert_failed();
}


void test_json_name_value_pairs(void) {
  // assert valid names
  g_assert(NULL != json_name_value_pair("x", h_token("<anything>", 10)));    // trivial
  g_assert(NULL != json_name_value_pair("\\\"", h_token("<anything>", 10))); // \" is a valid json_string
  g_assert(NULL != json_name_value_pair("\\\\", h_token("<anything>", 10))); // \\ is a valid json_string

  uint8_t *test = "\"x\": 42";
  g_assert(NULL != h_parse(END(json_name_value_pair("x", json_any_number)), LEN(test)));
  g_assert(NULL == h_parse(END(json_name_value_pair("y", json_any_number)), LEN(test))); // fails as x /= y
  g_assert(NULL != h_parse(END(json_name_value_pair("x", h_token("42", 2))), LEN(test)));
  g_assert(NULL == h_parse(END(json_name_value_pair("x", h_token("43", 2))), LEN(test))); // 42 /= 43
}


// Example functions to parse a strict json object name-value pair
HParser *success_true() { return json_name_value_pair("success", lit_true); }
HParser *id_int()       { return json_name_value_pair("id", h_many1(h_ch_range('0', '9'))); }

void test_json_specific_object(void) {
  uint8_t *test1 = "{ \"x\": 42 }";
  g_assert(NULL != h_parse(END(json_object(json_name_value_pair("x", json_any_number))), LEN(test1)));

  uint8_t *test2 = "{ \"x\": 42, \"y\": 84 }";
  g_assert(NULL != h_parse(END(json_object(h_sequence(json_name_value_pair("x", json_any_number),
						      comma,
						      json_name_value_pair("y", json_any_number),
						      NULL))),
			   LEN(test2)));

  uint8_t *test3 = "{ \"x\": 42, \"y\": \"foo\" }";
  g_assert(NULL != h_parse(END(json_object(h_sequence(json_name_value_pair("x", json_any_number),
						      comma,
						      json_name_value_pair("y", json_any_string),
						      NULL))),
			   LEN(test3)));

  uint8_t *test4 = "{ \"success\": true}";
  g_assert(NULL != h_parse(END(json_object(h_sequence(json_name_value_pair("success", lit_true),
						      NULL))),
			   LEN(test4)));
  g_assert(NULL != h_parse(END(json_object(success_true())), LEN(test4)));

  uint8_t *test5 = "{ \"success\": true, \"id\": 42}";
  g_assert(NULL != h_parse(END(json_object(h_sequence(json_name_value_pair("success", lit_true),
						      comma,
						      json_name_value_pair("id", json_any_number),
						      NULL))),
			   LEN(test5)));

  // The sequence-parser determines a strict order of success and id fields.
  // The second line fails because of this.
  g_assert(NULL != h_parse(END(json_object(h_sequence(success_true(), comma, id_int(), NULL))), LEN(test5)));
  g_assert(NULL == h_parse(END(json_object(h_sequence(id_int(), comma, success_true(), NULL))), LEN(test5)));

  // The permutation parser allows for different order.
  // Switch success and id parsers, it must still match.
  // It has the small bug that is also accepts the comma in the wrong place:
  // -> { "success":true "id":42 , } <-
  // TODO: Write a better hammer parser that allows permutation with a separator in between.
  g_assert(NULL != h_parse(END(json_object(h_sequence(success_true(), comma, id_int(), NULL))), LEN(test5)));
  g_assert(NULL != h_parse(END(json_object(h_permutation(id_int(), comma, success_true(), NULL))), LEN(test5)));
}


void test_post_url_chars(void) {
  g_assert(NULL != h_parse(END(post_url_chars()), LEN("/bla")));
  g_assert(NULL != h_parse(END(post_url_chars()), LEN("/bla/foo.text")));
  g_assert(NULL == h_parse(END(post_url_chars()), LEN("/bla?foo")));   // no ?query=val in POSTs allowed
}


void test_post_url_reject(void) {  
  if (g_test_subprocess()) {
    // must fail assertion as ? is not allowed in a post request url
    HParser *p = post("/bla?foo", h_ch('A'), h_ch('B'));
  }
  g_test_trap_subprocess(NULL, 0, 0);
  g_test_trap_assert_failed();
}


void test_request_uri(void) {
    g_assert(NULL != h_parse(END(request_uri()), LEN("/bla")));
    g_assert(NULL != h_parse(END(request_uri()), LEN("/bla?")));
    g_assert(NULL != h_parse(END(request_uri()), LEN("/bla?foo=bar")));
    g_assert(NULL != h_parse(END(request_uri()), LEN("/bla")));
    g_check_parse_match(END(request_uri()), pr, "/bla", 4, "(u0x2f u0x62 u0x6c u0x61)");
}

void test_request_line(void) {
  //g_assert(NULL != h_parse(END(request_uri()), LEN("/bla")));
  HParser *p = request_line(post_method(), h_literal("/bla"));
  uint8_t *req = "POST /bla HTTP/1.1\r\n";
  size_t len = strlen(req);
  g_assert(NULL != h_parse(END(p), req, len));
  g_check_parse_match(END(p), pr, req, len, "<2f.62.6c.61>");
}


void test_request(void) {
  uint8_t *req =
    "GET /bla?foo HTTP/1.1\r\n"
    "Host: 127.0.0.1.2.3.4\r\n"
    "Content-Type: application/json ; charset = utf-666\r\n"
    "\r\n"
    ;
  g_assert(NULL != h_parse(END(generic_http_request()), LEN(req)));

  uint8_t *req2 =
    "POST /bla HTTP/1.1\r\n"
    "Host: foo\r\n"
    "\r\n"
    "XXX"
    ;
  size_t len2 = strlen(req2);
  g_assert(NULL != h_parse(END(post("/bla", named_header("Host"), h_literal("XXX"))), req2, len2));
  g_check_parse_match(END(post("/bla", named_header("Host"), h_literal("XXX"))),
		      pr,
		      req2, len2,
		      "(<2f.62.6c.61> (<48.6f.73.74> <66.6f.6f>) <58.58.58>)");
}



int main(int argc, char *argv[]) {
  g_test_init(&argc, &argv, NULL);
  init_json_parser();
  g_test_add_func("/test_sp", test_sp);
  g_test_add_func("/test_tab", test_tab);
  g_test_add_func("/test_http_version_1_1", test_http_version_1_1);
  g_test_add_func("/test_status_code_200", test_status_code_200);
  g_test_add_func("/test_status_code", test_status_code);
  g_test_add_func("/test_status_line", test_status_line);

  g_test_add_func("/test_lws", test_lws);
  g_test_add_func("/test_header_names", test_header_names);
  g_test_add_func("/test_header_values", test_header_values);
  g_test_add_func("/test_general_header", test_general_header);
  g_test_add_func("/test_named_header", test_named_header);
  g_test_add_func("/test_response_header", test_response_header);
  g_test_add_func("/test_entity_header", test_entity_header);
  g_test_add_func("/test_full_header", test_full_header);
  g_test_add_func("/test_message_body", test_message_body);
  g_test_add_func("/test_full_request", test_full_request);

  g_test_add_func("/test_json_any_object", test_json_any_object);
  g_test_add_func("/test_json_any_string", test_json_any_string);
  g_test_add_func("/test_json_name_value_pair_assert", test_json_name_value_pair_assert);
  g_test_add_func("/test_json_name_value_pairs", test_json_name_value_pairs);
  g_test_add_func("/test_json_specific_object", test_json_specific_object);

  g_test_add_func("/test_post_url_chars", test_post_url_chars);
  g_test_add_func("/test_post_url_reject", test_post_url_reject);
  g_test_add_func("/test_request_uri", test_request_uri);
  g_test_add_func("/test_request_line", test_request_line);
  g_test_add_func("/test_request", test_request);

  g_test_run();
}
