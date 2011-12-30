/* Copyright (C) 2006-2008 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPTIONMACROS_H
#define OPTIONMACROS_H

#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <climits>

using namespace std;

/* INSTRUCTIONS:
- A fatal routine should be provided.
- If gettext is required, define the _ macro. Otherwise, the empty definition
  here will be used. However, if the macro USE_GETTEXT is defined, the automatic
  definition in this file will not be used to make sure a wrong order of
  definitions is detected.
- Option parsing may change the argument vector. If this should not happen,
  define the macro OPTION_STRDUP such that it allocates a copy of the string
  passed to it.
- Printing options should be done by using the "%.*s" format and
  "(int) optlength, optcurrent" arguments or even better, the OPTFMT format
  macor and OPTPRARG arg.

A simple example argument parser is shown below:

PARSE_FUNCTION(parse_options)
	OPTIONS
		OPTION('f', "long-f", REQUIRED_ARG)
		END_OPTION
		printf("Unkown option: %s\n", optcurrent);
	NO_OPTION
		printf("Non-option argument: %s\n", optcurrent);
	END_OPTIONS
END_FUNCTION

*/


/* Definitions to make the macro's work regardless of configuration. */
#if !defined _ && !defined USE_GETTEXT
#define _(_x) (_x)
#endif

#ifndef OPTION_STRDUP
#define OPTION_STRDUP(_x) (_x)
#define OPTION_FREE(_x) (void) optptr
#else
#define OPTION_FREE(_x) free(_x)
#endif

/** Format string for printing options. */
#define OPTFMT "%.*s"
/** Arguments for printf style functions, to be used in combination with @a OPTFMT. */
#define OPTPRARG (int) optlength, optcurrent

/** Define an option parsing function.
	@param name The name of the function to define.
*/
#define PARSE_FUNCTION(name) void name(int argc, char **argv) {\
	char *optArg; \
	int optargind; \
	int optnomore = 0;

/** Declare a chile option parsing function.
	@param name The name of the function to define.
*/
#define CHILD_PARSE_FUNCTION_DECL(name) int name(int argc, char **argv, char *optcurrent, char *optArg, size_t optlength, arg_type_t opttype, int _optargind);

/** Define a chile option parsing function.
	@param name The name of the function to define.
*/
#define CHILD_PARSE_FUNCTION(name) int name(int argc, char **argv, char *optcurrent, char *optArg, size_t optlength, arg_type_t opttype, int _optargind) { \
	int optargind = _optargind, optcontrol = 0;

/** Signal the end of a child option parsing function. */
#define END_CHILD_FUNCTION return -1; check_next: if (optargind != _optargind) return 4; return optcontrol; }

/** Call a child option parsing function.
	@param name The name of the function to call.
*/
#define CALL_CHILD(name) do { int retval = name(argc, argv, optcurrent, optArg, optlength, opttype, optargind); \
	if (retval == -1) break; \
	else if (retval == 4) optargind++; \
	else if (retval == 1) optcontrol++; \
	goto check_next; } while (0)

/** Indicate the start of option processing.

	This is separate from @a PARSE_FUNCTION so that local variables can be
	defined.
*/
#define OPTIONS \
	for (optargind = 1; optargind < argc; optargind++) { \
		char optcontrol = 0; \
		char *optcurrent, *optptr; \
		optcurrent = argv[optargind]; \
		if (optcurrent[0] == '-' && !optnomore) { \
			size_t optlength; \
			arg_type_t opttype; \
\
			if (optcurrent[1] == '-') { \
				if ((optArg = strchr(optcurrent, '=')) == NULL) { \
					optlength = strlen(optcurrent); \
				} else { \
					optlength = optArg - optcurrent; \
					optArg++; \
					if (*optArg == 0) { \
						fatal(_("Option " OPTFMT " has zero length argument\n"), OPTPRARG); \
					} \
				} \
				opttype = LONG; \
			} else { \
				optlength = 2; \
				if (optcurrent[1] != 0 && optcurrent[2] != 0) \
					optArg = optcurrent + 2; \
				else \
					optArg = NULL; \
				opttype = SHORT; \
			} \
			if (optlength > INT_MAX) optlength = INT_MAX; \
			next_opt:
/* The last line above is to make sure the cast to int in error messages does
   not overflow. */

/** Signal the start of non-switch option processing. */
#define NO_OPTION } else {

/** Signal the end of option processing. */
#define END_OPTIONS check_next: if (optcontrol == 1 || optcontrol == 3) { \
		if (optcontrol == 1) { \
			optptr = optcurrent = OPTION_STRDUP(optcurrent); \
		} \
		optcontrol = 2; \
		optcurrent++; \
		optcurrent[0] = '-'; \
		optArg = optcurrent[2] != 0 ? optcurrent + 2 : NULL; \
		goto next_opt; \
	} else if (optcontrol == 2) { \
		OPTION_FREE(optptr); \
	} }}

/** Signal the end of the option processing function. */
#define END_FUNCTION }

/** Internal macro to check whether the requirements regarding option arguments
		have been met. */
#define CHECK_ARG(argReq) \
	switch(argReq) { \
		case NO_ARG: \
			if (optArg != NULL) { \
				if (opttype == SHORT) { \
					optcontrol++; \
					optArg = NULL; \
				} else { \
					fatal(_("Option " OPTFMT " does not take an argument\n"), OPTPRARG); \
				} \
			} \
			break; \
		case REQUIRED_ARG: \
			if (optArg == NULL && (optargind+1 >= argc)) { \
				fatal(_("Option " OPTFMT " requires an argument\n"), OPTPRARG); \
			} \
			if (optArg == NULL) optArg = argv[++optargind]; \
			break; \
		default: \
			break; \
	}

/** Check for a short style (-o) option.
	@param shortName The name of the short style option.
	@param argReq Whether or not an argument is required/allowed. One of NO_ARG,
		OPTIONAL_ARG or REQUIRED_ARG.
*/
#define SHORT_OPTION(shortName, argReq) if (opttype == SHORT && optcurrent[1] == shortName) { CHECK_ARG(argReq) {

/** Check for a single dash as option.

	This is usually used to signal standard input/output.
*/
#define SINGLE_DASH SHORT_OPTION('\0', NO_ARG)

/** Check for a double dash as option.

	This is usually used to signal the end of options.
*/
#define DOUBLE_DASH LONG_OPTION("", NO_ARG)

/** Check for a short style (-o) or long style (--option) option.
	@param shortName The name of the short style option.
	@param longName The name of the long style option.
	@param argReq Whether or not an argument is required/allowed. One of NO_ARG,
		OPTIONAL_ARG or REQUIRED_ARG.
*/
#define OPTION(shortName, longName, argReq) if ((opttype == SHORT && optcurrent[1] == shortName) || (opttype == LONG && strlen(longName) == optlength - 2 && strncmp(optcurrent + 2, longName, optlength - 2) == 0)) { CHECK_ARG(argReq) {

/** Check for a long style (--option) option.
	@param longName The name of the long style option.
	@param argReq Whether or not an argument is required/allowed. One of NO_ARG,
		OPTIONAL_ARG or REQUIRED_ARG.
*/
#define LONG_OPTION(longName, argReq) if (opttype == LONG && strlen(longName) == optlength - 2 && strncmp(optcurrent + 2, longName, optlength - 2) == 0) { CHECK_ARG(argReq) {

/** Signal the end of processing for the previous (SHORT_|LONG_)OPTION. */
#define END_OPTION } goto check_next; }

/** Check for presence of a short style (-o) option and set the variable if so.
	@param shortName The name of the short style option.
	@param var The variable to set.
*/
#define BOOLEAN_SHORT_OPTION(shortName, var) SHORT_OPTION(shortName, NO_ARG) var = 1; END_OPTION

/** Check for presence of a long style (--option) option and set the variable
		if so.
	@param longName The name of the long style option.
	@param var The variable to set.
*/
#define BOOLEAN_LONG_OPTION(longName, var) LONG_OPTION(longName, NO_ARG) var = 1; END_OPTION

/** Check for presence of a short style (-o) or long style (--option) option
		and set the variable if so.
	@param shortName The name of the short style option.
	@param longName The name of the long style option.
	@param var The variable to set.
*/
#define BOOLEAN_OPTION(shortName, longName, var) OPTION(shortName, longName, NO_ARG) var = 1; END_OPTION

/** Tell option processor that all further arguments are non-option arguments. */
#define NO_MORE_OPTIONS do { optnomore = 1; } while(0)

/** Check an option argument for an integer value.
	@param var The variable to store the result in.
	@param min The minimum allowable value.
	@param max The maximum allowable value.
*/
#define PARSE_INT(var, min, max) do {\
	char *endptr; \
	long value; \
	errno = 0; \
	\
	value = strtol(optArg, &endptr, 10); \
	if (*endptr != 0) { \
		fatal(_("Garbage after value for " OPTFMT " option\n"), OPTPRARG); \
	} \
	if (errno != 0 || value < min || value > max) { \
		fatal(_("Value for " OPTFMT " option (%ld) is out of range\n"), OPTPRARG, value); \
	} \
	var = (int) value; } while(0)

#define PARSE_BOOLEAN(var) do {\
	if (optArg == NULL || strcmp(optArg, "true") == 0 || strcmp(optArg, "t") == 0 || strcmp(optArg, "yes") == 0 || \
			strcmp(optArg, "y") == 0 || strcmp(optArg, "1") == 0) \
		(var) = 1; \
	else if (strcmp(optArg, "false") == 0 || strcmp(optArg, "f") == 0 || strcmp(optArg, "no") == 0 || \
			strcmp(optArg, "n") == 0 || strcmp(optArg, "0") == 0) \
		(var) = 0; \
	else \
		fatal(_("Value for " OPTFMT " option (%s) is not a valid boolean value\n"), OPTPRARG, optArg); \
	} while (0)

typedef enum {
	SHORT,
	LONG
} arg_type_t;

enum {
	NO_ARG,
	OPTIONAL_ARG,
	REQUIRED_ARG
};

#endif
