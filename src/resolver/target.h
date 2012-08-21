#ifndef __MURPHY_RESOLVER_TARGET_H__
#define __MURPHY_RESOLVER_TARGET_H__

#include <stdarg.h>

#include "resolver-types.h"
#include "resolver.h"
#include "parser-api.h"

int create_targets(mrp_resolver_t *r, yy_res_parser_t *parser);
void dump_targets(mrp_resolver_t *r, FILE *fp);
int update_target_by_name(mrp_resolver_t *r, const char *name, va_list ap);
int update_target_by_id(mrp_resolver_t *r, int id, va_list ap);

#endif /* __MURPHY_RESOLVER_TARGET_H__ */