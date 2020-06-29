// See the file "COPYING" in the main distribution directory for copyright.


#pragma once

// These are in a separate file to break circular dependences 
typedef enum {
	STMT_ANY = -1,
	STMT_ALARM, // Does no longer exist but kept to create enums consistent.
	STMT_PRINT, STMT_EVENT,
	STMT_EXPR,
	STMT_IF, STMT_WHEN, STMT_SWITCH,
	STMT_FOR, STMT_NEXT, STMT_BREAK,
	STMT_RETURN,
	STMT_ADD, STMT_DELETE,
	STMT_LIST, STMT_EVENT_BODY_LIST,
	STMT_INIT,
	STMT_FALLTHROUGH,
	STMT_WHILE,
	STMT_NULL
#define NUM_STMTS (int(STMT_NULL) + 1)
} BroStmtTag;

typedef enum {
	FLOW_NEXT,		// continue on to next statement
	FLOW_LOOP,		// go to top of loop
	FLOW_BREAK,		// break out of loop
	FLOW_RETURN,		// return from function
	FLOW_FALLTHROUGH	// fall through to next switch case
} stmt_flow_type;

extern const char* stmt_name(BroStmtTag t);
