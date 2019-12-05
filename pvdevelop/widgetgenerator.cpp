/****************************************************************************
**
** Copyright (C) 2000-2006 Lehrig Software Engineering.
**
** This file is part of the pvbrowser project.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include "../pvbrowser/pvdefine.h"
#include <string.h>
#include <ctype.h>
#include "opt.h"
#include <QWidget>
#include <QStringList>
#include <QRegExp>
#include <QFile>
#include <QtUiTools>
#include <QFontInfo>
#include "dlginsertwidget.h"
#include "dlgeditlayout.h"
#include "widgetgenerator.h"
#include "cutil.h"
#include "pname_mangler.h"
#include "interpreter.h"
#include "pvserver.h"
#include "designer.h"

#include "parser.h"

extern OPT_DEVELOP opt_develop;
extern dlgeditlayout *editlayout;

QStringList tablist;
static QStringList strlist;
static char line[1024];
static int iitem = 1;

#define MAX_IVAL 20

QWidget *theroot;

QWidget *findChild(const char *item) // Yes I know: It's a dirty hack																										 // because of MSDEV 6.0 :-)
{
	//printf("findChild(%s)\n",item);
	QWidget *ret;
#ifdef PVWIN32
#if QT_VERSION < 0x050000		 
	QString txt = item;
	ret = qFindChild<QWidget *>(theroot, txt);
#else
	ret = theroot->findChild<QWidget *>(item);
#endif
#else
	ret = theroot->findChild<QWidget *>(item);
#endif
	if(ret == NULL)
		printf("findChild(%s) ret == NULL\n",item);
	return ret;
}

static int createStrList(QObject *item)
{
	QObject *subitem;
	QList<QObject *> levellist;
	levellist.clear();
	levellist = item->children();
	// printf("String list item: '%s'\n", (const char *) item->objectName().toLatin1());
	for(int i=0; i<levellist.size(); i++)
	{
		subitem = levellist.at(i);
		if(subitem->isWidgetType() &&
			((QWidget *) subitem)->statusTip().startsWith("TQTabWidget:"))
		{
			strlist.append(subitem->objectName());
			QList<QObject *> childlist;
			QList<QObject *> rlist;
			childlist.clear();
			childlist = subitem->children();
			for(int ic=0; ic<childlist.size(); ic++)
			{
				rlist.clear();
				rlist = childlist.at(ic)->children();
				for(int ir=0; ir<rlist.size(); ir++)
				{
					int isub = rlist.size()-ir-1;
					if(!rlist.at(isub)->objectName().isEmpty())
					{
						//printf("objectName=%s\n",
						//			(const char *) rlist.at(isub)->objectName().toUtf8());
						strlist.append(rlist.at(isub)->objectName());
						createStrList(rlist.at(isub));
					}
				}
			}
		}
		else
		{
			if(subitem->isWidgetType() &&
				((QWidget *) subitem)->statusTip().startsWith("TQ"))
			{
				strlist.append(subitem->objectName());
			}
			createStrList(subitem);
		}
	}
/*
	QObject *subitem;
	QList<QObject *> levellist;
	levellist.clear();
	levellist = item->children();
	for(int i=0; i<levellist.size(); i++)
	{
		subitem = levellist.at(i);
		if(subitem->isWidgetType() &&
			((QWidget *) subitem)->statusTip().startsWith("TQ"))
		{
			strlist.append(subitem->objectName());
		}
		createStrList(subitem);
	}
*/
	return 0;
}

static int sortWidgets(QObject *item)
{
	QObject *subitem;
	QList<QObject *> levellist;
	levellist.clear();
	levellist = item->children();
	for(int i=0; i<levellist.size(); i++)
	{
		subitem = levellist.at(i);
		if(subitem->isWidgetType() &&
			((QWidget *) subitem)->statusTip().startsWith("TQTabWidget")) //xlehrig
		{
			QList<QObject *> sublist;
			sublist.clear();
			sublist = subitem->children();
			QList<QObject *> subsublist;
			subsublist.clear();
			subsublist = sublist.at(0)->children();
			for(int index=subsublist.size()-1; index>=0; index--)
			{
				//printf("index=%d\n",index);
				((QTabWidget *) subitem)->setCurrentIndex(index);
			}
		}
		sortWidgets(subitem);
	}
	return 0;
}

static int getStrList(QWidget *root)
{
	sortWidgets(root);
	strlist.clear();
	createStrList(root);
	return 0;
}

static int generateLayoutConstuctors(FILE *fout)
{
	QTextEdit *e = editlayout->uidlg->textEditConstructors;
	if(e == NULL || fout == NULL) return -1;

	QString txt;
	QTextBlock line = e->document()->begin();
	while(line.isValid())
	{
		txt = line.text();
		txt.remove(" ");
		txt.remove("\n");
		if(txt.contains("pvQLayout") && txt.contains("(") && txt.contains(")"))
		{
			fprintf(fout,"	%s\n",(const char *) txt.toUtf8().data());
			fprintf(fout,"\n");
		}
		line = line.next();
	}
	return 0;
}

static int generateLayoutDefinition(FILE *fout)
{
	QTextEdit *e = editlayout->uidlg->textEditDef;
	if(e == NULL || fout == NULL) return -1;

	QString txt;
	QTextBlock line = e->document()->begin();
	while(line.isValid())
	{
		txt = line.text();
		txt.remove(" ");
		txt.remove("\n");
		if(txt.contains("pvAdd") && txt.contains("(") && txt.contains(")"))
		{
			fprintf(fout,"	%s\n",(const char *) txt.toUtf8().data());
		}
		line = line.next();
	}
	fprintf(fout,"\n");
	return 0;
}

#define isEnum 1
#define isName 2
static int generateWidgetEnum(FILE *fout, QWidget *root, int type=isEnum)
{
	QString item;
	QWidget *widget;
	int ind;

	theroot = root;
	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	ID_MAIN_WIDGET = 0\n");
	}
	else
	{
		if(type == isName)
		{
			fprintf(fout,"// our mask contains the following widget names\n");
			fprintf(fout,"	static const char *widgetName[] = {\n");
			fprintf(fout,"	\"ID_MAIN_WIDGET\",\n");
		}
		else
		{
			fprintf(fout,"// our mask contains the following objects\n");
			fprintf(fout,"enum {\n");
			fprintf(fout,"	ID_MAIN_WIDGET = 0,\n");
		}  
	}

	// loop over widgets
	ind = 1;
	for(int i=0; i<strlist.size(); i++)
	{
		item = strlist.at(i);
		widget = findChild(item.toUtf8()); // root->findChild<QWidget *>(item);
		if(widget != NULL)
		{
			if(opt_develop.script == PV_LUA)
			{
				fprintf(fout,"	%s = %d\n",(const char *) widget->objectName().toUtf8(), ind);
			}
			else
			{
				QString name = QString(widget->objectName());
				if(type == isName)
				{
					fprintf(fout,"	\"%s\",\n",(const char *) name.toUtf8());
				}
				else
				{
					QString cname;
					if (mangle_cname(widget->objectName(), cname) == 0) 
						fprintf(fout,"	%s,\n",(const char *) cname.toLatin1());
					else
						fprintf(fout,"	%s,\n",(const char *) widget->objectName().toUtf8());
				}
			}
		}
		else
		{
			printf("WARNING generateWidgetEnum:findChild=%s not found\n",(const char *) item.toUtf8());
		}
		ind++;
	}

	// may be we have to include the layout BEGIN
	QTextEdit *e = editlayout->uidlg->textEditConstructors;
	if(e != NULL)
	{
		QString txt;
		QTextBlock line = e->document()->begin();
		while(line.isValid())
		{
			txt = line.text();
			txt.remove(" ");
			txt.remove("\n");
			if(txt.contains("pvQLayout") && txt.contains("(") && txt.contains(")"))
			{
				char buf[1024];
				if		 (txt.contains("(p,0,")) ;							// we have nothing todo
				else if(txt.contains("(p,ID_MAIN_WIDGET,")) ; // we have nothing todo
				else if(txt.length() < (int) (sizeof(buf)-1))
				{
					char *start, *end;
					strcpy(buf,txt.toUtf8().data());
					start = strchr(buf,',');
					if(start != NULL)
					{
						start++;
						end = strchr(start,',');
						if(end != NULL) *end = '\0';
						if(opt_develop.script == PV_LUA)
						{
							fprintf(fout,"	%s = %d\n",start,ind++);
						}
						else
						{
							if(type == isName)
							{
								fprintf(fout,"	\"%s\",\n",start);
							}
							else
							{
								fprintf(fout,"	%s,\n",start);
							}  
						}
					}
				}
				else printf("too long layout string: %s\n",(const char *) txt.toUtf8().data());
			}
			line = line.next();
		}
	}
	// may be we have to include the layout END

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	ID_END_OF_WIDGETS = %d\n", ind);
		fprintf(fout,"\n");
	}
	else
	{
		if(type == isName)
		{
			fprintf(fout,"	\"ID_END_OF_WIDGETS\",\n");
			fprintf(fout,"	\"\"};\n");
			fprintf(fout,"\n");
		}
		else
		{
			fprintf(fout,"	ID_END_OF_WIDGETS\n");
			fprintf(fout,"};\n");
			fprintf(fout,"\n");
		}
	}
	return 0;
}

static const char *quote0(QString &text)
{
	const char *cptr;
	static char buf[1024];
	int i;

	int  datasize = strlen(text.toUtf8());
	char data[datasize+1];
	strncpy(data,text.toUtf8().data(),datasize);
	data[datasize] = '\0';
	cptr = data;

	i = 0; // convert to utf8 and quote '"' and '\n'
	//cptr = text.toUtf8().data();
	while(*cptr != '\0' && i < (int) (sizeof(buf)-2))
	{
		if		 (*cptr == '\"')
		{
			buf[i++] = '\\';
			buf[i++] = *cptr;
		}  
		else if(*cptr == '\n') 
		{
			buf[i++] = '\\';
			buf[i++] = 'n';
		}
		else
		{
			buf[i++] = *cptr;
		}
		cptr++;
	}
	buf[i] = '\0';
	return buf;
}

static const char *quote(QString &text)
{
	const char *cptr;
	static char buf[1024],buf2[1048];
	int i;

	int  datasize = strlen(text.toUtf8());
	char data[datasize+1];
	strncpy(data,text.toUtf8().data(),datasize);
	data[datasize] = '\0';
	cptr = data;

	i = 0; // convert to utf8 and quote '"' and '\n'
	//cptr = text.toUtf8().data();
	while(*cptr != '\0' && i < (int) (sizeof(buf)-2))
	{
		if		 (*cptr == '\"')
		{
			buf[i++] = '\\';
			buf[i++] = *cptr;
		}  
		else if(*cptr == '\n') 
		{
			buf[i++] = '\\';
			buf[i++] = 'n';
		}
		else
		{
			buf[i++] = *cptr;
		}
		cptr++;
	}
	buf[i] = '\0';
	if(opt_develop.script == 0)
	{
		// is C/C++
		strcpy(buf2,"pvtr(\""); strcat(buf2,buf); strcat(buf2,"\")");
	}
	else
	{
		// is a scripting language
		strcpy(buf2,"\""); strcat(buf2,buf); strcat(buf2,"\"");
	}  
	return buf2;
}

static int generateDefineMaskWidget(FILE *fout, QWidget *widget, const char *tabparentname)
{
	QString qbuf,type,tooltip,whatsthis,statustip,text;
	char buf[1024], *cptr,prefix[16],midfix[16],postfix[16];
	int x,y,w,h;

	if(opt_develop.script == PV_LUA)
	{
		strcpy(prefix,"  pv.");
		strcpy(midfix,"pv.");
		strcpy(postfix,"");
	}
	else
	{
		strcpy(prefix,"  ");
		strcpy(midfix,"");
		strcpy(postfix,";");
	}

	if(fout == NULL || widget == NULL)
	{
		printf("WARNING: generateDefineMaskWidget pointer == NULL\n");
		return -1;
	}
	// get values begin ######################################################
	qbuf = widget->statusTip(); // parse statusTip
	strcpy(buf,qbuf.toUtf8());
	cptr = strstr(buf,":");
	if(cptr != NULL) *cptr = '\0';
	type = buf;

	tooltip		= widget->toolTip();
	whatsthis = widget->whatsThis();
	statustip = widget->statusTip();
	// get values end ########################################################

	// get properties begin ##################################################
	x = widget->x();
	y = widget->y();
	w = widget->width();
	h = widget->height();

	QString iname, pname, iorig, porig;
	QObject *parent_widget = widget->parent();

	iorig.append(widget->objectName());
	porig.append(parent_widget->objectName());

	if (mangle_cname(iorig, iname) != 0) 
		iname = iorig;

	if (mangle_cname(porig, pname) != 0) 
		pname = porig;


#define _C(x) (const char *) x.toLatin1()

	// get properties end ####################################################

	if(opt_develop.arg_debug) printf("generateDefineMaskWidget(%s,%s)\n",(const char *) type.toUtf8(), (const char *) iname.toUtf8());

	if		 (type == "TQWidget")
	{
		QWidget *grandpa = NULL;
		QWidget *grandpa_parent = NULL;
		QWidget *pa = (QWidget *) widget->parent();
		if(pa != NULL) grandpa = (QWidget *) widget->parent()->parent();
		if(grandpa != NULL) grandpa_parent = (QWidget *) widget->parent()->parent()->parent();
		if(grandpa != NULL && grandpa->statusTip().startsWith("TQTabWidget:"))
		{
			QTabWidget *tab = (QTabWidget *) grandpa;
			QString txt = tab->tabText(tab->indexOf(widget));
			QString name, tabname;
			if (mangle_cname(tab->objectName(), name) != 0)
				name = QString(tab->objectName());
			if (mangle_cname(tab->objectName(), tabname) != 0)
				tabname = QString(tab->objectName());

			fprintf(fout,"%spvQWidget(p,%s,%s)%s\n",prefix,_C(iname),
				(const char *) name.toLatin1(),postfix);
			fprintf(fout,"%spvAddTab(p,%s,%s,%s)%s\n",prefix,
				(const char *) tabname.toLatin1() ,_C(iname),quote(txt),postfix);
		}
		else if(grandpa_parent != NULL && grandpa_parent->statusTip().startsWith("TQToolBox:"))
		{
			QToolBox *tool = (QToolBox *) grandpa_parent;
			QString txt = tool->itemText(tool->indexOf(widget));
			fprintf(fout,"%spvQWidget(p,%s,%s)%s\n",prefix,_C(iname),(const char *) tool->objectName().toUtf8(),postfix);
			fprintf(fout,"%spvAddTab(p,%s,%s,%s)%s\n",prefix,(const char *) tool->objectName().toUtf8() ,_C(iname),quote(txt),postfix);
		}
		else
		{
			printf("unknown ancestor of TQWidget\n");
			fprintf(fout,"%spvQWidget(p,%s,%s)%s\n",prefix,_C(iname),tabparentname,postfix);
			if(pa != NULL) printf("pa->statusTip=%s\n",(const char *) pa->statusTip().toUtf8());
			if(grandpa != NULL) printf("grandpa->statusTip=%s\n",(const char *) grandpa->statusTip().toUtf8());
			if(grandpa->parent() != NULL) printf("grandpa->parent()->statusTip=%s\n",(const char *) ((QWidget *) grandpa->parent())->statusTip().toUtf8());
		}
		iitem++;
	}
	else if(type == "TQPushButton")
	{
		fprintf(fout,"%spvQPushButton(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		MyQPushButton *obj = (MyQPushButton *) widget;
		text = obj->text();
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(!text.isEmpty()) fprintf(fout,"%spvSetText(p,%s,%s)%s\n",prefix,_C(iname), quote(text),postfix);
		iitem++;
	}
	else if(type == "TQLabel")
	{
		MyLabel *obj = (MyLabel *) widget;
		text = obj->text();
		fprintf(fout,"%spvQLabel(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(!text.isEmpty()) fprintf(fout,"%spvSetText(p,%s,%s)%s\n",prefix,_C(iname), quote(text),postfix);
		// alignment
		QString align;
		int flags = 0;
		int alignment = (int) obj->alignment();
		if(alignment & Qt::AlignLeft)		 
		{ 
			flags++; align += "AlignLeft";		
		}
		if(alignment & Qt::AlignRight)	 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignRight";		
		}
		if(alignment & Qt::AlignHCenter) 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignHCenter"; 
		}
		if(alignment & Qt::AlignJustify) 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignJustify"; 
		}
		if(alignment & Qt::AlignTop)		 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignTop";			
		}
		if(alignment & Qt::AlignBottom)  
		{ 
			if(flags) align += "|";
			flags++; align += "AlignBottom";	
		}
		if(alignment & Qt::AlignVCenter) 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignVCenter"; 
		}
		if(align.contains("Align"))
		{
			if(align.contains("AlignLeft|AlignVCenter")) flags = 0; 
			if(flags) fprintf(fout,"%spvSetAlignment(p,%s,%s%s)%s\n",prefix,_C(iname),midfix,(const char *) align.toUtf8(),postfix);
		}

		iitem++;
	}
	else if(type == "TQLineEdit")
	{
		MyLineEdit *obj = (MyLineEdit *) widget;
		text = obj->text();
		fprintf(fout,"%spvQLineEdit(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(!text.isEmpty()) fprintf(fout,"%spvSetText(p,%s,%s)%s\n",prefix,_C(iname), quote(text),postfix);
		if(obj->isReadOnly())
		{
			fprintf(fout,"%spvSetEditable(p,%s,0)%s\n",prefix,_C(iname),postfix);
		}
		if(obj->echoMode() == QLineEdit::Password)
		{
			fprintf(fout,"%spvSetEchoMode(p,%s,2)%s\n",prefix,_C(iname),postfix);
		}
		else if(obj->echoMode() == QLineEdit::NoEcho)
		{
			fprintf(fout,"%spvSetEchoMode(p,%s,0)%s\n",prefix,_C(iname),postfix);
		}
		// alignment
		QString align;
		int flags = 0;
		int alignment = (int) obj->alignment();
		if(alignment & Qt::AlignLeft)		 
		{ 
			flags++; align += "AlignLeft";		
		}
		if(alignment & Qt::AlignRight)	 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignRight";		
		}
		if(alignment & Qt::AlignHCenter) 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignHCenter"; 
		}
		if(alignment & Qt::AlignJustify) 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignJustify"; 
		}
		if(alignment & Qt::AlignTop)		 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignTop";			
		}
		if(alignment & Qt::AlignBottom)  
		{ 
			if(flags) align += "|";
			flags++; align += "AlignBottom";	
		}
		if(alignment & Qt::AlignVCenter) 
		{ 
			if(flags) align += "|";
			flags++; align += "AlignVCenter"; 
		}
		if(align.contains("Align"))
		{
			if(align.contains("AlignLeft|AlignVCenter")) flags = 0; 
			if(flags) fprintf(fout,"%spvSetAlignment(p,%s,%s%s)%s\n",prefix,_C(iname),midfix,(const char *) align.toUtf8(),postfix);
		}
		
		iitem++;
	}
	else if(type == "TQMultiLineEdit")
	{
		QString tooltip;
		int editable, maxlines;
		MyMultiLineEdit *obj = (MyMultiLineEdit *) widget;
		text = obj->toPlainText();
		if(obj->isReadOnly()) editable = 0;
		else									editable = 1;
		tooltip = obj->toolTip();
		maxlines = obj->maxlines;
		fprintf(fout,"%spvQMultiLineEdit(p,%s,%s,%d,%d)%s\n",prefix,_C(iname),_C(pname),editable,maxlines,postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQTextBrowser")
	{
		fprintf(fout,"%spvQTextBrowser(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQListView")
	{
		MyListView *obj = (MyListView *) widget;
		fprintf(fout,"%spvQListView(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if		 (obj->selectionMode() == QAbstractItemView::MultiSelection)
		{
			fprintf(fout,"%spvSetMultiSelection(p,%s,1)%s\n",prefix,_C(iname),postfix);
		}
		else if(obj->selectionMode() == QAbstractItemView::NoSelection)
		{
			fprintf(fout,"%spvSetMultiSelection(p,%s,2)%s\n",prefix,_C(iname),postfix);
		}
		else
		{
			fprintf(fout,"%spvSetMultiSelection(p,%s,0)%s\n",prefix,_C(iname),postfix);
		}
		iitem++;
	}
	else if(type == "TQComboBox")
	{
		MyComboBox *obj = (MyComboBox *) widget;
		QString insertionpolicy = "AtBottom";
		int editable = 0;
		if(obj->isEditable()) editable = 1;
		if((int) obj->insertPolicy() == 0) insertionpolicy = "NoInsertion";
		if((int) obj->insertPolicy() == 1) insertionpolicy = "AtTop";
		if((int) obj->insertPolicy() == 2) insertionpolicy = "AtCurrent";
		if((int) obj->insertPolicy() == 3) insertionpolicy = "AtBottom";
		if((int) obj->insertPolicy() == 4) insertionpolicy = "AfterCurrent";
		if((int) obj->insertPolicy() == 5) insertionpolicy = "BeforeCurrent";
		fprintf(fout,"%spvQComboBox(p,%s,%s,%d,%s%s)%s\n",prefix,_C(iname),_C(pname),editable,midfix,(const char *) insertionpolicy.toUtf8(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQLCDNumber")
	{
		QLCDNumber *obj = (QLCDNumber *) widget;
		int numdigits;
#if QT_VERSION < 0x050000		 
		numdigits = obj->numDigits();
#else
		numdigits = obj->digitCount();
#endif
		QString style = "Outline";
		if(obj->segmentStyle() == QLCDNumber::Filled) style = "Filled";
		if(obj->segmentStyle() == QLCDNumber::Flat)		style = "Flat";
		QString mode = "Hex";
		if(obj->mode() == QLCDNumber::Dec) mode = "Dec";
		if(obj->mode() == QLCDNumber::Oct) mode = "Oct";
		if(obj->mode() == QLCDNumber::Bin) mode = "Bin";
		fprintf(fout,"%spvQLCDNumber(p,%s,%s,%d,%s%s,%s%s)%s\n",prefix,_C(iname),_C(pname),numdigits,midfix, (const char *) style.toUtf8(),midfix, (const char *) mode.toUtf8(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQSlider")
	{
		MySlider *obj = (MySlider *) widget;
		int pagestep = obj->pageStep();
		int value		 = obj->value();
		int minvalue = obj->minimum();
		int maxvalue = obj->maximum();
		QString orientation = "Vertical";
		if(obj->orientation() == Qt::Horizontal ) orientation = "Horizontal";
		fprintf(fout,"%spvQSlider(p,%s,%s,%d,%d,%d,%d,%s%s)%s\n",prefix,_C(iname),_C(pname),minvalue,maxvalue,pagestep,value,midfix,(const char *) orientation.toUtf8(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQButtonGroup")
	{
		MyButtonGroup *obj = (MyButtonGroup *) widget;
		text = obj->title();
		fprintf(fout,"%spvQButtonGroup(p,%s,%s,-1,%sHORIZONTAL,%s)%s\n",prefix,_C(iname),_C(pname),midfix,quote(text),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQGroupBox")
	{
		MyGroupBox *obj = (MyGroupBox *) widget;
		text = obj->title();
		fprintf(fout,"%spvQGroupBox(p,%s,%s,-1,%sHORIZONTAL,%s)%s\n",prefix,_C(iname),_C(pname),midfix,quote(text),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQListBox")
	{
		MyListBox *obj = (MyListBox *) widget;
		fprintf(fout,"%spvQListBox(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if		 (obj->selectionMode() == QAbstractItemView::MultiSelection)
		{
			fprintf(fout,"%spvSetMultiSelection(p,%s,1)%s\n",prefix,_C(iname),postfix);
		}
		else if(obj->selectionMode() == QAbstractItemView::NoSelection)
		{
			fprintf(fout,"%spvSetMultiSelection(p,%s,2)%s\n",prefix,_C(iname),postfix);
		}
		else
		{
			fprintf(fout,"%spvSetMultiSelection(p,%s,0)%s\n",prefix,_C(iname),postfix);
		}
		iitem++;
	}
	else if(type == "TQIconView")
	{
		fprintf(fout,"%spvQIconView(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQTable")
	{
		MyTable *obj = (MyTable *) widget;
		fprintf(fout,"%spvQTable(p,%s,%s,%d,%d)%s\n",prefix,_C(iname),_C(pname),obj->rowCount(),obj->columnCount(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(!obj->isEnabled())
		{
			fprintf(fout,"%spvSetEditable(p,%s,0)%s\n",prefix,_C(iname),postfix);
		}
		iitem++;
	}
	else if(type == "TQSpinBox")
	{
		MySpinBox *obj = (MySpinBox *) widget;
		fprintf(fout,"%spvQSpinBox(p,%s,%s,%d,%d,1)%s\n",prefix,_C(iname),_C(pname),obj->minimum(),obj->maximum(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQDial")
	{
		MyDial *obj = (MyDial *) widget;
		fprintf(fout,"%spvQDial(p,%s,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),_C(pname),obj->minimum(),obj->maximum(),obj->pageStep(),obj->value(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQProgressBar")
	{
		MyProgressBar *obj = (MyProgressBar *) widget;
		QString orientation = "Vertical";
		if(obj->orientation() == Qt::Horizontal ) orientation = "Horizontal";
		fprintf(fout,"%spvQProgressBar(p,%s,%s,%d,%s%s)%s\n",prefix,_C(iname),_C(pname),obj->maximum(),midfix,(const char *) orientation.toUtf8(),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQRadio")
	{
		MyRadioButton *obj = (MyRadioButton *) widget;
		text = obj->text();
		fprintf(fout,"%spvQRadioButton(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(!text.isEmpty()) fprintf(fout,"%spvSetText(p,%s,%s)%s\n",prefix,_C(iname), quote(text),postfix);
		if(obj->isChecked())
		{
			fprintf(fout,"%spvSetChecked(p,%s,1)%s\n",prefix,_C(iname),postfix);
		}
		iitem++;
	}
	else if(type == "TQCheck")
	{
		MyCheckBox *obj = (MyCheckBox *) widget;
		text = obj->text();
		fprintf(fout,"%spvQCheckBox(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(!text.isEmpty()) fprintf(fout,"%spvSetText(p,%s,%s)%s\n",prefix,_C(iname), quote(text),postfix);
		if(obj->isChecked())
		{
			fprintf(fout,"%spvSetChecked(p,%s,1)%s\n",prefix,_C(iname),postfix);
		}
		iitem++;
	}
	else if(type == "TQFrame")
	{
		MyFrame *obj = (MyFrame *) widget;
		QString shape = "NoFrame";
		if(obj->frameShape() == QFrame::StyledPanel) shape = "StyledPanel";
		if(obj->frameShape() == QFrame::VLine)			 shape = "VLine";
		if(obj->frameShape() == QFrame::HLine)			 shape = "HLine";
		if(obj->frameShape() == QFrame::WinPanel)		 shape = "WinPanel";
		if(obj->frameShape() == QFrame::Panel)			 shape = "Panel";
		if(obj->frameShape() == QFrame::Box)				 shape = "Box";
		QString shadow = "Plain";
		if(obj->frameShadow() == QFrame::Raised)		 shadow = "Raised";
		if(obj->frameShadow() == QFrame::Sunken)		 shadow = "Sunken";
		int linewidth = obj->lineWidth();
		int margin = 1; //obj->margin();
		fprintf(fout,"%spvQFrame(p,%s,%s,%s%s,%s%s,%d,%d)%s\n",prefix,_C(iname),_C(pname),midfix,(const char *) shape.toUtf8(),midfix, (const char *) shadow.toUtf8(), linewidth, margin,postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQDraw")
	{
		fprintf(fout,"%spvQDraw(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQImage")
	{
		if(whatsthis.contains(".bmp") || whatsthis.contains(".BMP"))
		{
			if(opt_develop.script == PV_LUA)
			{
				fprintf(fout,"%spvQImageScript(p,%s,%s,\"%s\")%s\n",prefix,_C(iname),_C(pname),(const char *) whatsthis.toUtf8(),postfix);
			}
			else
			{
				fprintf(fout,"%spvQImage(p,%s,%s,\"%s\",&w,&h,&depth)%s\n",prefix,_C(iname),_C(pname),(const char *) whatsthis.toUtf8(),postfix);
			}
			fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		}
		else
		{
			fprintf(fout,"%spvDownloadFile(p,\"%s\")%s\n",prefix,(const char *) whatsthis.toUtf8(),postfix);
			if(opt_develop.script == PV_LUA)
			{
				fprintf(fout,"%spvQImageScript(p,%s,%s,\"%s\")%s\n",prefix,_C(iname),_C(pname),(const char *) whatsthis.toUtf8(),postfix);
			}
			else
			{
				fprintf(fout,"%spvQImage(p,%s,%s,\"%s\",&w,&h,&depth)%s\n",prefix,_C(iname),_C(pname),(const char *) whatsthis.toUtf8(),postfix);
			}
			fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		}
		iitem++;
	}
	else if(type == "TQGl")
	{
		fprintf(fout,"%spvQGL(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		fprintf(fout,"%spvGlBegin(p,%s)%s\n",prefix,_C(iname),postfix);
		fprintf(fout,"	initializeGL(p);		// (todo: write your gl initialization routine) see example\n");
		fprintf(fout,"	resizeGL(p,%d,%d);	// (todo: write your resize routine) see example\n",w,h);
		fprintf(fout,"%spvGlEnd(p)%s\n",postfix,postfix);
		iitem++;
	}
	else if(type == "TQVtk")
	{
		fprintf(fout,"%spvQVtkTclWidget(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQTabWidget")
	{
		MyQTabWidget *obj = (MyQTabWidget *) widget;
		fprintf(fout,"%spvQTabWidget(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		int tabposition = 0;
		if(obj->tabPosition() == QTabWidget::South) tabposition = 1;
		if(obj->tabPosition() == QTabWidget::West)	tabposition = 2;
		if(obj->tabPosition() == QTabWidget::East)	tabposition = 3;
		if(tabposition != 0)
		{
			fprintf(fout,"%spvSetTabPosition(p,%s,%d)%s\n",prefix,_C(iname),tabposition,postfix);
		}
		iitem++;
	}
	else if(type == "TQToolBox")
	{
		fprintf(fout,"%spvQToolBox(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQwtPlotWidget")
	{
		QwtPlotWidget *obj = (QwtPlotWidget *) widget;
		fprintf(fout,"%spvQwtPlotWidget(p,%s,%s,%d,%d)%s\n",prefix,_C(iname),_C(pname),obj->nCurves,obj->nMarker,postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		QColor c = obj->canvasBackground();
		fprintf(fout,"%sqpwSetCanvasBackground(p,%s,%d,%d,%d)%s\n",prefix,_C(iname),c.red(),c.green(),c.blue(),postfix);
		if(obj->axisEnabled(QwtPlot::yLeft))	 fprintf(fout,"%sqpwEnableAxis(p,%s,%syLeft)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->axisEnabled(QwtPlot::yRight))  fprintf(fout,"%sqpwEnableAxis(p,%s,%syRight)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->axisEnabled(QwtPlot::xBottom)) fprintf(fout,"%sqpwEnableAxis(p,%s,%sxBottom)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->axisEnabled(QwtPlot::xTop))		 fprintf(fout,"%sqpwEnableAxis(p,%s,%sxTop)%s\n",prefix,_C(iname),midfix,postfix);
		text = obj->title().text();
		if(!text.isEmpty()) fprintf(fout,"%sqpwSetTitle(p,%s,%s)%s\n",prefix,_C(iname),quote(text),postfix);
		iitem++;
	}
	else if(type == "TQwtScale")
	{
		MyQwtScale *obj = (MyQwtScale *) widget;
		fprintf(fout,"%spvQwtScale(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		if(obj->scaleDraw()->alignment() == QwtScaleDraw::LeftScale)	 fprintf(fout,"%sqwtScaleSetPosition(p,%s,ScaleLeft)%s\n",prefix,_C(iname),postfix);
		if(obj->scaleDraw()->alignment() == QwtScaleDraw::RightScale)		fprintf(fout,"%sqwtScaleSetPosition(p,%s,ScaleRight)%s\n",prefix,_C(iname),postfix);
		if(obj->scaleDraw()->alignment() == QwtScaleDraw::TopScale)		fprintf(fout,"%sqwtScaleSetPosition(p,%s,ScaleTop)%s\n",prefix,_C(iname),postfix);
		if(obj->scaleDraw()->alignment() == QwtScaleDraw::BottomScale)	 fprintf(fout,"%sqwtScaleSetPosition(p,%s,ScaleBottom)%s\n",prefix,_C(iname),postfix);
		text = obj->title().text();
		if(!text.isEmpty()) fprintf(fout,"%sqwtScaleSetTitle(p,%s,%s)%s\n",prefix,_C(iname),quote(text),postfix);
		text = obj->title().font().family();
		int size			= obj->title().font().pointSize();
		int bold, italic, underline, strikeout;
		bold = italic = underline = strikeout = 0;
		if(obj->title().font().bold())			bold = 1;
		if(obj->title().font().italic())		italic = 1;
		if(obj->title().font().underline()) underline = 1;
		if(obj->title().font().strikeOut()) strikeout = 1;
		if(obj->statusTip().contains(":font:")) fprintf(fout,"%sqwtScaleSetTitleFont(p,%s,\"%s\",%d,%d,%d,%d,%d)%s\n",prefix,_C(iname),quote0(text),size,bold,italic,underline,strikeout,postfix);
		QColor c = obj->title().color();
		fprintf(fout,"%sqwtScaleSetTitleColor(p,%s,%d,%d,%d)%s\n",prefix,_C(iname),c.red(),c.green(),c.blue(),postfix);
		iitem++;
	}
	else if(type == "TQwtThermo")
	{
		fprintf(fout,"%spvQwtThermo(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQwtKnob")
	{
		MyQwtKnob *obj = (MyQwtKnob *) widget;
		fprintf(fout,"%spvQwtKnob(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		fprintf(fout,"%sqwtKnobSetKnobWidth(p,%s,%d)%s\n",prefix,_C(iname),obj->knobWidth(),postfix);
		fprintf(fout,"%sqwtKnobSetBorderWidth(p,%s,%d)%s\n",prefix,_C(iname),obj->borderWidth(),postfix);
		fprintf(fout,"%sqwtKnobSetTotalAngle(p,%s,%f)%s\n",prefix,_C(iname),obj->totalAngle(),postfix);
		if(obj->symbol() == QwtKnob::Line)
			fprintf(fout,"%sqwtKnobSetSymbol(p,%s,%sKnobLine)%s\n",prefix,_C(iname),midfix,postfix);
		else 
			fprintf(fout,"%sqwtKnobSetSymbol(p,%s,%sKnobDot)%s\n",prefix,_C(iname),midfix,postfix);
		iitem++;
	}
	else if(type == "TQwtCounter")
	{
		MyQwtCounter *obj = (MyQwtCounter *) widget;
		fprintf(fout,"%spvQwtCounter(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		fprintf(fout,"%sqwtCounterSetNumButtons(p,%s,%d)%s\n",prefix,_C(iname),obj->numButtons(),postfix);
		fprintf(fout,"%sqwtCounterSetStep(p,%s,%f)%s\n",prefix,_C(iname),obj->step(),postfix);
		fprintf(fout,"%sqwtCounterSetMinValue(p,%s,%f)%s\n",prefix,_C(iname),obj->minVal(),postfix);
		fprintf(fout,"%sqwtCounterSetMaxValue(p,%s,%f)%s\n",prefix,_C(iname),obj->maxVal(),postfix);
		fprintf(fout,"%sqwtCounterSetStepButton1(p,%s,%d)%s\n",prefix,_C(iname),obj->stepButton1(),postfix);
		fprintf(fout,"%sqwtCounterSetStepButton2(p,%s,%d)%s\n",prefix,_C(iname),obj->stepButton2(),postfix);
		fprintf(fout,"%sqwtCounterSetStepButton3(p,%s,%d)%s\n",prefix,_C(iname),obj->stepButton3(),postfix);
		fprintf(fout,"%sqwtCounterSetValue(p,%s,%f)%s\n",prefix,_C(iname),obj->value(),postfix);
		iitem++;
	}
	else if(type == "TQwtWheel")
	{
		MyQwtWheel *obj = (MyQwtWheel *) widget;
		fprintf(fout,"%spvQwtWheel(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		fprintf(fout,"%sqwtWheelSetMass(p,%s,%f)%s\n",prefix,_C(iname),obj->mass(),postfix);
		fprintf(fout,"%sqwtWheelSetTotalAngle(p,%s,%f)%s\n",prefix,_C(iname),obj->viewAngle(),postfix);
		fprintf(fout,"%sqwtWheelSetViewAngle(p,%s,%f)%s\n",prefix,_C(iname),obj->viewAngle(),postfix);
		fprintf(fout,"%sqwtWheelSetTickCnt(p,%s,%d)%s\n",prefix,_C(iname),obj->tickCnt(),postfix);
		fprintf(fout,"%sqwtWheelSetInternalBorder(p,%s,%d)%s\n",prefix,_C(iname),obj->internalBorder(),postfix);
		iitem++;
	}
	else if(type == "TQwtSlider")
	{
		MyQwtSlider *obj = (MyQwtSlider *) widget;
		fprintf(fout,"%spvQwtSlider(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		fprintf(fout,"%sqwtSliderSetThumbLength(p,%s,%d)%s\n",prefix,_C(iname),obj->thumbLength(),postfix);
		fprintf(fout,"%sqwtSliderSetThumbWidth(p,%s,%d)%s\n",prefix,_C(iname),obj->thumbWidth(),postfix);
		fprintf(fout,"%sqwtSliderSetBorderWidth(p,%s,%d)%s\n",prefix,_C(iname),obj->borderWidth(),postfix);
		if(obj->bgStyle() == QwtSlider::BgTrough)
			fprintf(fout,"%sqwtSliderSetBgStyle(p,%s,%sSliderBgTrough)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->bgStyle() == QwtSlider::BgSlot)
			fprintf(fout,"%sqwtSliderSetBgStyle(p,%s,%sSliderBgSlot)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->bgStyle() == QwtSlider::BgBoth)
			fprintf(fout,"%sqwtSliderSetBgStyle(p,%s,%sSliderBgBoth)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->scalePosition() == QwtSlider::NoScale)
			fprintf(fout,"%sqwtSliderSetScalePos(p,%s,%sSliderNone)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->scalePosition() == QwtSlider::LeftScale)
			fprintf(fout,"%sqwtSliderSetScalePos(p,%s,%sSliderLeft)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->scalePosition() == QwtSlider::RightScale)
			fprintf(fout,"%sqwtSliderSetScalePos(p,%s,%sSliderRight)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->scalePosition() == QwtSlider::TopScale)
			fprintf(fout,"%sqwtSliderSetScalePos(p,%s,%sSliderTop)%s\n",prefix,_C(iname),midfix,postfix);
		if(obj->scalePosition() == QwtSlider::BottomScale)
			fprintf(fout,"%sqwtSliderSetScalePos(p,%s,%sSliderBottom)%s\n",prefix,_C(iname),midfix,postfix);
		fprintf(fout,"%sqwtSliderSetValue(p,%s,%f)%s\n",prefix,_C(iname),obj->value(),postfix);
		iitem++;
	}
	else if(type == "TQwtCompass")
	{
		fprintf(fout,"%spvQwtCompass(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQwtAnalogClock")
	{
		fprintf(fout,"%spvQwtAnalogClock(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQwtDial")
	{
		fprintf(fout,"%spvQwtDial(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQDateEdit")
	{
		fprintf(fout,"%spvQDateEdit(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQTimeEdit")
	{
		fprintf(fout,"%spvQTimeEdit(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQDateTimeEdit")
	{
		fprintf(fout,"%spvQDateTimeEdit(p,%s,%s)%s\n",prefix,_C(iname),_C(pname),postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQCustomWidget")
	{
		if(whatsthis.length() < (int) (sizeof(buf) - 1))
		strcpy(buf,whatsthis.toUtf8());
		cptr = strchr(buf,':');
		if(cptr != NULL)
		{
			*cptr = '\0';
			cptr++;
		}
		if(cptr != NULL) fprintf(fout,"%spvQCustomWidget(p,%s,%s,\"%s\",\"%s\")%s\n",prefix,_C(iname),_C(pname),buf,cptr,postfix);
		else						 fprintf(fout,"%spvQCustomWidget(p,%s,%s,\"%s\")%s\n",prefix,_C(iname),_C(pname),buf,postfix);
		fprintf(fout,"%spvSetGeometry(p,%s,%d,%d,%d,%d)%s\n",prefix,_C(iname),x,y,w,h,postfix);
		iitem++;
	}
	else if(type == "TQVbox")
	{
		iitem++;
	}
	else if(type == "TQHbox")
	{
		iitem++;
	}
	else if(type == "TQGrid")
	{
		iitem++;
	}

	if(statustip.contains(":foreground:"))
	{
		QColor col;
		if(type == "TQPushButton") col = widget->palette().color(QPalette::ButtonText);
		else											 col = widget->palette().color(QPalette::WindowText);
		fprintf(fout,"%spvSetPaletteForegroundColor(p,%s,%d,%d,%d)%s\n",prefix,_C(iname),col.red(),col.green(),col.blue(),postfix);
	}
	if(statustip.contains(":background:"))
	{
		QColor col;
		if		 (type == "TQPushButton") col = widget->palette().color(QPalette::Button);
		else if(type == "TQRadio")			col = widget->palette().color(QPalette::Button);
		else if(type == "TQCheck")			col = widget->palette().color(QPalette::Button);
		else if(type == "TQMultiLineEdit" ||
						type == "TQLineEdit"			||
						type == "TQTextBrowser"		)
																		col = widget->palette().color(QPalette::Base);
		else														col = widget->palette().color(QPalette::Window);
		fprintf(fout,"%spvSetPaletteBackgroundColor(p,%s,%d,%d,%d)%s\n",prefix,_C(iname),col.red(),col.green(),col.blue(),postfix);
	}
	QFont f = widget->font();
	if(statustip.contains("font:"))
	{
		if(f.family()		 == "MS Shell Dlg 2" &&
			 f.pointSize() == 8 &&
			 f.bold()			 == 0 &&
			 f.italic()		 == 0 &&
			 f.underline() == 0 &&
			 f.strikeOut() == 0 )
		{
		}
		else
		{
			fprintf(fout,"%spvSetFont(p,%s,\"%s\",%d,%d,%d,%d,%d)%s\n",prefix,
				_C(iname),(const char *) f.family().toUtf8(),
				f.pointSize(),f.bold(),f.italic(),f.underline(),f.strikeOut(),postfix);
		}
	}
	else
	{ 
		if(f.family()		 != "Sans Serif" ||
			 f.pointSize() != 10					 ||
			 f.bold()			 == true				 ||
			 f.italic()		 == true				 ||
			 f.underline() == true				 ||
			 f.strikeOut() == true				 )
		{

			fprintf(fout,"%spvSetFont(p,%s,\"%s\",%d,%d,%d,%d,%d)%s\n",prefix,
				_C(iname),(const char *) f.family().toUtf8(),
				f.pointSize(),f.bold(),f.italic(),f.underline(),f.strikeOut(),postfix);
		}
	}
	if(!tooltip.isEmpty())	 fprintf(fout,"%spvToolTip(p,%s,%s)%s\n",prefix,_C(iname), quote(tooltip),postfix);
	if(!whatsthis.isEmpty()) fprintf(fout,"%spvSetWhatsThis(p,%s,%s)%s\n",prefix,_C(iname), quote(whatsthis),postfix);
	if(type == "TQLabel")
	{
		MyLabel *label = (MyLabel *) widget;
		QString shape, shadow;

		int linewidth = label->lineWidth();
		int margin = label->margin();

		shape = "NoFrame";
		if(label->frameShape() == QFrame::NoFrame)		 shape = "NoFrame";
		if(label->frameShape() == QFrame::Box)				 shape = "Box";
		if(label->frameShape() == QFrame::Panel)			 shape = "Panel";
		if(label->frameShape() == QFrame::StyledPanel) shape = "StyledPanel";
		if(label->frameShape() == QFrame::HLine)			 shape = "HLine";
		if(label->frameShape() == QFrame::VLine)			 shape = "VLine";
		if(label->frameShape() == QFrame::WinPanel)		 shape = "WinPanel";

		shadow = "Plain";
		if(label->frameShadow() == QFrame::Plain)  shadow = "Plain";
		if(label->frameShadow() == QFrame::Raised) shadow = "Raised";
		if(label->frameShadow() == QFrame::Sunken) shadow = "Sunken";

		if(label->frameShape() != QFrame::NoFrame)
			fprintf(fout,"%spvSetStyle(p,%s,%s%s,-1,-1,-1)%s\n",prefix,_C(iname),midfix,(const char *) shape.toUtf8(),postfix);
		if(label->frameShadow() != QFrame::Plain)
			fprintf(fout,"%spvSetStyle(p,%s,-1,%s%s,-1,-1)%s\n",prefix,_C(iname),midfix,(const char *) shadow.toUtf8(),postfix);
		if(linewidth != 1)
			fprintf(fout,"%spvSetStyle(p,%s,-1,-1,%d,-1)%s\n",prefix,_C(iname),linewidth,postfix);
		if(margin != 0)
			fprintf(fout,"%spvSetStyle(p,%s,-1,-1,-1,%d)%s\n",prefix,_C(iname),margin,postfix);
	}
	w = widget->minimumWidth();
	h = widget->minimumHeight();
	if(w > 0 || h > 0)
	{
		fprintf(fout,"%spvSetMinSize(p,%s,%d,%d)%s\n",prefix,_C(iname),w,h,postfix);
	}
	w = widget->maximumWidth();
	h = widget->maximumHeight();
	if(w < 5000 || h < 5000)
	{
		fprintf(fout,"%spvSetMaxSize(p,%s,%d,%d)%s\n",prefix,_C(iname),w,h,postfix);
	}
	fprintf(fout,"\n");
	return 0;
}

static int generateDefineMaskWidgets(FILE *fout, QWidget *root)
{
	QString item;
	QWidget *widget;
	char tabparentname[512];

	theroot = root;
	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	pv.pvStartDefinition(p,ID_END_OF_WIDGETS)\n");
		fprintf(fout,"\n");
	}
	else
	{
		fprintf(fout,"static int generated_defineMask(PARAM *p)\n");
		fprintf(fout,"{\n");
		fprintf(fout,"	int w,h,depth;\n");
		fprintf(fout,"\n");
		fprintf(fout,"	if(p == NULL) return 1;\n");
		fprintf(fout,"	if(widgetName[0] == NULL) return 1; // suppress unused warning\n");
		fprintf(fout,"	w = h = depth = strcmp(toolTip[0],whatsThis[0]);\n");
		fprintf(fout,"	if(widgetType[0] == -1) return 1;\n");
		fprintf(fout,"	if(w==h) depth=0; // fool the compiler\n");
		fprintf(fout,"	pvStartDefinition(p,ID_END_OF_WIDGETS);\n");
		fprintf(fout,"\n");
	}

	strcpy(tabparentname,"0");
	// loop over widgets
	for(int i=0; i<strlist.size(); i++)
	{
		item = strlist.at(i);
		widget = findChild(item.toLatin1()); //root->findChild<QWidget *>(item);

		if (widget == NULL) {
			fprintf(stderr, "Not found: %s\n", (const char *) item.toLatin1());
			return -1;
		}

		QString orig = QString(widget->objectName());
		QString cname;
		if (mangle_cname(widget->objectName(), cname) != 0) {
			cname = QString(widget->objectName());
		}
		const char *name = cname.toUtf8();

		if(widget->statusTip().startsWith("TQTabWidget:"))
		{
			strcpy(tabparentname,(const char *) name);
		}
		if(widget->statusTip().startsWith("TQToolBox:"))
		{
			strcpy(tabparentname,(const char *) name);
		}
		if(widget->statusTip().startsWith("TQWidget:"))
		{
			if(opt_develop.arg_debug) printf("tabparentname0=%s\n",(const char *) name);
			QWidget *p = (QWidget *) widget->parent();
			if(p != NULL)
			{
				strcpy(tabparentname,(const char *) p->objectName().toUtf8());
				if(opt_develop.arg_debug) printf("tabparentname1=%s\n",tabparentname);
				QWidget *gp = (QWidget *) p->parent();
				if(gp != NULL)
				{
					int what = 0;
					if(strcmp(tabparentname,"qt_scrollarea_viewport")			== 0) what = 1;
					if(strcmp(tabparentname,"qt_tabwidget_stackedwidget") == 0) what = 2;
					strcpy(tabparentname,(const char *) gp->objectName().toUtf8());
					if(opt_develop.arg_debug) printf("tabparentname2=%s\n",tabparentname);
					QWidget *ggp = (QWidget *) gp->parent();
					if(ggp != NULL && strlen(tabparentname) == 0 && what == 1)
					{
						strcpy(tabparentname,(const char *) ggp->objectName().toUtf8());
						if(opt_develop.arg_debug) printf("tabparentname3=%s\n",tabparentname);
					}
					// if(what == 0) strcpy(tabparentname,"ERROR_PLEASE_FIX_ME");
				}  
			}
		}
		if(widget != NULL)
		{
			generateDefineMaskWidget(fout,widget,tabparentname);
		}
		else
		{
			printf("WARNING generateDefineMaskWidgets:findChild=%s not found\n",(const char *) item.toUtf8());
		}
	}

	// generateLayout
	generateLayoutConstuctors(fout);
	generateLayoutDefinition(fout);

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	pv.pvEndDefinition(p);\n");
	}
	else
	{
		fprintf(fout,"	pvEndDefinition(p);\n");
	}

	// generate TabOrder
	for(int i=0; i<tablist.size(); i++)
	{
		QString item = tablist.at(i);
		fprintf(fout,"%s",(const char *) item.toUtf8().data());
	}

	if(opt_develop.script == PV_LUA)
	{
	}
	else
	{
		fprintf(fout,"	return 0;\n");
		fprintf(fout,"}\n");
		fprintf(fout,"\n");
	}
	return 0;
}

int drawDrawWidgets(QWidget *root)
{
	QString item;
	QWidget *widget;
	getStrList(root);
	theroot = root; // Important for the 'hack'
	// loop over widgets
	for(int i=0; i<strlist.size(); i++)
	{
		item = strlist.at(i);
		widget = findChild(item.toUtf8()); //root->findChild<QWidget *>(item);
		if (!widget) {
			printf("Widget '%s' not found", (const char *) item.toLatin1());
			return -1;
		}
		if(widget->statusTip().startsWith("TQDraw:"))
		{
			QString fname = widget->whatsThis();
			if(!fname.isEmpty())
			{
				QDrawWidget *dw = (QDrawWidget *) widget;
				dw->beginDraw(1);
				dw->playSVG(fname.toUtf8());
				dw->endDraw();
			}
		}
	}
	return 0;
}

static int generateToolTip(FILE *fout, QWidget *root)
{
	QString item,qbuf;
	QWidget *widget;

	theroot = root;
	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	toolTip = {}\n");
		fprintf(fout,"	toolTip[0] = \"\"\n");
	}
	else
	{
		fprintf(fout,"	static const char *toolTip[] = {\n");
		fprintf(fout,"	\"\",\n");
	}

	// loop over widgets
	for(int i=0; i<strlist.size(); i++)
	{
		item = strlist.at(i);
		widget = findChild(item.toUtf8()); //root->findChild<QWidget *>(item);
		if(widget != NULL)
		{
			qbuf = widget->toolTip();
			if(opt_develop.script == PV_LUA)
			{
				fprintf(fout,"	toolTip[%d] = \"%s\"\n",i+1,quote0(qbuf));
			}
			else
			{
				fprintf(fout,"	\"%s\",\n",quote0(qbuf));
			}
		}
		else
		{
			printf("WARNING generateToolTip:findChild=%s not found\n",(const char *) item.toUtf8());
		}
	}

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"\n");
	}
	else
	{
		fprintf(fout,"	\"\"};\n");
		fprintf(fout,"\n");
	}

	return 0;
}

static int generateWhatsThis(FILE *fout, QWidget *root)
{
	QString item,qbuf;
	QWidget *widget;

	theroot = root;
	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	whatsThis = {}\n");
		fprintf(fout,"	whatsThis[0] = \"\"\n");
	}
	else
	{
		fprintf(fout,"	static const char *whatsThis[] = {\n");
		fprintf(fout,"	\"\",\n");
	}

	// loop over widgets
	for(int i=0; i<strlist.size(); i++)
	{
		item = strlist.at(i);
		widget = findChild(item.toUtf8()); //root->findChild<QWidget *>(item);
		if(widget != NULL)
		{
			qbuf = widget->whatsThis();
			if(opt_develop.script == PV_LUA)
			{
				fprintf(fout,"	whatsThis[%d] = \"%s\"\n",i+1,quote0(qbuf));
			}
			else
			{
				fprintf(fout,"	\"%s\",\n",quote0(qbuf));
			}
		}
		else
		{
			printf("WARNING generateWhatsThis:findChild=%s not found\n",(const char *) item.toUtf8());
		}
	}

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"\n");
	}
	else
	{
		fprintf(fout,"	\"\"};\n");
		fprintf(fout,"\n");
	}
	return 0;
}

static int generateWidgetType(FILE *fout, QWidget *root)
{
	QString item,qbuf;
	QWidget *widget;
	char		buf[1024],*cptr;

	theroot = root;
	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	widgetType = {}\n");
		fprintf(fout,"	widgetType[0] = pv.TQWidget\n");
	}
	else
	{
		fprintf(fout,"	static const int widgetType[ID_END_OF_WIDGETS+1] = {\n	0,\n");
	}

	// loop over widgets
	for(int i=0; i<strlist.size(); i++)
	{
		item = strlist.at(i);
		widget = findChild(item.toUtf8()); //root->findChild<QWidget *>(item);
		if(widget != NULL)
		{
			qbuf = widget->statusTip(); // parse statusTip
			strcpy(buf,qbuf.toUtf8());
			cptr = strstr(buf,":");
			if(cptr != NULL) *cptr = '\0';
			if(strncmp(buf,"TQ",2) != 0) strcpy(buf,"0");
			if(opt_develop.script == PV_LUA)
			{
				fprintf(fout,"	widgetType[%d] = pv.%s\n",i+1,buf);
			}
			else
			{
				fprintf(fout,"	%s,\n",buf);
			}
		}
		else
		{
			printf("WARNING generateWidgetType:findChild=%s not found\n",(const char *) item.toUtf8());
		}
	}

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"\n");
	}
	else
	{
		fprintf(fout,"	-1 };\n");
		fprintf(fout,"\n");
	}
	return 0;
}

/*
	if		 (type == "TQWidget")
	{
	}
	else if(type == "TQPushButton")
	{
	}
	else if(type == "TQLabel")
	{
	}
	else if(type == "TQLineEdit")
	{
	}
	else if(type == "TQComboBox")
	{
	}
	else if(type == "TQLCDNumber")
	{
	}
	else if(type == "TQButtonGroup")
	{
	}
	else if(type == "TQRadio")
	{
	}
	else if(type == "TQCheck")
	{
	}
	else if(type == "TQSlider")
	{
	}
	else if(type == "TQFrame")
	{
	}
	else if(type == "TQImage")
	{
	}
	else if(type == "TQDraw")
	{
	}
	else if(type == "TQGl")
	{
	}
	else if(type == "TQTabWidget")
	{
	}
	else if(type == "TQGroupBox")
	{
	}
	else if(type == "TQListBox")
	{
	}
	else if(type == "TQTable")
	{
	}
	else if(type == "TQSpinBox")
	{
	}
	else if(type == "TQDial")
	{
	}
	else if(type == "TQProgressBar")
	{
	}
	else if(type == "TQMultiLineEdit")
	{
	}
	else if(type == "TQTextBrowser")
	{
	}
	else if(type == "TQListView")
	{
	}
	else if(type == "TQIconView")
	{
	}
	else if(type == "TQVtk")
	{
	}
	else if(type == "TQwtPlotWidget")
	{
	}
	else if(type == "TQwtScale")
	{
	}
	else if(type == "TQwtThermo")
	{
	}
	else if(type == "TQwtKnob")
	{
	}
	else if(type == "TQwtCounter")
	{
	}
	else if(type == "TQwtWheel")
	{
	}
	else if(type == "TQwtSlider")
	{
	}
	else if(type == "TQwtDial")
	{
	}
	else if(type == "TQwtCompass")
	{
	}
	else if(type == "TQwtAnalogClock")
	{
	}
	else if(type == "TQDateEdit")
	{
	}
	else if(type == "TQTimeEdit")
	{
	}
	else if(type == "TQDateTimeEdit")
	{
	}
	else if(type == "TQToolBox")
	{
	}
	else if(type == "TQCustomWidget")
	{
	}
	else if(type == "TQVbox")
	{
	}
	else if(type == "TQHbox")
	{
	}
	else if(type == "TQGrid")
	{
	}
*/

static int generateGeneratedArea(FILE *fout, QWidget *root)
{
	strlist.clear();
	iitem = 1;

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	--- begin construction of our mask -------------------------------------------------\n");
	}
	else
	{
		fprintf(fout,"// _begin_of_generated_area_ (do not edit -> use ui2pvc) -------------------\n");
		fprintf(fout,"// USE_CNAME_MANGLING (do not remove this)\n");

		fprintf(fout,"\n");
	}

	getStrList(root);
	generateWidgetEnum(fout, root, isEnum);
	generateWidgetEnum(fout, root, isName);
	generateToolTip(fout, root);
	generateWhatsThis(fout, root);
	generateWidgetType(fout,root);
	generateDefineMaskWidgets(fout, root);

	if(opt_develop.script == PV_LUA)
	{
		fprintf(fout,"	--- end construction of our mask ---------------------------------------------------\n");
	}
	else
	{
		fprintf(fout,"// _end_of_generated_area_ (do not edit -> use ui2pvc) ---------------------\n");
	}
	strlist.clear();
	return 0;
}

extern FileLines file_lines;

int generateMask(const char *filename, QWidget *root)
{
	FILE *fout;
	FileLines *fl;
	int found_begin, found_end, done_end, done;

	setlocale(LC_NUMERIC, "C");
	if(loadFile(filename) != 0) return -1;
	if(opt_develop.murx)
	{
		fout = fopen("murx.cpp","w");
	}
	else
	{
		fout = fopen(filename,"w");
	}
	if(fout == NULL)
	{
		unloadFile();
		printf("ERROR: generateMask could not write %s\n",filename);
		return -1;
	}
	if(opt_develop.arg_debug) printf("generateMask %s\n",filename);

	found_begin = found_end = done_end = done = 0;
	fl = &file_lines;
	fl = fl->next;
	while(fl != NULL)
	{
		if(opt_develop.script == PV_LUA)
		{
			if(strstr(fl->line,"--- begin construction of our mask ---") != NULL) found_begin = 1;
			if(strstr(fl->line,"--- end construction of our mask ---")	 != NULL) found_end = 1;
		}
		else
		{
			if(strstr(fl->line,"_begin_of_generated_area_") != NULL) found_begin = 1;
			if(strstr(fl->line,"_end_of_generated_area_")		!= NULL) found_end = 1;
		}
		if(found_begin == 0)
		{
			fprintf(fout,"%s",fl->line);
		}
		else if(found_begin == 1 && found_end == 0)
		{
			if(done==0) generateGeneratedArea(fout, root);
			done = 1;
		}
		else if(found_begin == 1 && found_end == 1)
		{
			if(done_end==1) fprintf(fout,"%s",fl->line);
			done_end = 1;
		}
		fl = fl->next;
	}

	fclose(fout);
	unloadFile();
	if(opt_develop.arg_debug) printf("generateMask end\n");
	return 0;
}

#if 0 // XXX
int lua_getWidgetsFromMask(const char *filename, QWidget *root)
{
	FILE *fin;
	int ret, found_begin, found_start;
	int found_enum_start, found_enum_end;
	int found_tooltip_start, found_tooltip_end;
	int found_whatsthis_start, found_whatsthis_end;

	iitem = 1;
	enumlist.clear();
	tooltiplist.clear();
	whatsthislist.clear();
	tablist.clear();
	if(editlayout != NULL)
	{
		editlayout->uidlg->textEditConstructors->clear();
		editlayout->uidlg->textEditDef->clear();
	}
	ret = 0;
	fin = fopen(filename,"r");
	if(fin == NULL)
	{
		printf("getWidgetsFromMask could not open %s\n", filename);
		return -1;
	}

	// find start
	found_begin = found_start = 0;
	found_enum_start = found_enum_end = 0;
	found_tooltip_start = found_tooltip_end = 0;
	found_whatsthis_start = found_whatsthis_end = 0;
	while(fgets(line,sizeof(line)-1,fin) != NULL)
	{
		if(strstr(line,"--- begin construction of our mask ---") != NULL) found_begin = 1;
		if(found_begin==1)
		{
			if(strstr(line,"ID_MAIN_WIDGET") != NULL) found_enum_start = 1;
			if(found_enum_start==1 && found_enum_end==0)
			{
				if(strstr(line,"=") == NULL) found_enum_end = 1;
				else appendEnum();
			}

			if(strstr(line,"toolTip") != NULL && strstr(line,"{}") == NULL) found_tooltip_start = 1;
			if(found_tooltip_start==1 && found_tooltip_end==0)
			{
				if(strstr(line,"=") == NULL) found_tooltip_end = 1;
				else appendToolTip();
			}

			if(strstr(line,"whatsThis") != NULL && strstr(line,"{}") == NULL) found_whatsthis_start = 1;
			if(found_whatsthis_start==1 && found_whatsthis_end==0)
			{
				if(strstr(line,"=") == NULL) found_whatsthis_end = 1;
				else appendWhatsThis();
			}

			if(strstr(line,"pvStartDefinition(p,") != NULL)
			{
				found_start = 1;
				break;
			}
		}
	}

	if(found_start==1)
	{
		getWidgets(fin,root, false);
		while(fgets(line,sizeof(line)-1,fin) != NULL)
		{
			if(isCommand("pvTabOrder(") == 1) // get TabOrder
			{
				tablist.append(QString::fromUtf8(line));
			}
			if(strstr(line,"return") != NULL) break;
		}
	}
	else
	{
		printf("WARNING getWidgetsFromMask could not get widgets from %s\n", filename);
		ret = -1;
	}

	fclose(fin);
	enumlist.clear();
	tooltiplist.clear();
	whatsthislist.clear();
	return ret;
}

#endif

int getWidgetsFromMask(const char *filename, QWidget *root)
{
	FILE *fin;
	int ret;


	iitem = 1;
	if (editlayout != NULL)
	{
		editlayout->uidlg->textEditConstructors->clear();
		editlayout->uidlg->textEditDef->clear();
	}
	ret = 0;
	if(opt_develop.arg_debug) printf("before fopen(%s)\n", filename);
	fin = fopen(filename,"r");
	if(fin == NULL)
	{
		printf("getWidgetsFromMask could not open %s\n", filename);
		return -1;
	}

	PvCppParser parser(fin, editlayout);

	ret = parser.parse(root);

	fclose(fin);

	return ret;
}

// add additional language here
#include "generatepython.h"
#include "generatephp.h"
#include "generateperl.h"
#include "generatetcl.h"
#include "generatelua.h"

#ifdef PVWIN32
static int win32Hack(QWidget *root, const char *uifile, QList<QObject *> *widgets)
{
	if(root == NULL || uifile == NULL) return -1;
	char line[4096], *cptr;
	FILE *fin;
	theroot = root;
	int  first = 1;

	fin = fopen(uifile,"r");
	if(fin == NULL)
	{
		printf("win32Hack: could not open %s\n", uifile);
		return -1;
	}

	widgets->clear();
	while(fgets(line, sizeof(line)-1, fin) != NULL)
	{
		//cptr = strstr(line, "<widget class=");
		cptr = strstr(line, "<widget");
		if(cptr != NULL)
		{
			cptr = strstr(cptr, "name=\"");
			if(cptr == NULL)
			{
				printf("win32Hack: ERROR no name in widget line=%s\n", line);
				return -1;
			}
			else
			{
				char *name;
				name = strchr(cptr, '\"');
				name++;
				cptr = strchr(name, '\"');
				if(cptr == NULL)
				{
					printf("win32Hack: ERROR no terminatin \" in widget line=%s\n", line);
					return -1;
				}
				else
				{
					*cptr = '\0';
					if(first == 0)
					{
						printf("win32Hack:findChild(%s)\n", name);
						QWidget *widget = findChild(name);
						if(widget == NULL)
						{
							printf("win32Hack: ERROR could not find widget name=%s\n", name);
							//return -1;
						}
						else
						{
							widgets->append(widget);
						}
					}
					first = 0;
				}
			}
		}
	}
	
	fclose(fin);
	return 0;
}
#endif

static int setWidgetTree(QWidget *root, const char *uifile)
{
	if(root == NULL) return -1;
	char buf[80];
	int iobj = 1;
	root->setObjectName("ID_MAIN_WIDGET");
	QList<QObject *> widgets;

	if(uifile == NULL) return -1; // uifile only needed for win32Hack()
#ifdef PVWIN32
	win32Hack(root, uifile, &widgets);
#else
	widgets = root->findChildren<QObject *>(QRegExp("*", Qt::CaseInsensitive, QRegExp::Wildcard));
#endif
	QWidget *widget;
	printf("widgets.size() = %d\n", widgets.size());
	for(int i=0; i<widgets.size(); i++)
	{
		widget = (QWidget *) widgets.at(i);
		if(widget->isWidgetType())
		{
			if(widget->objectName().startsWith("qt_"))
			{
				//printf("qt_: %s\n", (const char *) widget->objectName().toUtf8());
				sprintf(buf,"qtobj%d", iobj++);
				widget->setObjectName(buf);
			}
			else if(widget->objectName() == "")
			{
				//printf(": %s\n", (const char *) widget->objectName().toUtf8());
				sprintf(buf,"qtobj%d", iobj++);
				widget->setObjectName(buf);
			}
			else if(widget->inherits("PvbCustomWidget"))
			{
				widget->setStatusTip("TQCustomWidget:");
			}
			else if(widget->inherits("QPushButton"))
			{
				widget->setStatusTip("TQPushButton:");
			}
			else if(widget->inherits("QLineEdit"))
			{
				widget->setStatusTip("TQLineEdit:");
			}
			else if(widget->inherits("QComboBox"))
			{
				widget->setStatusTip("TQComboBox:");
			}
			else if(widget->inherits("QLCDNumber"))
			{
				widget->setStatusTip("TQLCDNumber:");
			}
			else if(widget->inherits("Qunknown"))
			{
				widget->setStatusTip("TQButtonGroup:");
			}
			else if(widget->inherits("QRadioButton"))
			{
				widget->setStatusTip("TQRadio:");
			}
			else if(widget->inherits("QCheckBox"))
			{
				widget->setStatusTip("TQCheck:");
			}
			else if(widget->inherits("QSlider"))
			{
				widget->setStatusTip("TQSlider:");
			}
			else if(widget->inherits("QTableWidget"))
			{
				widget->setStatusTip("TQTable:");
			}
			else if(widget->inherits("PvbImage"))
			{
				widget->setStatusTip("TQImage:");
			}
			else if(widget->inherits("PvbDraw"))
			{
				widget->setStatusTip("TQDraw:");
			}
			else if(widget->inherits("PvbOpengl"))
			{
				widget->setStatusTip("TQGl:");
			}
			else if(widget->inherits("QTabWidget"))
			{
				widget->setStatusTip("TQTabWidget:");
			}
			else if(widget->inherits("QGroupBox"))
			{
				widget->setStatusTip("TQGroupBox:");
			}
			else if(widget->inherits("Qunknown"))
			{
				widget->setStatusTip("TQListBox:");
			}
			else if(widget->inherits("QSpinBox"))
			{
				widget->setStatusTip("TQSpinBox:");
			}
			else if(widget->inherits("QDial"))
			{
				widget->setStatusTip("TQDial:");
			}
			else if(widget->inherits("QProgressBar"))
			{
				widget->setStatusTip("TQProgressBar:");
			}
			else if(widget->inherits("QTextBrowser"))
			{
				widget->setStatusTip("TQTextBrowser:");
			}
			else if(widget->inherits("QTextEdit"))
			{
				widget->setStatusTip("TQMultiLineEdit:");
			}
//			else if(widget->inherits("QListView"))
//			{
//				widget->setStatusTip("TQListView:");
//			}
//			else if(widget->inherits("QListWidget"))
//			{
//				widget->setStatusTip("TQListView:");
//			}
			else if(widget->inherits("QTreeView"))
			{
				widget->setStatusTip("TQListView:");
			}
			else if(widget->inherits("QListView"))
			{
				widget->setStatusTip("TQListBox:");
			}
			else if(widget->inherits("PvbIconview"))
			{
				widget->setStatusTip("TQIconView:");
			}
			else if(widget->inherits("PvbVtk"))
			{
				widget->setStatusTip("TQVtk:");
			}
			else if(widget->inherits("QwtPlot"))
			{
				widget->setStatusTip("TQwtPlotWidget:");
			}
			else if(widget->inherits("Qunknown"))
			{
				widget->setStatusTip("TQwtScale:");
			}
			else if(widget->inherits("QwtThermo"))
			{
				widget->setStatusTip("TQwtThermo:");
			}
			else if(widget->inherits("QwtKnob"))
			{
				widget->setStatusTip("TQwtKnob:");
			}
			else if(widget->inherits("QwtCounter"))
			{
				widget->setStatusTip("TQwtCounter:");
			}
			else if(widget->inherits("QwtWheel"))
			{
				widget->setStatusTip("TQwtWheel:");
			}
			else if(widget->inherits("QwtSlider"))
			{
				widget->setStatusTip("TQwtSlider:");
			}
			else if(widget->inherits("QwtCompass"))
			{
				widget->setStatusTip("TQwtCompass:");
			}
			else if(widget->inherits("QwtAnalogClock"))
			{
				widget->setStatusTip("TQwtAnalogClock:");
			}
			else if(widget->inherits("QwtDial"))
			{
				widget->setStatusTip("TQwtDial:");
			}
			else if(widget->inherits("QDateEdit"))
			{
				widget->setStatusTip("TQDateEdit:");
			}
			else if(widget->inherits("QTimeEdit"))
			{
				widget->setStatusTip("TQTimeEdit:");
			}
			else if(widget->inherits("QDateTimeEdit"))
			{
				widget->setStatusTip("TQDateTimeEdit:");
			}
			else if(widget->inherits("QToolBox"))
			{
				widget->setStatusTip("TQToolBox:");
			}
			else if(widget->inherits("Q"))
			{
				widget->setStatusTip("TQVbox:");
			}
			else if(widget->inherits("Q"))
			{
				widget->setStatusTip("TQHbox:");
			}
			else if(widget->inherits("Q"))
			{
				widget->setStatusTip("TQGrid:");
			}
			else if(widget->inherits("QLabel"))
			{
				widget->setStatusTip("TQLabel:");
			}
			else if(widget->inherits("QFrame"))
			{
				widget->setStatusTip("TQFrame:");
			}
			else
			{
				widget->setStatusTip("TQWidget:");
			}
		}
		else // not a widget type
		{
			//printf("not a widget type %s\n", (const char *) widget->objectName().toUtf8());
			if		 (widget->inherits("QVBoxLayout"))
			{
				printf("TQVbox\n");
				//widget->setStatusTip("TQVbox:");
			}
			else if(widget->inherits("QHBoxLayout"))
			{
				printf("TQHbox\n");
				//widget->setStatusTip("TQHbox:");
			}
			else if(widget->inherits("QGridLayout"))
			{
				printf("TQGrid\n");
				//widget->setStatusTip("TQGrid:");
			}
		}
	}
	return 0;
}

static int perhapsSetTabOrder(const char *uifile)
{
	FILE *fin;
	char *cptrbegin, *cptrend;
	QString lastTab, currentTab;

	tablist.clear();
	fin = fopen(uifile,"r");
	if(fin == NULL)
	{
		printf("could not open %s\n", uifile);
		return -1;
	}

	while(fgets(line, sizeof(line)-1, fin) != NULL)
	{
		if(strstr(line,"<tabstop>") != NULL)
		{
			cptrbegin = strchr(line,'>');
			if(cptrbegin != NULL)
			{
				cptrbegin++;
				cptrend = strchr(cptrbegin,'<');
				if(cptrend != NULL)
				{
					*cptrend = '\0';
					if(lastTab == "")
					{
						lastTab = cptrbegin;
					}
					else
					{
						currentTab = cptrbegin;
						if(opt_develop.script == PV_LUA)
						{
							tablist.append("	pv.pvTabOrder(p," + lastTab + "," + currentTab + ")\n");
						}
						else
						{
							tablist.append("	pvTabOrder(p," + lastTab + "," + currentTab + ");\n");
						}
						lastTab = currentTab;
					}  
				}
			}
		}
	}

	fclose(fin);
	return 0;
}

static int perhapsSetLayout(const char *cppfile)
{
	FILE *fin;
	if(editlayout == NULL) return -1;
	QTextEdit *e		= editlayout->uidlg->textEditConstructors;
	QTextEdit *edef = editlayout->uidlg->textEditDef;
	int found = 0;

	e->clear();
	fin = fopen(cppfile,"r");
	if(fin == NULL)
	{
		printf("could not open %s\n", cppfile);
		return -1;
	}

	while(fgets(line, sizeof(line)-1, fin) != NULL)
	{
		if(strstr(line,"pvQLayout")				!= NULL) found = 1;
		if(strstr(line,"pvEndDefinition") != NULL) break;
		if(found == 1)
		{
			if(strstr(line,"pvAdd") != NULL)
			{
				edef->append(line);
			}
			else
			{
				e->append(line);
			}  
		}
	}

	fclose(fin);
	return 0;
}

int importUi(const char *uifile, Designer *designer)
{
	char filename[80], *cptr;
	int imask;
	QWidget *root;
	char buf[MAXOPT_DEVELOP];

	printf("importUi(%s)\n", uifile);
	if(strlen(uifile) > 70)
	{
		printf("filename too long %s\n", uifile);
		return -1;
	}

	if(opt_develop.arg_debug) printf("before fopen(uifile=%s)\n", uifile);
	FILE *fin = fopen(uifile,"r");
	if(fin != NULL)
	{
		while(fgets(buf,sizeof(buf),fin) != NULL)
		{
			if(strstr(buf,"<layout") != NULL)
			{
				printf("ATTENTION: You are not allowed to define a layout management in Qt Designer and import the ui file to pvdevelop\n");
				printf("					 You must define a possible layout management within pvdevelop instead.\n");
				printf("					 IMPORT ABORTED\n");
				fclose(fin);
				return -1;
			}
		}  
	}
	else
	{
		printf("Usage Error: Before you can import a ui-file you must create the framework for the mask with pvdevelop.\n");
		return -1;
	}
	fclose(fin);

	if(opt_develop.arg_debug) printf("perhapsSetTabOrder(%s)\n", uifile);
	perhapsSetTabOrder(uifile);
	QFile ui(uifile);
	if(opt_develop.arg_debug) printf("ui.open()\n");
	bool ret = ui.open(QIODevice::ReadOnly);
	if(ret == false)
	{
		printf("could not open %s\n", uifile);
		return -1;
	}
	if(designer->root == NULL)
	{
		printf("usage error: frame of mask must be already created before you can import from ui-file.");
		return -1;
	}
	if(opt_develop.arg_debug) printf("uiloader()\n");
	QUiLoader uiloader(designer->root);
	if( (root = uiloader.load(&ui)) == NULL)
	{
		printf("could not load ui file %s\n", uifile);
		return -1;
	}
	strcpy(filename, uifile);
	cptr = strchr(filename, '.');
	if(cptr != NULL) *cptr = '\0';
	if(opt_develop.script == PV_LUA)
	{
		strcat(filename,".lua");
	}
	else
	{
		strcat(filename,".cpp");
	}
	sscanf(filename, "mask%d", &imask);
	perhapsSetLayout(filename);
	if(setWidgetTree(root, uifile) < 0)
	{
		printf("error: setWidgetTree %s\n", uifile);
		return -1;
	}
	designer->root = (MyRootWidget *) root;
	generateMask(filename, designer->root);
	// add additional language here
	if(opt_develop.script == PV_PYTHON)
	{
		generatePython(imask, designer->root);
	}
	if(opt_develop.script == PV_PHP)
	{
		generatePHP(imask, designer->root);
	}
	if(opt_develop.script == PV_PERL)
	{
		generatePerl(imask, designer->root);
	}
	if(opt_develop.script == PV_TCL)
	{
		generateTcl(imask, designer->root);
	}
	if(opt_develop.script == PV_LUA)
	{
		generateLua(imask, designer->root);
	}
	designer->root->modified = 0;
	return 0;
}


