
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "pname_mangler.h"

// #define TEST

int NULL_SOCKET = -1;


#ifdef TEST

typedef struct {
} QWidget;

typedef struct {
} QString;

typedef struct {
} QStringList;

typedef struct create_context {
	QWidget *pw;
	const char *id;
	void *editlayout;
	struct param_t *params;
	int iitem;
	int s;
} C;

#else

#include <QStringList>
#include <QWidget>

#include "qdrawwidget.h"
#include "interpreter.h"
#include "dlginsertwidget.h"
#include "dlgeditlayout.h"
#include "qlcdnumber.h"
#include "pvserver.h"

#include "parser.h"

#endif

#define TAG_BEGIN  "_begin_of_generated_area_"

enum {
	SET_GEOMETRY,
	SET_TABPOS,
	SET_EDITABLE,
	SET_PALETTEFGCOL,
	SET_PALETTEBGCOL,
	SET_TEXT,
	SET_ALIGNMENT,
	SET_CHECKED,
	SET_ECHOMODE,
	SET_MULTISEL,
	SET_FONT,
	SET_STYLE,
	SET_TOOLTIP,
	SET_ADDTAB,
	SET_MINSIZE,
	SET_MAXSIZE,
	SET_DLFILE,
	SET_ADDWIDGET_OR_LAYOUT,
	SET_ADDSTRETCH,
	SET_GLBEGIN,
	SET_GLEND,
	SET_WHATSTHIS
};

bool contains(const char *a, const char *b)
{
	return (strstr(a, b));
}

static int isHorizontal(const char *cval)
{
	if (strcmp(cval,"Horizontal") == 0) return 1;
	if (strcmp(cval,"HORIZONTAL") == 0) return 1;
	if (strcmp(cval,"horizontal") == 0) return 1;
	return 0;
}

/** XXX Deprecated. We should rather maintain an index table to
 * retrieve added widgets.
 */

QWidget *PvCppParser::findChild(const char *id)
{
	QWidget *ret;
#ifdef PVWIN32
#if QT_VERSION < 0x050000		 
	QString txt = id;
	ret = qFindChild<QWidget *>(this_root, txt);
#else
	ret = this_root->findChild<QWidget *>(id);
#endif
#else
	ret = this_root->findChild<QWidget *>(id);
#endif
	if(ret == NULL)
		printf("%s(%s) ret == NULL\n", __FUNCTION__, id);
	return ret;

}


////////////////////////////////////////////////////////////////////////////
// Initializing enums, etc.

int PvCppParser::appendEnum()
{
	char *cptr_begin, *cptr_end;
	QStringList &enumlist = widget_context.enumlist;

	cptr_begin = &line[0];
	while(*cptr_begin==' ') cptr_begin++;
	cptr_end = cptr_begin;
	while(*cptr_end != ' ' && *cptr_end != ',' && *cptr_end != '\n' && *cptr_end != '\0') cptr_end++;
	*cptr_end = '\0';
	enumlist.append(cptr_begin);
	if(debug && cptr_begin != NULL)
	{
		printf("appendEnum:");
		printf("%s\n",cptr_begin);
	}
	return 0;
}

int PvCppParser::appendToolTip()
{
	char *cptr_begin, *cptr_end;

	QStringList &tooltiplist = widget_context.tooltiplist;

	cptr_begin = strchr(line,'\"');
	if(cptr_begin == NULL) cptr_end = NULL;
	else									 cptr_end = strrchr(line,'\"');
	if(cptr_begin != NULL)
	{
		cptr_begin++;
		if(cptr_end != NULL)
		{
			*cptr_end = '\0';
			tooltiplist.append(QString::fromUtf8(cptr_begin));
		}
		else tooltiplist.append("error");
	}
	else tooltiplist.append("error");
	if(debug && cptr_begin != NULL)
	{
		printf("appendToolTip:");
		printf("%s\n",cptr_begin);
	}
	return 0;
}

int PvCppParser::appendWhatsThis()
{
	char *cptr_begin, *cptr_end;

	QStringList &whatsthislist = widget_context.whatsthislist;

	cptr_begin = strchr(line,'\"');
	if(cptr_begin == NULL) cptr_end = NULL;
	else									 cptr_end = strrchr(line,'\"');
	if(cptr_begin != NULL)
	{
		cptr_begin++;
		if(cptr_end != NULL)
		{
			*cptr_end = '\0';
			whatsthislist.append(QString::fromUtf8(cptr_begin));
		}
		else whatsthislist.append("error");
	}
	else whatsthislist.append("error");
	if(debug && cptr_begin != NULL)
	{
		printf("appendWhatsThis:");
		printf("%s\n",cptr_begin);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// CREATORS

#ifndef TEST

int create_vbox(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	if (c->editlayout == NULL) return -1;

	QString txt = QString::fromUtf8(c->line);
	txt.remove(" "); txt.remove("\t"); txt.remove("\n"); // XXX
	c->editlayout->uidlg->textEditConstructors->append(txt);
	if		 (strcmp(c->argv[1], "0") == 0)
		*item = (QWidget *) new QVBoxLayout();
	else if (strcmp(c->argv[1],"ID_MAIN_WIDGET") == 0)
		*item = (QWidget *) new QVBoxLayout();
	else
		*item = (QWidget *) new QVBoxLayout(c->pw);

	*itemtype = TQVbox;
	return 0;
}

int create_hbox(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	if (c->editlayout == NULL) return -1;

	QString txt = QString::fromUtf8(c->line);
	txt.remove(" "); txt.remove("\t"); txt.remove("\n");
	c->editlayout->uidlg->textEditConstructors->append(txt);
	if		 (strcmp(c->argv[1], "0") == 0)
		*item = (QWidget *) new QHBoxLayout();
	else if (strcmp(c->argv[1],"ID_MAIN_WIDGET") == 0)
		*item = (QWidget *) new QHBoxLayout();
	else
		*item = (QWidget *) new QHBoxLayout(c->pw);

	*itemtype = TQHbox;
	return 0;
}

int create_lgrid(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	if (c->editlayout == NULL) return -1;

	QString txt = QString::fromUtf8(c->line);
	txt.remove(" "); txt.remove("\t"); txt.remove("\n");
	c->editlayout->uidlg->textEditConstructors->append(txt);
	if		 (strcmp(c->argv[1], "0") == 0)
		*item = (QWidget *) new QGridLayout();
	else if (strcmp(c->argv[1],"ID_MAIN_WIDGET") == 0)
		*item = (QWidget *) new QGridLayout();
	else
		*item = (QWidget *) new QGridLayout(c->pw);

	*itemtype = TQGrid;
	return 0;
}

int create_widget(C *c, const char *id,
	QWidget **item, int *itemtype)
{

	if (!c->pw) {
		return -1;
	}

	if (c->pw->statusTip().startsWith("TQTabWidget:")) {
		QList<QObject *> childs;
		childs.clear();
		childs = c->pw->children();
		if (childs.count() > 0) {
			QWidget *q = (QWidget *) new MyQWidget(c->socket, 0,
				(QWidget *) childs.at(0), id);
			q->setStatusTip("TQWidget:");
			*item = q;
		} else {
			printf("TQTabWidget: childs->count() <= 0\n");
			return -1;
		}
	} else if (c->pw->statusTip().startsWith("TQToolBox:")) {
		QWidget *q = (QWidget *) new MyQWidget(c->socket, 0, c->pw, id);
		q->setStatusTip("TQWidget:");
		*item = q;
	}

	*itemtype = TQWidget;
	return 0;
}

int create_label(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyLabel(c->socket, 0, c->pw, id);
	*itemtype = TQLabel;
	q->setStatusTip("TQLabel:");
	q->setAutoFillBackground(true);
	*item = q;
	return 0;
}

int create_combo(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q  = (QWidget *) new MyComboBox(c->socket, 0, c->pw, id);
	MyComboBox *comboitem = (MyComboBox *) q;
	const char *mode = c->argv[5];
	*itemtype = TQComboBox;
	if(c->params->ival[0] == 1) comboitem->setEditable(1);
	else						 comboitem->setEditable(0);
	int insertionpolicy = (int) QComboBox::InsertAtBottom;
	if     (contains(mode,",NoInsertion)"))
		insertionpolicy = (int) QComboBox::NoInsert;
	else if(contains(mode,",AtTop)"))
		insertionpolicy = (int) QComboBox::InsertAtTop;
	else if(contains(mode,",AtCurrent)"))
		insertionpolicy = (int) QComboBox::InsertAtCurrent;
	else if(contains(mode,",AtBottom)"))
		insertionpolicy = (int) QComboBox::InsertAtBottom;
	else if(contains(mode,",AfterCurrent)"))
		insertionpolicy = (int) QComboBox::InsertAfterCurrent;
	else if(contains(mode,",BeforeCurrent)"))
		insertionpolicy = (int) QComboBox::InsertBeforeCurrent;

	comboitem->setInsertPolicy((QComboBox::InsertPolicy) insertionpolicy);
	q->setStatusTip("TQComboBox:");
	*item = q;
	return 0;
}


int create_lineedit(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyLineEdit(c->socket, 0, c->pw, id);
	*itemtype = TQLineEdit;
	q->setStatusTip("TQLineEdit:");
	*item = q;
	return 0;
}

int create_pushbutton(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQPushButton(c->socket, 0, c->pw, id);
	*itemtype = TQPushButton;
	q->setStatusTip("TQPushButton:");
	*item = q;
	return 0;
}

int create_lcdnumber(C *c, const char *id,
	QWidget **item, int *itemtype)
{

	QWidget *q = (QWidget *) new QLCDNumber(c->pw);
	QLCDNumber *lcditem = (QLCDNumber *) q;
	q->setObjectName(id);
	const char *mode = c->argv[5];
#if QT_VERSION < 0x050000			 
	lcditem->setNumDigits(c->params->ival[0]);
#else
	lcditem->setDigitCount(c->params->ival[0]);
#endif
	if (contains(mode,"Hex"))
		lcditem->setMode(QLCDNumber::Hex);
	else if (contains(mode,"Dec"))
		lcditem->setMode(QLCDNumber::Dec);
	else if (contains(mode,"Oct"))
		lcditem->setMode(QLCDNumber::Oct);
	else if (contains(mode,"Bin"))
		lcditem->setMode(QLCDNumber::Bin);
	else if (contains(mode,"Outline"))
		lcditem->setSegmentStyle(QLCDNumber::Outline);
	else if (contains(mode,"Filled"))
		lcditem->setSegmentStyle(QLCDNumber::Filled);
	else if (contains(mode,"Flat"))
		lcditem->setSegmentStyle(QLCDNumber::Flat);

	*itemtype = TQLCDNumber;
	q->setStatusTip("TQLCDNumber:");
	*item = q;

	return 0;
}

int create_slider(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int ori;
	if (isHorizontal(c->argv[8]))
		ori = Qt::Horizontal;
	else
		ori = Qt::Vertical;

	QWidget *q = (QWidget *) new MySlider(c->socket,
		0, c->params->ival[0], c->params->ival[1], c->params->ival[2],
		c->params->ival[3],
		(Qt::Orientation) ori, c->pw, id);
	*itemtype = TQSlider;
	q->setStatusTip("TQSlider:");
	*item = q;

	return 0;
}

int create_buttongrp(C *c, const char *id,
	QWidget **item, int *itemtype)
{

	int ori;
	if (isHorizontal(c->params->cval))
		ori = Qt::Horizontal;
	else
		ori = Qt::Vertical;

	QWidget *q = (QWidget *) new MyButtonGroup(c->socket, 0,
		c->params->ival[0], (Qt::Orientation) ori,
		c->argv[6], c->pw, id);
	*itemtype = TQButtonGroup;
	q->setStatusTip("TQButtonGroup:");
	*item = q;

	return 0;
}

int create_radio(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyRadioButton(c->socket, 0, c->pw, id);
	*itemtype = TQRadio;
	q->setStatusTip("TQRadio:");
	q->setAutoFillBackground(true);
	*item = q;
	return 0;
}

int create_checkbox(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyCheckBox(c->socket, 0, c->pw, id);
	*itemtype = TQCheck;
	q->setStatusTip("TQCheck:");
	q->setAutoFillBackground(true);
	*item = q;
	return 0;
}

static int frame_style(const char *shapespec, const char *stylespec,
	int *shape, int *shadow)
{
	if (contains(shapespec,"-1"))     *shape = -1;
	else if (contains(shapespec,"NoFrame"))     *shape = QFrame::StyledPanel;
	else if (contains(shapespec,"StyledPanel")) *shape = QFrame::StyledPanel;
	else if (contains(shapespec,"VLine"))       *shape = QFrame::VLine;
	else if (contains(shapespec,"HLine"))       *shape = QFrame::HLine;
	else if (contains(shapespec,"WinPanel"))    *shape = QFrame::WinPanel;
	else if (contains(shapespec,"Panel"))       *shape = QFrame::Panel;
	else if (contains(shapespec,"Box"))         *shape = QFrame::Box;

	if (contains(stylespec,"-1"))     *shadow = -1;
	if      (contains(stylespec,"Raised"))  *shadow = QFrame::Raised;
	else if (contains(stylespec,"Sunken"))  *shadow = QFrame::Sunken;
	else if (contains(stylespec,"Sunken"))  *shadow = QFrame::Sunken;
	else if (contains(stylespec,"Plain"))   *shadow = QFrame::Plain;

	return 0;
}

int create_frame(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int shape;
	int shadow;
	int line_width   = atoi(c->argv[6]);
	int margin		 = atoi(c->argv[7]);


	frame_style(c->argv[4], c->argv[5], &shape, &shadow);

	QWidget *q = (QWidget *) new MyFrame(c->socket, 0,
		shape, shadow, line_width, margin, c->pw, id);
	*itemtype = TQFrame;
	*item = q;
	q->setStatusTip("TQFrame:");
	return 0;
}

int create_draw(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new QDrawWidget(c->pw, id);
	*itemtype = TQDraw;
	q->setStatusTip("TQDraw:");
	QString fname = c->whatsthislist.at(c->iitem + 1);
	fname.replace('\\', "");
	q->setWhatsThis(fname);
	*item = q;
	return 0;
}

int create_image(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QImageWidget *) new QImageWidget(c->socket, 0, c->pw, id);
	printf("insert image id %s\n", id);
	QString qtext = QString(c->argv[4]);
	*itemtype = TQImage;
	q->setStatusTip("TQImage:");
	qtext.replace('\\', "");
	q->setWhatsThis(qtext);
	if (qtext.length() > 0) {
		printf("Image '%s'\n", (const char *) qtext.toUtf8());
		((QImageWidget *) q)->setImage(qtext.toUtf8());
	}
	*item = q;
	return 0;
}

int create_gl(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int shape = QFrame::Panel;
	int shadow = QFrame::Raised;
	int line_width = 10;
	int margin = 1; // not used in Qt4
	QWidget *q = new MyFrame(c->socket,0,(QFrame::Shape) shape, (QFrame::Shadow) shadow,line_width,margin,c->pw);
	//q = (QWidget *) new PvGLWidget(pw, 0, &s);
	q->setObjectName(id);
	*itemtype = TQGl;
	q->setStatusTip("TQGl:");
	*item = q;

	return 0;

}

int create_tabwidget(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQTabWidget(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQTabWidget;
	q->setStatusTip("TQTabWidget:");
	*item = q;

	return 0;
}

int create_toolbox(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQToolBox(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQToolBox;
	q->setStatusTip("TQToolBox:");
	*item = q;

	return 0;
}

int create_group(C *c, const char *id,
	QWidget **item, int *itemtype)
{

	int columns = c->params->ival[0];
	int ori;

	if(isHorizontal(c->argv[5]))
		ori = Qt::Horizontal;
	else
		ori = Qt::Vertical;

	QWidget *q = (QWidget *) new MyGroupBox(c->socket, 0, columns,
		(Qt::Orientation) ori, c->argv[6], c->pw);
	q->setObjectName(id);
	*itemtype = TQGroupBox;
	q->setStatusTip("TQGroupBox:");
	*item = q;

	return 0;
}

int create_listbox(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyListBox(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQListBox;
	q->setStatusTip("TQListBox:");
	*item = q;
	return 0;
}

int create_table(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyTable(c->socket, 0,
		c->params->ival[0], c->params->ival[1], c->pw);
	q->setObjectName(id);
	*itemtype = TQTable;
	q->setStatusTip("TQTable:");
	*item = q;
	return 0;
}

int create_spinbox(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int *i = c->params->ival;
	QWidget *q = (QWidget *) new MySpinBox(c->socket, 0,
		i[0], i[1], i[2], c->pw);
	q->setObjectName(id);
	*itemtype = TQSpinBox;
	q->setStatusTip("TQSpinBox:");
	*item = q;
	return 0;
}

int create_dial(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int *i = c->params->ival;
	QWidget *q = (QWidget *) new MyDial(c->socket, 0,
		i[0], i[1], i[2], i[3], c->pw);
	q->setObjectName(id);
	*itemtype = TQDial;
	q->setStatusTip("TQDial:");
	*item = q;
	return 0;
}

int create_progress(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int ori;

	if (isHorizontal(c->argv[5]))
		ori = Qt::Horizontal;
	else
		ori = Qt::Vertical;

	QWidget *q = (QWidget *) new MyProgressBar(c->socket, 0, c->params->ival[0], (Qt::Orientation) ori, c->pw);
	q->setObjectName(id);
	*itemtype = TQProgressBar;
	q->setStatusTip("TQProgressBar:");
	*item = q;
	return 0;
}

int create_mlineedit(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	char buf[80];
	QWidget *q = (QWidget *) new MyMultiLineEdit(c->socket,
		0, c->params->ival[0], c->params->ival[1], c->pw);
	q->setObjectName(id);
	sprintf(buf,"TQMultiLineEdit:maxlines=%d:", c->params->ival[1]);
	*itemtype = TQMultiLineEdit;
	q->setStatusTip("TQMultiLineEdit:");
	*item = q;
	return 0;
}

int create_textbrowser(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyTextBrowser(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQTextBrowser;
	q->setStatusTip("TQTextBrowser:");
	*item = q;
	return 0;
}

int create_listview(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyListView(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQListView;
	q->setStatusTip("TQListView:");
	*item = q;
	return 0;
}

int create_iconview(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyIconView(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQIconView;
	q->setStatusTip("TQIconView:");
	*item = q;
	return 0;
}

int create_tclwidget(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int shape = QFrame::Panel;
	int shadow = QFrame::Sunken;
	int line_width = 10;
	int margin = 1; // not used in Qt4
	QWidget *q = (QWidget *) new MyFrame(c->socket,
		0,(QFrame::Shape) shape,
		(QFrame::Shadow) shadow,line_width,margin,c->pw);
	*itemtype = TQVtk;
	q->setStatusTip("TQVtk:");
	*item = q;

	return 0;
}

int create_plotwidget(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new QwtPlotWidget(c->socket,
		0, c->pw, c->params->ival[0], c->params->ival[1]);
	q->setObjectName(id);
	*itemtype = TQwtPlotWidget;
	q->setStatusTip("TQwtPlotWidget:");
	*item = q;
	return 0;
}

int create_qwtscale(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int pos = c->params->ival[0];

	if      (contains(c->params->cval,"ScaleLeft") == 0)
		pos = PV::ScaleLeft;
	else if (contains(c->params->cval,"ScaleRight") == 0)
		pos = PV::ScaleRight;
	else if (contains(c->params->cval,"ScaleTop") == 0)
		pos = PV::ScaleTop;
	else if (contains(c->params->cval,"ScaleBottom") == 0)
		pos = PV::ScaleBottom;

	QWidget *q = (QWidget *) new MyQwtScale(c->socket,
		0,(QwtScaleDraw::Alignment) pos, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtScale;
	q->setStatusTip("TQwtScale:");
	*item = q;
	return 0;
}

int create_qwtthermo(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtThermo(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtThermo;
	q->setStatusTip("TQwtThermo:");
	*item = q;
	return 0;
}

int create_qwtknob(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtKnob(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtKnob;
	q->setStatusTip("TQwtKnob:");
	*item = q;
	return 0;
}

int create_counter(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtCounter(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtCounter;
	q->setStatusTip("TQwtCounter:");
	*item = q;
	return 0;
}

int create_wheel(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtWheel(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtWheel;
	q->setStatusTip("TQwtWheel:");
	*item = q;
	return 0;
}

int create_qwtslider(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtSlider(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtSlider;
	q->setStatusTip("TQwtSlider:");
	*item = q;
	return 0;
}

int create_qwtdial(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtDial(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtDial;
	q->setStatusTip("TQwtDial:");
	*item = q;
	return 0;
}

int create_compass(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtCompass(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtCompass;
	q->setStatusTip("TQwtCompass:");
	*item = q;
	return 0;
}

int create_analog(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQwtAnalogClock(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQwtAnalogClock;
	q->setStatusTip("TQwtAnalogClock:");
	*item = q;
	return 0;
}

int create_dateedit(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQDateEdit(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQDateEdit;
	q->setStatusTip("TQDateEdit:");
	*item = q;
	return 0;
}

int create_timeedit(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQTimeEdit(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQTimeEdit;
	q->setStatusTip("TQTimeEdit:");
	*item = q;
	return 0;
}

int create_datetimeedit(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	QWidget *q = (QWidget *) new MyQDateTimeEdit(c->socket, 0, c->pw);
	q->setObjectName(id);
	*itemtype = TQDateTimeEdit;
	q->setStatusTip("TQDateTimeEdit:");
	*item = q;
	return 0;
}

int create_custom(C *c, const char *id,
	QWidget **item, int *itemtype)
{
	int shape = QFrame::Panel;
	int shadow = QFrame::Raised;
	int line_width = 5;
	int margin = 1; // not used in Qt4
	QWidget *q = new MyFrame(c->socket, 
		0,(QFrame::Shape) shape,
		(QFrame::Shadow) shadow, line_width, margin, c->pw);
	q->setObjectName(id);
	*itemtype = TQCustomWidget;
	q->setStatusTip("TQCustomWidget:");
	*item = q;

	QLabel *xitem = new QLabel(q);
	xitem->setGeometry(5, 2, 4096, 50);

#warning "FIXME: create custom text not parsed"
	#if 0
	const char *cptr = strchr(line,'\"');
	if(cptr != NULL)
	{
		char *cptr2;
		if  (strchr(cptr,'\"') == NULL) cptr2 = NULL;
		else  cptr2 = strrchr(cptr,'\"');
		if(cptr2 != NULL) {
			cptr2++;
			*cptr2 = '\0';
			xitem->setText(cptr);
		}
	}
	#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// Parameter setters:


static const
struct lookup_s {
	const char *identifier;
	int (*create_method)(C *c, const char *id,
		QWidget **item, int *itemtype);
	int null;
} const_lookup[] = {
	{ "pvQLayoutVbox",     create_vbox,         },
	{ "pvQLayoutHbox",     create_hbox,         },
	{ "pvQLayoutGrid",     create_lgrid,        },
	{ "pvQWidget",         create_widget,       },
	{ "pvQLabel",          create_label,        },
	{ "pvQComboBox",       create_combo,        },
	{ "pvQLineEdit",       create_lineedit,     },
	{ "pvQPushButton",     create_pushbutton,   },
	{ "pvQLCDNumber",      create_lcdnumber,    },
	{ "pvQSlider",         create_slider,       },
	{ "pvQButtonGroup",    create_buttongrp,    },
	{ "pvQRadioButton",    create_radio,        },
	{ "pvQCheckBox",       create_checkbox,     },
	{ "pvQFrame",          create_frame,        },
	{ "pvQDraw",           create_draw,         },
	{ "pvQImage",          create_image,        },
	{ "pvQImageScript",    create_image,        },
	{ "pvQGL",             create_gl,           },
	{ "pvQTabWidget",      create_tabwidget,    },
	{ "pvQToolBox",        create_toolbox,      },
	{ "pvQGroupBox",       create_group,        },
	{ "pvQListBox",        create_listbox,      },
	{ "pvQTable",          create_table,        },
	{ "pvQSpinBox",        create_spinbox,      },
	{ "pvQDial",           create_dial,         },
	{ "pvQProgressBar",    create_progress,     },
	{ "pvQMultiLineEdit",  create_mlineedit,    },
	{ "pvQTextBrowser",    create_textbrowser,  },
	{ "pvQListView",       create_listview,     },
	{ "pvQIconView",       create_iconview,     },
	{ "pvQVtkTclWidget",   create_tclwidget,    },
	{ "pvQwtPlotWidget",   create_plotwidget,   },
	{ "pvQwtScale",        create_qwtscale,     },
	{ "pvQwtThermo",       create_qwtthermo,    },
	{ "pvQwtKnob",         create_qwtknob,      },
	{ "pvQwtCounter",      create_counter,      },
	{ "pvQwtWheel",        create_wheel,        },
	{ "pvQwtSlider",       create_qwtslider,    },
	{ "pvQwtDial",         create_qwtdial,      },
	{ "pvQwtCompass",      create_compass,      },
	{ "pvQwtAnalogClock",  create_analog,       },
	{ "pvQDateEdit",       create_dateedit,     },
	{ "pvQTimeEdit",       create_timeedit,     },
	{ "pvQDateTimeEdit",   create_datetimeedit, },
	{ "pvQCustomWidget",   create_custom,       },
	{ NULL, NULL, 0 },
};

#endif // TEST


static const
struct param_lut_s {
	const char *identifier;
	int method;
	int dummy;
} param_lookup[] = {
	{ "pvSetGeometry",                   SET_GEOMETRY,              },
	{ "pvSetTabPosition",                SET_TABPOS,                },
	{ "pvSetEditable",                   SET_EDITABLE,              },
	{ "pvSetPaletteForegroundColor",     SET_PALETTEFGCOL,          },
	{ "pvSetPaletteBackgroundColor",     SET_PALETTEBGCOL,          },
	{ "pvSetText",                       SET_TEXT,                  },
	{ "pvSetAlignment",                  SET_ALIGNMENT,             },
	{ "pvSetChecked",                    SET_CHECKED,               },
	{ "pvSetEchoMode",                   SET_ECHOMODE,              },
	{ "pvSetMultiSelection",             SET_MULTISEL,              },
	{ "pvSetFont",                       SET_FONT,                  },
	{ "pvSetStyle",                      SET_STYLE,                 },
	{ "pvToolTip",                       SET_TOOLTIP,               },
	{ "pvAddTab",                        SET_ADDTAB,                },
	{ "pvSetMinSize",                    SET_MINSIZE,               },
	{ "pvSetMaxSize",                    SET_MAXSIZE,               },
	{ "pvDownloadFile",                  SET_DLFILE,                },
	{ "pvAddWidgetOrLayout",             SET_ADDWIDGET_OR_LAYOUT,   },
	{ "pvAddStretch",                    SET_ADDSTRETCH             },
	{ "pvGlBegin",                       SET_GLBEGIN,               },
	{ "pvGlEnd",                         SET_GLEND,                 },
	{ "pvSetWhatsThis",                  SET_WHATSTHIS,             },
	{ NULL, 0, 0 },
};


////////////////////////////////////////////////////////////////////////////


int PvCppParser::parseLine()
{
	const char *walk = line;
	char *w = words;
	char c;
	int depth = 0;
	line_state = S_NONE;

	argc = 0;

	int i;
	for (i = 0; i < MAX_ARGS; i++) {
		argt[i] = T_UNDEF;
	}

	while ((c = *walk++)) {
		switch (line_state) {
			case S_NONE:
				switch (c) {
					case '\t':
					case ' ':
						break;
					case 'q':
					case 'p':
						line_state = S_FUNCNAME;
						argt[argc] = T_IDENTIFIER;
						argv[argc] = w;
						*w++ = c;
						break;
				}
				break;
			case S_FUNCNAME:
				switch (c) {
					case '(':
						argc++; *w++ = '\0';
						line_state = S_ARGS;
						break;
					default:
						if (isalpha(c)) {
							*w++ = c;
						} else {
							line_state = S_ERROR;
						}
				}
				break;
			case S_IDENTIFIER:
				switch (c) {
					case '(':
						depth++;
						line_state = S_FUNC_ARG;
						*w++ = c;
						break;
					case ')':
						*w++ = '\0';
						line_state = S_FINISH;
						break;
					case ',':
						argc++; *w++ = '\0';
						line_state = S_ARGS;
						break;
					case '_':
						*w++ = c;
						break;
					default:
						if (isalnum(c)) {
							*w++ = c;
						} else {
							line_state = S_ERROR;
						}
				}
				break;
			case S_FUNC_ARG:
				switch (c) {
					case '(':
						depth++;
						break;
					case ')':
						depth--;
						if (depth == 0) {
							line_state = S_ARGS;
							argc++;
							*w++ = '\0';
						}
						break;
					case '"':
						if (depth == 1 &&
							strncmp(argv[argc], "pvtr", 4) == 0) {
							// Reset pointer:
							w = (char *) argv[argc];
							line_state = S_STRING;
						}
						break;
					default:
						*w++ = c;
						break;
				}
				break;
			case S_ARGS:
				if (argc > MAX_ARGS) {
					fprintf(stderr, "arg array exceeded\n");
					return -1;
				}
				switch (c) {
					case '-':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						line_state = S_NUMBER;
						argv[argc] = w;
						argt[argc] = T_NUM;
						*w++ = c;
						break;
					case '"':
						argv[argc] = w;
						argt[argc] = T_STRING;
						line_state = S_STRING;
						break;
					case ',':
						argc++; *w++ = '\0';
						break;
					case ')':
						*w++ = '\0';
						line_state = S_FINISH;
						break;
					default:
						argv[argc] = w;
						argt[argc] = T_IDENTIFIER;
						line_state = S_IDENTIFIER;
						*w++ = c;
				}
				break;
			case S_NUMBER:
				switch (c) {
					case ',':
						line_state = S_ARGS;
						argc++; *w++ = '\0';
						break;
					case ')':
						line_state = S_FINISH;
						argc++; *w++ = '\0';
						break;
					default:
						if (!( (c >= '0') && (c <= '9') ) )
							line_state = S_ERROR;
						else
							*w++ = c;
				}
				break;
			case S_STRING:
				switch (c) {
					case '"':
						if (depth == 0) 
							line_state = S_ARGS;
						else
							line_state = S_FUNC_ARG;
						*w++ = '\0';
						break;
					case '\\':
						line_state = S_ESCAPE;
						break;
					default:
						*w++ = c;
				}
				break;
			case S_ESCAPE:
				switch (c) {
					case '"':
					case '\\':
						line_state = S_STRING;
						*w++ = c;
						break;
					default:
						line_state = S_ERROR;
						break;
				}
				break;
			case S_FINISH:
				
				return argc;
			case S_ERROR:
				fprintf(stderr, "Tokenize error:\n%s", line);
				walk--;
				while (walk-- > line) {
					fputc(' ', stderr);
				}
				fputc('^', stderr);
				fprintf(stderr, "\n");
				return 0;
		}
	}
	return 0;
}

int PvCppParser::convertArgs(int mode)
{
	int i, j, c;
	parameters.parent_id = NULL;
	parameters.this_id = NULL;
	parameters.cval = NULL;
	j = 0; c = 0;

	// XXX printf("-----------------------\n");

	if (argc > 2) {
		parameters.this_id = argv[2];
		// Third arg could be parent id or text
		parameters.parent_id = argv[3]; // mode 1 (constructor)
		parameters.text = argv[3]; // mode 0 (parameter)
	}

	for (i = 3 + mode; i < argc; i++) {
		switch (argt[i]) {
			case T_NUM:
				parameters.ival[j++] = atoi(argv[i]);
				break;
			case T_IDENTIFIER:
				switch (c) {
					case 0:
						parameters.cval = argv[i];
						break;
					case 1:
						// parameters.text = argv[i];
						break;
					// default:
					// 	printf("EXCEED IDENTIFIER RANGE\n");
				}
				c++;
				break;
			case T_STRING:
				switch (c) {
					case 0:
						parameters.text = argv[i];
						break;
					case 1:
						break;
					// default:
						// printf("EXCEED STRING RANGE\n");
				}
				c++;
				break;
			default:
				printf("\tUNDEFINED, type %d\n", argt[i]);
		}
		// printf("\tArg[%d]: '%s', type %d\n", i, argv[i], argt[i]);
	}
	return 0;
}


int PvCppParser::readWidget(QWidget *root)
{
	int ret;
	C &context = widget_context;

	QString pid;
	QString id;

	convertArgs(1);

	// The case for pvDownloadFile
	if (!parameters.parent_id) {
		return -1;
	}
	id = parameters.this_id;
	pid = parameters.parent_id;

	if (mangling) {
		demangle_cname(QString(parameters.parent_id), pid);
		demangle_cname(QString(parameters.this_id), id);
		printf("PID: %s\n", (const char *) pid.toUtf8());
	}

	if (parameters.parent_id[0] == '0') {
		printf("CHOOSE ROOT\n");
		context.pw = root;
	} else {
		context.pw = findChild(pid.toUtf8());
	}

	if (context.pw == NULL) {
		printf("FALLBACK: Parent not found, use root");
		context.pw = root;
	}

#ifndef TEST
	const struct lookup_s *walk = const_lookup;
	while (walk->identifier) {
		if (strcmp(argv[0], walk->identifier) == 0) {
			state = S_CONSTRUCTOR;
			context.iitem = iitem; // Ugly
			ret = walk->create_method(&context, id.toUtf8(), &item, &itemtype);
			if (ret == 0) {
				iitem++;
			} else {
				return -1;
			}
			return 1;
		}
		walk++;
	}
	// printf("unhandled: %s", line);
#endif
	return 0;
}


static int set_alignment(C *c, QWidget *item, int itemtype)
{
	static const struct align_s {
		const char *id;
		int align;
	} align_lut[] = {
		{ "AlignLeft", Qt::AlignLeft },
		{ "AlignRight", Qt::AlignRight },
		{ "AlignHCenter", Qt::AlignHCenter }, 
		{ "AlignJustify", Qt::AlignJustify },
		{ "AlignTop", Qt::AlignTop },
		{ "AlignBottom", Qt::AlignBottom },
		{ "AlignVCenter", Qt::AlignVCenter },
		{ NULL, 0 }
	};
		
	int align = 0;
	const struct align_s *walk = align_lut;
	while (*walk->id) {
		if (contains(c->params->cval, walk->id))	{ 
			align |= walk->align;
		}
		walk++;
	}

	if (align != 0) {
		switch (itemtype) {
			case TQLabel:
				((MyLabel *) item)->setAlignment((Qt::Alignment) align);
				break;
			case TQLineEdit:
				((MyLineEdit *) item)->setAlignment((Qt::Alignment) align);
				break;
			default:
				return -1;
				break;
		}
	}

	return 0;
}


int PvCppParser::set_parameter(int method, QWidget *item, int itemtype)
{
	QString st, qtext = QString(parameters.text);
	int *ival = parameters.ival;
	int ret = 0;

	switch (method) {
		case SET_GEOMETRY:
			switch (itemtype) {
			case TQImage:
				{
					QImageWidget *iw = (QImageWidget *) item;
					iw->setGeometry(ival[0],ival[1],ival[2],ival[3]);
				}
				break;
			case TQDraw:
				{
					QDrawWidget *dw = (QDrawWidget *) item;
					dw->setGeometry(ival[0],ival[1],ival[2],ival[3]);
					myResize(item,ival[2],ival[3]);
				}
				break;
			case TQVtk:
				item->setGeometry(ival[0],ival[1],ival[2],ival[3]);
				myResize(item,ival[2],ival[3]);
				break;
			default:
				item->setGeometry(ival[0],ival[1],ival[2],ival[3]);
			}
			break;
		case SET_TABPOS:
			if (itemtype == TQTabWidget) {
				((MyQTabWidget *)item)->setTabPosition(
					(QTabWidget::TabPosition) ival[0]);

			}
			break;
		case SET_EDITABLE:
			switch (itemtype) {
				case TQTable:
					((MyTable *)item)->setEnabled(false);
					break;
				case TQComboBox:
					((MyComboBox *)item)->setEditable(ival[0]);
					break;
				case TQMultiLineEdit:
					if(ival[0] == 1)
						((MyMultiLineEdit *)item)->setReadOnly(false);
					else
						((MyMultiLineEdit *)item)->setReadOnly(true);
					break;
				default:
					return -1;
			}
			break;
		case SET_PALETTEFGCOL:
			mySetForegroundColor(item, itemtype, ival[0],ival[1],ival[2]);
			st = item->statusTip();
			if(!st.contains("foreground:")) {
				st.append("foreground:");
				item->setStatusTip(st);
			}
			break;

		case SET_PALETTEBGCOL:
			mySetBackgroundColor(item, itemtype, ival[0],ival[1],ival[2]);
			st = item->statusTip();
			if(!st.contains("background:")) {
				st.append("background:");
				item->setStatusTip(st);
			}
			break;
			
		case SET_TEXT:
			switch(itemtype)
			{
				case TQLabel:
					((MyLabel *) item)->setText(qtext);
					break;
				case TQPushButton:
					((MyQPushButton *) item)->setText(qtext);
					break;
				case TQRadio:
					((MyRadioButton *) item)->setText(qtext);
					break;
				case TQCheck:
					((MyCheckBox *) item)->setText(qtext);
					break;
				case TQLineEdit:
					((MyLineEdit *) item)->setText(qtext);
					break;
				case TQGroupBox:
					((MyGroupBox *) item)->setTitle(qtext);
					break;
				default:
					printf("unknown pvSetText(%s) itemtype=%d\n",
						(const char *) qtext.toUtf8(),itemtype);
					break;
			}
			break;
		case SET_ALIGNMENT:
			ret = set_alignment(&widget_context, item, itemtype);
			if (ret < 0) {
				printf("unknown pvSetAlignment() itemtype=%d\n", itemtype);
			}
			break;
		case SET_CHECKED:
			switch (itemtype) {
				case TQRadio:
				{
					MyRadioButton *but = (MyRadioButton *) item;
					if(ival[0] == 1) but->setChecked(true);
					else           but->setChecked(false);
				}
				break;
				case TQCheck:
				{
					MyCheckBox *but = (MyCheckBox *) item;
					if(ival[0] == 1) but->setChecked(true);
					else           but->setChecked(false);
				}
			}
			st = item->statusTip();
			if (!st.contains("check:")) {
				st.append("check:");
				item->setStatusTip(st);
			}
			break;
		case SET_ECHOMODE:
			if (itemtype == TQLineEdit) {
				MyLineEdit *le = (MyLineEdit *) item;
				switch (ival[0]) {
					case 0:
						le->setEchoMode(QLineEdit::NoEcho);
						break;
					case 2:
						le->setEchoMode(QLineEdit::Password);
						break;
					default:
						le->setEchoMode(QLineEdit::Normal);
				}
			}
			st = item->statusTip();
			if (!st.contains("echomode:")) {
				st.append("echomode:");
				item->setStatusTip(st);
			}
			break;
		case SET_MULTISEL:
			switch (itemtype) {
				case TQListView:
				{
					MyListView *lv = (MyListView *) item;
					lv->setMultiSelection(ival[0]);
				}
				break;
				case TQListBox:
				{
					MyListBox *lb = (MyListBox *) item;
					switch (ival[0]) {
					case 1:
						lb->setSelectionMode(QAbstractItemView::MultiSelection);
						break;
					case 2:
						lb->setSelectionMode(QAbstractItemView::NoSelection);
						break;
					default:
						lb->setSelectionMode(QAbstractItemView::SingleSelection);
					}
				}
			}

			st = item->statusTip();
			if (!st.contains("multiselect:")) {
				st.append("multiselect:");
				item->setStatusTip(st);
			}
			break;
		case SET_FONT:
		{
			QFont font(qtext, ival[0]);

			font.setBold( (ival[1] ? true : false) );
			font.setItalic( (ival[2] ? true : false) );
			font.setUnderline( (ival[2] ? true : false) );
			font.setStrikeOut( (ival[4] ? true : false) );

			if (itemtype == TQMultiLineEdit) {
				MyMultiLineEdit *me = (MyMultiLineEdit *) item;
				me->setCurrentFont(font);
			} else {
				QWidget *w = (QWidget *) item;
				w->setFont(font);
			}
			st = item->statusTip();
			if (!st.contains(":font:")) {
				st.append("font:");
				item->setStatusTip(st);
			}

			break;
		}
		case SET_STYLE:
		{
			int shape, shadow;
			int line_width = atoi(argv[5]);
			frame_style(argv[3], argv[4], &shape, &shadow);
			switch (itemtype) {
				case TQLabel:
				{
					MyLabel *ptr = (MyLabel *) item;
					if (shape			!= -1)
						ptr->setFrameShape((QFrame::Shape) shape);
					if (shadow			!= -1)
						ptr->setFrameShadow((QFrame::Shadow) shadow);
					if (line_width != -1)
						ptr->setLineWidth(line_width);
					if (ival[3]			!= -1) // margin
						ptr->setMargin(ival[3]);
				}
				break;
				case TQFrame:
				{
					MyFrame *ptr = (MyFrame *) item;
					if(shape			!= -1)
						ptr->setFrameShape((QFrame::Shape) shape);
					if(shadow			!= -1)
						ptr->setFrameShadow((QFrame::Shadow) shadow);
					if(line_width != -1)
						ptr->setLineWidth(line_width);
				}

			}

		}
			break;
		case SET_TOOLTIP:
			qtext.replace("\n","\\n");
			item->setToolTip(qtext);
			break;
		case SET_ADDTAB:
			{
				QString tab(argv[4]);
				QString id(parameters.this_id);
				QString pid(parameters.parent_id);
				if (mangling) {
					demangle_cname(QString(parameters.parent_id), pid);
					demangle_cname(QString(parameters.this_id), id);
				}
				QWidget *w1 = (QWidget *)
					findChild(id.toUtf8());
				QWidget *w2 =
					(QWidget *) findChild(pid.toUtf8());
				if (w1 == NULL || w2 == NULL) return -1;
				if (w1->statusTip().contains("TQTabWidget")) {
					MyQTabWidget *ptr = (MyQTabWidget *) w1;
					ptr->addTab(w2, tab);
				}
				else if (w1->statusTip().contains("TQToolBox"))
				{
					MyQToolBox *ptr = (MyQToolBox *) w1;
					ptr->addItem(w2, tab);
				}
			}
			break;
		case SET_MINSIZE:
			item->setMinimumSize(ival[0],ival[1]);
			break;
		case SET_MAXSIZE:
			item->setMaximumSize(ival[0],ival[1]);
			break;
		case SET_DLFILE:
			break;
		case SET_ADDSTRETCH:
		case SET_ADDWIDGET_OR_LAYOUT:
			if (widget_context.editlayout != NULL) {
				QString txt = QString::fromUtf8(line);
				txt.remove(" "); txt.remove("\t"); txt.remove("\n");
				widget_context.editlayout->uidlg->textEditDef->append(txt);
			}
		case SET_GLBEGIN:
			break;
		case SET_GLEND:
			break;
		case SET_WHATSTHIS:
			break;
	}

	return 0;
}

int PvCppParser::readParameters(QWidget *widget, int widget_type)
{
	C &c = widget_context;

	convertArgs(0);

	const struct param_lut_s *walk = param_lookup;
	while (walk->identifier) {
		if (strcmp(argv[0], walk->identifier) == 0) {
			state = S_PARAM;
			set_parameter(walk->method, widget, widget_type);

			if (walk->method != SET_DLFILE && iitem > 0 &&
				iitem < c.whatsthislist.length()) {
				switch (widget_type) {
				 case TQDraw:
				 case TQImage:
				 case TQVbox:
				 case TQHbox:
				 case TQGrid:
				 	break;
				 default:
					widget->setWhatsThis(c.whatsthislist.at(iitem));
				}
			}

			return 1;
		}
		walk++;
	}
	printf("unhandled: %s", line);
	return 0;

}

int PvCppParser::parse(QWidget *root)
{
	int ret;
	int count = 0;
	int nargs;

	state = S_START;
	this_root = root;

	widget_context.strlist.clear();
	widget_context.enumlist.clear();
	widget_context.tooltiplist.clear();
	widget_context.whatsthislist.clear();

	// One time init: XXX
	widget_context.line = line; // To store current line
	widget_context.argv = argv;
	widget_context.params = &parameters;

	iitem = 0;

	while (fgets(line, sizeof(line)-1, file) != NULL) {
		count++;
		switch (state) {
			case S_START:
				if (contains(line, TAG_BEGIN)) {
					printf("Begin\n");
					state = S_NEUTRAL;
				}
				break;
			case S_NEUTRAL:
				if (contains(line,"USE_CNAME_MANGLING")) {
					mangling = true;
				} else
				if (contains(line, "enum")) {
					printf("ENUM\n");
					state = S_ENUM;
				} else
				if (contains(line, "toolTip[]")) {
					printf("TOOLTIP\n");
					state = S_TOOLTIP;
				} else
				if (contains(line, "whatsThis[]")) {
					printf("WHATSTHIS\n");
					state = S_HELP;
				} else
				if (contains(line, "pvStartDefinition")) {
					printf("DEFINITION\n");
					state = S_DEFINITION;
				}
				break;
			case S_ENUM:
				// Note that behaviour of ENUM is different than
				// the ones below:
				if (contains(line, "}")) {
					state = S_NEUTRAL;
				} else {
					appendEnum(); // Currently 'line'-destructive FIXME
				}
				break;
			case S_TOOLTIP:
				// printf("Line:\n<%s>", line);
				if (contains(line, "}")) {
					printf("END TOOLTIP\n");
					state = S_NEUTRAL;
				}
				appendToolTip();
				break;
			case S_HELP:
				if (contains(line, "}")) {
					state = S_NEUTRAL;
				}
				appendWhatsThis();
				break;
			case S_PARAM:
			case S_CONSTRUCTOR:
			case S_DEFINITION:

				nargs = parseLine();

				if (nargs > 0) {
					if (strcmp(argv[0], "pvEndDefinition") == 0) {
						printf("END DEF\n");
						state = S_END_DEF;
					} else {
						// printf("Widget DEF line %d\n", count);
						if (state == S_DEFINITION) {
							ret = readWidget(root);
							if (ret < 0) {
								fprintf(stderr,
									"Item '%s' not created, line %d\n",
									argv[0],
									count);
							} else
							if (ret == 0) {
								printf("Unexpected identifier in "
								"constructor state: '%s'"
								"\nTry parameter mode...\n", argv[0]);
								state = S_PARAM;
								// We reuse the previous item type.
								// This is very ugly, but we assume
								// that this is only within the layout
								// construction context.
								// We can NOT use findChild() on a
								// layout entity.
								readParameters(item, itemtype);
							}
						} else {
							readParameters(item, itemtype);
						}
					}
				} else {
					state = S_DEFINITION;
				}
				break;
			case S_END_DEF:
				break;
			case S_END:
				break;

		}
	}
	return 0;
}

#ifdef TEST

int main()
{
	FILE *f = fopen("../modhub/mask1.cpp", "r");

	if (f) {
		PvCppParser p(f, 0);
		p.parse(NULL);
		fclose(f);
	} else {
		fprintf(stderr, "File not found\n");
	}

	return 0;
}

#endif
