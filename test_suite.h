// This file is copied from:
// https://github.com/UpstandingHackers/hammer/blob/master/src/test_suite.h
// And remains licensed under this GPL 2 license.
// -- Guido.

/* Test suite for Hammer.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef HAMMER_TEST_SUITE__H
#define HAMMER_TEST_SUITE__H
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

// Equivalent to g_assert_*, but not using g_assert...
#define g_check_inttype(fmt, typ, n1, op, n2) do {				\
    typ _n1 = (n1);							\
    typ _n2 = (n2);							\
    if (!(_n1 op _n2)) {						\
      g_test_message("%s line %d: Check failed: (%s): (" fmt " %s " fmt ")",	\
		     __FILE__, __LINE__,				\
		     #n1 " " #op " " #n2,				\
		     _n1, #op, _n2);					\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_bytes(len, n1, op, n2) do {	\
    const uint8_t *_n1 = (n1);			\
    const uint8_t *_n2 = (n2);			\
    if (!(memcmp(_n1, _n2, len) op 0)) {	\
      g_test_message("%s line %d: Check failed: (%s)",    	\
		     __FILE__, __LINE__,				\
		     #n1 " " #op " " #n2);				\
      g_test_fail();				\
    }						\
  } while(0)

#define g_check_string(n1, op, n2) do {		        \
    const char *_n1 = (n1);				\
    const char *_n2 = (n2);				\
    if (!(strcmp(_n1, _n2) op 0)) {			\
      printf("%s line %d: Check failed:  %s %s %s\n",	\
	     __FILE__, __LINE__, _n1, #op, #n2);	\
      g_test_fail();					\
    }							\
  } while(0)

#define g_check_regular(lang) do {			\
    if (!lang->isValidRegular(lang->env)) {		\
      g_test_message("Language is not regular");	\
      g_test_fail();					\
    }							\
  } while(0)

#define g_check_contextfree(lang) do {			\
    if (!lang->isValidCF(lang->env)) {			\
      g_test_message("Language is not context-free");	\
      g_test_fail();					\
    }							\
  } while(0)

#define g_check_compilable(lang, backend, params) do {	\
    if (!h_compile(lang, backend, params)) {		\
      g_test_message("Language is not %s(%s)", #backend, params);	\
      g_test_fail();					\
    }							\
  } while(0)

  
// TODO: replace uses of this with g_check_parse_failed
#define g_check_failed(res) do {					\
    const HParseResult *result = (res);					\
    if (NULL != result) {						\
      g_test_message("%s line %d: Check failed: shouldn't have succeeded, but did"); \
		     __FILE__, __LINE__,				\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_parse_failed(parser, backend, input, inp_len) do {	\
    int skip = h_compile((HParser *)(parser), (HParserBackend)backend, NULL); \
    if(skip != 0) {	\
      g_test_message("%s line %d: Backend not applicable, skipping test");	\
		     __FILE__, __LINE__,				\
      break;					\
    }	\
    const HParseResult *result = h_parse(parser, (const uint8_t*)input, inp_len); \
    if (NULL != result) {						\
      g_test_message("%d line %s: Check failed: shouldn't have succeeded, but did"); \
		     __FILE__, __LINE__,				\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_parse_ok(parser, backend, input, inp_len) do {		\
    int skip = h_compile((HParser *)(parser), (HParserBackend) backend, NULL); \
    if(skip) {								\
      g_test_message("%s line %d: Backend not applicable, skipping test");		\
		     __FILE__, __LINE__,				\
      break;								\
    }									\
    HParseResult *res = h_parse(parser, (const uint8_t*)input, inp_len); \
    if (!res) {								\
      g_test_message("Parse failed on %s line %d", __FILE__, __LINE__);	\
      g_test_fail();							\
    } else {								\
      HArenaStats stats;						\
      h_allocator_stats(res->arena, &stats);				\
      g_test_message("Parse used %zd bytes, wasted %zd bytes. "		\
                     "Inefficiency: %5f%%",				\
		     stats.used, stats.wasted,				\
		     stats.wasted * 100. / (stats.used+stats.wasted));	\
      h_delete_arena(res->arena);					\
    }									\
  } while(0)

#define g_check_parse_match(parser, backend, input, inp_len, result) do { \
    int skip = h_compile((HParser *)(parser), (HParserBackend) backend, NULL); \
    if(skip) {								\
      g_test_message("%s line %d: Backend not applicable, skipping test", \
		     __FILE__, __LINE__);				\
      break;								\
    }									\
    HParseResult *res = h_parse(parser, (const uint8_t*)input, inp_len); \
    if (!res) {								\
      printf("Parse failed on %s line %d", __FILE__, __LINE__);		\
      g_test_fail();							\
    } else {								\
      char* cres = h_write_result_unamb(res->ast);			\
      g_check_string(cres, ==, result);					\
      free(cres);							\
      HArenaStats stats;						\
      h_allocator_stats(res->arena, &stats);				\
      g_test_message("Parse used %zd bytes, wasted %zd bytes. "		\
                     "Inefficiency: %5f%%",				\
		     stats.used, stats.wasted,				\
		     stats.wasted * 100. / (stats.used+stats.wasted));	\
      h_delete_arena(res->arena);					\
    }									\
  } while(0)

#define g_check_hashtable_present(table, key) do {			\
    if(!h_hashtable_present(table, key)) {				\
      g_test_message("%s line %d: Check failed: key should have been in table, but wasn't"); \
		     __FILE__, __LINE__,				\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_hashtable_absent(table, key) do {			\
    if(h_hashtable_present(table, key)) {				\
      g_test_message("%s line %d: Check failed: key shouldn't have been in table, but was"); \
		     __FILE__, __LINE__,				\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_hashtable_size(table, n) do {				\
    size_t expected = n;						\
    size_t actual = (table)->used;					\
    if(actual != expected) {						\
      g_test_message("%s line %d: Check failed: table size should have been %zu, but was %zu", \
		     __FILE__, __LINE__,				\
		     expected, actual);					\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_stringmap_present(table, key) do {			\
    bool end = (key[strlen(key)-1] == '$');				\
    if(!h_stringmap_present(table, (uint8_t *)key, strlen(key), end)) {	\
      g_test_message("%s line %d: Check failed: \"%s\" should have been in map, but wasn't", key); \
		     __FILE__, __LINE__,				\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_stringmap_absent(table, key) do {			\
    bool end = (key[strlen(key)-1] == '$');				\
    if(h_stringmap_present(table, (uint8_t *)key, strlen(key), end)) {	\
      g_test_message("%s line %d: Check failed: \"%s\" shouldn't have been in map, but was", key); \
		     __FILE__, __LINE__,				\
      g_test_fail();							\
    }									\
  } while(0)


// This stuff needs to be made internal-only; it has no use in user-level test suites
#define g_check_terminal(grammar, parser) \
  g_check_hashtable_absent(grammar->nts, h_desugar(&system_allocator, NULL, parser))

#define g_check_nonterminal(grammar, parser) \
  g_check_hashtable_present(grammar->nts, h_desugar(&system_allocator, NULL, parser))

#define g_check_derives_epsilon(grammar, parser) \
  g_check_hashtable_present(grammar->geneps, h_desugar(&system_allocator, NULL, parser))

#define g_check_derives_epsilon_not(grammar, parser) \
  g_check_hashtable_absent(grammar->geneps, h_desugar(&system_allocator, NULL, parser))

#define g_check_firstset_present(k, grammar, parser, str) \
  g_check_stringmap_present(h_first(k, grammar, h_desugar(&system_allocator, NULL, parser)), str)

#define g_check_firstset_absent(k, grammar, parser, str) \
  g_check_stringmap_absent(h_first(k, grammar, h_desugar(&system_allocator, NULL, parser)), str)

#define g_check_followset_present(k, grammar, parser, str) \
  g_check_stringmap_present(h_follow(k, grammar, h_desugar(&system_allocator, NULL,  parser)), str)

#define g_check_followset_absent(k, grammar, parser, str) \
  g_check_stringmap_absent(h_follow(k, grammar, h_desugar(&system_allocator, NULL, parser)), str)




#define g_check_cmp_int32(n1, op, n2) g_check_inttype("%d", int32_t, n1, op, n2)
#define g_check_cmp_int64(n1, op, n2) g_check_inttype("%" PRId64, int64_t, n1, op, n2)
#define g_check_cmp_uint32(n1, op, n2) g_check_inttype("%u", uint32_t, n1, op, n2)
#define g_check_cmp_uint64(n1, op, n2) g_check_inttype("%" PRIu64, uint64_t, n1, op, n2)
#define g_check_cmpfloat(n1, op, n2) g_check_inttype("%g", float, n1, op, n2)
#define g_check_cmpdouble(n1, op, n2) g_check_inttype("%g", double, n1, op, n2)



#endif // #ifndef HAMMER_TEST_SUITE__H


