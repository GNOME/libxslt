<![CDATA[
/*
 * libxslt_tutorial.c: demo program for the XSL Transformation 1.0 engine
 *
 * based on xsltproc.c, by Daniel.Veillard@imag.fr
 * by John Fleck 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 *
 */ 

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/DOCBparser.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>



extern int xmlLoadExtDtdDefaultValue;

static void usage(const char *name) {
    printf("Usage: %s [options] stylesheet file [file ...]\n", name);
}

int
main(int argc, char **argv) {
	xsltStylesheetPtr cur = NULL;
	xmlDocPtr doc, res;

	if (argc <= 1) {
		usage(argv[0]);
		return(1);
	}

	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;
	cur = xsltParseStylesheetFile((const xmlChar *)argv[1]);
	doc = xmlParseFile(argv[2]);
	res = xsltApplyStylesheet(cur, doc, NULL);
	xsltSaveResultToFile(stdout, res, cur);

	xsltFreeStylesheet(cur);
	xmlFreeDoc(res);
	xmlFreeDoc(doc);

	return(0);

}
]]>
