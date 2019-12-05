/** \file
 *
 * Netpp property name to C identifier mangling (and inverse)
 *
 * <hackfin@section5.ch>
 *
 */

#include <string>
#include <stdint.h>
#include <stdio.h>
#include "pname_mangler.h"

// The upper MSBs are used for escape coding
#define IDENTICAL  0x00
#define ESCAPE     0x80
#define ESCESC     0x40
#define INVALID    0xc0
// The LUT mask for mangled codes. 63 positions are sufficient...
#define LUT_MASK   0x3f

// The Escape character
#define C_ESC '_'

struct ascii_range {
	uint8_t from;
	uint8_t to;
} s_ranges[] = {

	{ '0', '9' },
	{ 'a', 'z' },
	{ 'A', 'Z' },
	{ 0, 0 }
};

uint8_t s_lut[256];
uint8_t s_invlut[256];

// These characters are valid in a Property name:
const unsigned char  s_table[] = " .@#$'()[]:;&*%-+";
// The escape codes for the above:
const unsigned char s_esctab[] = "wdahzqbtceskfmonp";

/** Initialize LUT
 * with valid/escaped characters
 */
static
void lut_init(void)
{
	unsigned int i;
	struct ascii_range *r;
		

	for (i = 0; i < sizeof(s_lut); i++) {
		s_lut[i] = INVALID;
		s_invlut[i] = INVALID;
	}
	
	for (i = 0; i < sizeof(s_table); i++) {
		s_lut[s_table[i]] = ESCAPE | (s_esctab[i] - 'a');
		s_invlut[s_esctab[i]] = s_table[i];
	}

	s_lut[C_ESC] = ESCESC;
	s_invlut[C_ESC] = C_ESC;

	r = &s_ranges[0];
	while (r->from) {
		for (i = r->from; i <= r->to; i++) {
			s_lut[i] = IDENTICAL;
		}
		r++;
	}
}

int mangler_init(void)
{
	lut_init();
	return 0;
}

/**
 * Mangle string into C compatible identifier
 * \param      pn    Property name string
 * \param      cn    C identifier output string (appending)
 */
int mangle_cname(const QString &pn, QString &cn)
{
	unsigned char c;
	unsigned int i, end;
	int ret = 0;
	i = 0;

	if (!pn.startsWith("_")) { return -1; }


	for (end = pn.length(); i != end; i++) {
		c = pn.at(i).toLatin1();
		switch (s_lut[c] & 0xc0) {
			case IDENTICAL:
				cn += c; break;
			case ESCAPE:
				cn += C_ESC;
				cn += (s_lut[c] & LUT_MASK) + 'a';
				break;
			case ESCESC:
				cn += C_ESC; cn += C_ESC;
				break;
			case INVALID:
				ret = i + 1;
				cn += '%'; break;
			default:
				return -1;
		}
	}
	return ret;
}

/**
 * Demangle C identifier into back into a Property name
 *
 * \param      cn    C identifier string
 * \param      pn    Property name string (appending)
 */

int demangle_cname(const QString &cn, QString &pn)
{
	enum {
		S_ID,
		S_ESC
	} state = S_ID;

	if (!cn.startsWith("_")) { return 0; }

	pn.clear();

	int end;
	unsigned char c;
	int i;

	for (i = 0, end = cn.length(); i != end; i++) {
		c = cn.at(i).toLatin1();
		switch (state) {
			case S_ID:
				if (c == C_ESC) {
					state = S_ESC;
				} else {
					pn += c;
				}
				break;
			case S_ESC:
				if (c == C_ESC) {
					pn += C_ESC;
				} else {
					pn += s_invlut[c];
				}
				state = S_ID;
				break;
		}
	}
	return 1;
}

#ifdef TEST

int main(int argc, char **argv)
{
	QString s = "Mgmt_C.Array[0]:test(0)::a";
	QString cname;
	QString pname;
	int ret;

	if (argc > 1) {
		s = argv[1];
	}

	lut_init();

	ret = mangle_cname(s, cname);
	if (ret) {
		fprintf(stderr, "Invalid characters in Property string at %d\n", ret-1);
	}
	printf("Cname: '%s'\n", cname.c_str());
	demangle_cname(cname, pname);
	printf("Pname: '%s'\n", pname.c_str());

	if (pname == s) {
		printf("Conversion ok\n");
	} else {
		fprintf(stderr, "Conversion failed\n");
		return -1;
	}
	return 0;
}
	
#endif
