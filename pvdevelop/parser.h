
#include <stdio.h>

#define MAX_ARGS 16

struct param_t {
	const char *cval;
	const char *text;
	int ival[MAX_ARGS-1];
	const char *parent_id;
	const char *this_id;
};


typedef struct create_context {
	QStringList strlist;
	QStringList enumlist;
	QStringList tooltiplist;
	QStringList whatsthislist;
	QWidget *pw;
	const char *line; // XXX Should leave
	const char **argv;
	dlgeditlayout *editlayout;
	struct param_t *params;
	int iitem;
	int *socket;
} C;

extern int NULL_SOCKET;

class PvCppParser {
	FILE *file;

	bool mangling;
	char line[512];

	char words[512];
	int argc;
	const char *argv[MAX_ARGS];
	
	enum {
		T_UNDEF,
		T_NUM,
		T_IDENTIFIER,
		T_STRING,
	} argt[MAX_ARGS];

	QWidget *item;
	int itemtype;
	int iitem;

	struct param_t parameters;

	struct create_context widget_context;

	enum {
		S_START,
		S_NEUTRAL,
		S_ENUM,
		S_TOOLTIP,
		S_HELP,
		S_DEFINITION,
		S_END_DEF,
		S_CONSTRUCTOR,
		S_PARAM,
		S_END,
	} state;

	enum {
		S_NONE,
		S_FUNCNAME,
		S_IDENTIFIER,
		S_FUNC_ARG,
		S_ARGS,
		S_STRING,
		S_NUMBER,
		S_ESCAPE,
		S_ERROR,
		S_FINISH,
	} line_state;

	QWidget *this_root;

	QWidget *findChild(const char *id);

	int parseLine();
	int convertArgs(int mode);
	int readWidget(QWidget *root);
	int readParameters(QWidget *item, int itemtype);

	int appendEnum();
	int appendToolTip();
	int appendWhatsThis();

	bool debug;

	int set_parameter(int method, QWidget *item, int itemtype);


public:
	PvCppParser(FILE *f, dlgeditlayout *editlayout) {
		file = f;
		state = S_START;
		debug = true;
		mangling = false;
		widget_context.socket = &NULL_SOCKET;
		widget_context.editlayout = editlayout;
	}

	int parse(QWidget *root);

};


