/*
 * preproc.c: Preprocessing of style operations
 *
 * References:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 *   Michael Kay "XSLT Programmer's Reference" pp 637-643
 *   Writing Multiple Output Files
 *
 *   XSLT-1.1 Working Draft
 *   http://www.w3.org/TR/xslt11#multiple-output
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include "xsltconfig.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/uri.h>
#include <libxml/xmlerror.h>
#include "xslt.h"
#include "xsltutils.h"
#include "xsltInternals.h"
#include "transform.h"
#include "templates.h"
#include "variables.h"
#include "numbersInternals.h"
#include "preproc.h"
#include "extra.h"
#include "imports.h"

#define DEBUG_PREPROC


/************************************************************************
 *									*
 *			handling of precomputed data			*
 *									*
 ************************************************************************/

/**
 * xsltNewStylePreComp:
 * @ctxt:  an XSLT processing context
 * @type:  the construct type
 *
 * Create a new XSLT Style precomputed block
 *
 * Returns the newly allocated xsltStylePreCompPtr or NULL in case of error
 */
static xsltStylePreCompPtr
xsltNewStylePreComp(xsltTransformContextPtr ctxt, xsltStyleType type) {
    xsltStylePreCompPtr cur;

    cur = (xsltStylePreCompPtr) xmlMalloc(sizeof(xsltStylePreComp));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewStylePreComp : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltStylePreComp));

    cur->type = type;
    switch (cur->type) {
        case XSLT_FUNC_COPY:
            cur->func = xsltCopy;break;
        case XSLT_FUNC_SORT:
            cur->func = xsltSort;break;
        case XSLT_FUNC_TEXT:
            cur->func = xsltText;break;
        case XSLT_FUNC_ELEMENT:
            cur->func = xsltElement;break;
        case XSLT_FUNC_ATTRIBUTE:
            cur->func = xsltAttribute;break;
        case XSLT_FUNC_COMMENT:
            cur->func = xsltComment;break;
        case XSLT_FUNC_PI:
            cur->func = xsltProcessingInstruction;break;
        case XSLT_FUNC_COPYOF:
            cur->func = xsltCopyOf;break;
        case XSLT_FUNC_VALUEOF:
            cur->func = xsltValueOf;break;
        case XSLT_FUNC_NUMBER:
            cur->func = xsltNumber;break;
        case XSLT_FUNC_APPLYIMPORTS:
            cur->func = xsltApplyImports;break;
        case XSLT_FUNC_CALLTEMPLATE:
            cur->func = xsltCallTemplate;break;
        case XSLT_FUNC_APPLYTEMPLATES:
            cur->func = xsltApplyTemplates;break;
        case XSLT_FUNC_CHOOSE:
            cur->func = xsltChoose;break;
        case XSLT_FUNC_IF:
            cur->func = xsltIf;break;
        case XSLT_FUNC_FOREACH:
            cur->func = xsltForEach;break;
        case XSLT_FUNC_DOCUMENT:
            cur->func = xsltDocumentElem;break;
    }
    if (cur->func == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewStylePreComp : no function for type %d\n", type);
    }
    cur->next = ctxt->preComps;
    ctxt->preComps = cur;

    return(cur);
}

/**
 * xsltFreeStylePreComp:
 * @comp:  an XSLT Style precomputed block
 *
 * Free up the memory allocated by @comp
 */
static void
xsltFreeStylePreComp(xsltStylePreCompPtr comp) {
    if (comp == NULL)
	return;
    if (comp->inst != NULL)
	comp->inst->_private = NULL;
    if (comp->stype != NULL)
	xmlFree(comp->stype);
    if (comp->order != NULL)
	xmlFree(comp->order);
    if (comp->use != NULL)
	xmlFree(comp->use);
    if (comp->name != NULL)
	xmlFree(comp->name);
    if (comp->ns != NULL)
	xmlFree(comp->ns);
    if (comp->mode != NULL)
	xmlFree(comp->mode);
    if (comp->modeURI != NULL)
	xmlFree(comp->modeURI);
    if (comp->test != NULL)
	xmlFree(comp->test);
    if (comp->select != NULL)
	xmlFree(comp->select);

    if (comp->filename != NULL)
	xmlFree(comp->filename);

    if (comp->numdata.level != NULL)
	xmlFree(comp->numdata.level);
    if (comp->numdata.count != NULL)
	xmlFree(comp->numdata.count);
    if (comp->numdata.from != NULL)
	xmlFree(comp->numdata.from);
    if (comp->numdata.value != NULL)
	xmlFree(comp->numdata.value);
    if (comp->numdata.format != NULL)
	xmlFree(comp->numdata.format);
    if (comp->comp != NULL)
	xmlXPathFreeCompExpr(comp->comp);
    if (comp->nsList != NULL)
	xmlFree(comp->nsList);

    memset(comp, -1, sizeof(xsltStylePreComp));

    xmlFree(comp);
}


/************************************************************************
 *									*
 *		    XSLT-1.1 extensions					*
 *									*
 ************************************************************************/

/**
 * xsltDocumentComp:
 * @ctxt:  an XSLT processing context
 * @inst:  the instruction in the stylesheet
 *
 * Pre process an XSLT-1.1 document element
 */
static void
xsltDocumentComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *filename = NULL;
    xmlChar *base = NULL;
    xmlChar *URL = NULL;

    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_DOCUMENT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
    comp->ver11 = 0;

    if (xmlStrEqual(inst->name, (const xmlChar *) "output")) {
#ifdef DEBUG_EXTRA
	xsltGenericDebug(xsltGenericDebugContext,
	    "Found saxon:output extension\n");
#endif
	filename = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			 (const xmlChar *)"file",
			 XSLT_SAXON_NAMESPACE, &comp->has_filename);
    } else if (xmlStrEqual(inst->name, (const xmlChar *) "write")) {
#ifdef DEBUG_EXTRA
	xsltGenericDebug(xsltGenericDebugContext,
	    "Found xalan:write extension\n");
#endif
	filename = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			 (const xmlChar *)"select",
			 XSLT_XALAN_NAMESPACE, &comp->has_filename);
    } else if (xmlStrEqual(inst->name, (const xmlChar *) "document")) {
	filename = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			 (const xmlChar *)"href",
			 XSLT_XT_NAMESPACE, &comp->has_filename);
	if (filename == NULL) {
#ifdef DEBUG_EXTRA
	    xsltGenericDebug(xsltGenericDebugContext,
		"Found xslt11:document construct\n");
#endif
	    filename = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			     (const xmlChar *)"href",
			     XSLT_NAMESPACE, &comp->has_filename);
	    comp->ver11 = 1;
	} else {
#ifdef DEBUG_EXTRA
	    xsltGenericDebug(xsltGenericDebugContext,
		"Found xt:document extension\n");
#endif
	    comp->ver11 = 0;
	}
    }
    if (!comp->has_filename) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltDocumentComp: could not find the href\n");
	goto error;
    }

    if (filename != NULL) {
	/*
	 * Compute output URL
	 */
	base = xmlNodeGetBase(inst->doc, inst);
	URL = xmlBuildURI(filename, base);
	if (URL == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltDocumentComp: URL computation failed %s\n", filename);
	    comp->filename = xmlStrdup(filename);
	} else {
	    comp->filename = URL;
	}
    } else {
	comp->filename = NULL;
    }

error:
    if (base != NULL)
	xmlFree(base);
    if (filename != NULL)
	xmlFree(filename);
}

/************************************************************************
 *									*
 *		Most of the XSLT-1.0 transformations			*
 *									*
 ************************************************************************/

/**
 * xsltSortComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt sort node
 *
 * Process the xslt sort node on the source node
 */
static void
xsltSortComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;


    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_SORT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->stype = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			 (const xmlChar *)"data-type",
			 XSLT_NAMESPACE, &comp->has_stype);
    if (comp->stype != NULL) {
	if (xmlStrEqual(comp->stype, (const xmlChar *) "text"))
	    comp->number = 0;
	else if (xmlStrEqual(comp->stype, (const xmlChar *) "number"))
	    comp->number = 1;
	else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSortComp: no support for data-type = %s\n", comp->stype);
	    comp->number = -1;
	}
    }
    comp->order = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			      (const xmlChar *)"order",
			      XSLT_NAMESPACE, &comp->has_order);
    if (comp->order != NULL) {
	if (xmlStrEqual(comp->order, (const xmlChar *) "ascending"))
	    comp->descending = 0;
	else if (xmlStrEqual(comp->order, (const xmlChar *) "descending"))
	    comp->descending = 1;
	else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSortComp: invalid value %s for order\n", comp->order);
	    comp->descending = -1;
	}
    }
    /* TODO: xsl:sort lang attribute */
    /* TODO: xsl:sort case-order attribute */

    comp->select = xmlGetNsProp(inst,(const xmlChar *)"select", XSLT_NAMESPACE);
    if (comp->select == NULL) {
	comp->select = xmlNodeGetContent(inst);
	if (comp->select == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSortComp: select is not defined\n");
	}
    }
}

/**
 * xsltCopyComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt copy node
 *
 * Process the xslt copy node on the source node
 */
static void
xsltCopyComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;


    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_COPY);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;


    comp->use = xmlGetNsProp(inst, (const xmlChar *)"use-attribute-sets",
				    XSLT_NAMESPACE);
    if (comp->use == NULL)
	comp->has_use = 0;
    else
	comp->has_use = 1;
}

/**
 * xsltTextComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt text node
 *
 * Process the xslt text node on the source node
 */
static void
xsltTextComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_TEXT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
    comp->noescape = 0;

    prop = xmlGetNsProp(inst,
	    (const xmlChar *)"disable-output-escaping",
			XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    comp->noescape = 1;
	} else if (!xmlStrEqual(prop,
				(const xmlChar *)"no")){
	    xsltGenericError(xsltGenericErrorContext,
"xslt:text: disable-output-escaping allow only yes or no\n");
	}
	xmlFree(prop);
    }
}

/**
 * xsltElementComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt element node
 *
 * Process the xslt element node on the source node
 */
static void
xsltElementComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_ELEMENT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * TODO: more computation can be done there, especially namespace lookup
     */
    comp->name = xsltEvalStaticAttrValueTemplate(ctxt, inst,
				 (const xmlChar *)"name",
				 XSLT_NAMESPACE, &comp->has_name);
    comp->ns = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			 (const xmlChar *)"namespace",
			 XSLT_NAMESPACE, &comp->has_ns);

    comp->use = xsltEvalStaticAttrValueTemplate(ctxt, inst,
		       (const xmlChar *)"use-attribute-sets",
		       XSLT_NAMESPACE, &comp->has_use);
}

/**
 * xsltAttributeComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt attribute node
 *
 * Process the xslt attribute node on the source node
 */
static void
xsltAttributeComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_ATTRIBUTE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * TODO: more computation can be done there, especially namespace lookup
     */
    comp->name = xsltEvalStaticAttrValueTemplate(ctxt, inst,
				 (const xmlChar *)"name",
				 XSLT_NAMESPACE, &comp->has_name);
    comp->ns = xsltEvalStaticAttrValueTemplate(ctxt, inst,
			 (const xmlChar *)"namespace",
			 XSLT_NAMESPACE, &comp->has_ns);

}

/**
 * xsltCommentComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt comment node
 *
 * Process the xslt comment node on the source node
 */
static void
xsltCommentComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_COMMENT);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
}

/**
 * xsltProcessingInstructionComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt processing-instruction node
 *
 * Process the xslt processing-instruction node on the source node
 */
static void
xsltProcessingInstructionComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_PI);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->name = xsltEvalStaticAttrValueTemplate(ctxt, inst,
				 (const xmlChar *)"name",
				 XSLT_NAMESPACE, &comp->has_name);
}

/**
 * xsltCopyOfComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt copy-of node
 *
 * Process the xslt copy-of node on the source node
 */
static void
xsltCopyOfComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_COPYOF);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:copy-of : select is missing\n");
    }
}

/**
 * xsltValueOfComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt value-of node
 *
 * Process the xslt value-of node on the source node
 */
static void
xsltValueOfComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_VALUEOF);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    prop = xmlGetNsProp(inst,
	    (const xmlChar *)"disable-output-escaping",
			XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    comp->noescape = 1;
	} else if (!xmlStrEqual(prop,
				(const xmlChar *)"no")){
	    xsltGenericError(xsltGenericErrorContext,
"value-of: disable-output-escaping allow only yes or no\n");
	}
	xmlFree(prop);
    }
    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);
    if (comp->select == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:value-of : select is missing\n");
    }
}

/**
 * xsltNumberComp:
 * @ctxt:  a XSLT process context
 * @cur:   the xslt number node
 *
 * Process the xslt number node on the source node
 */
static void
xsltNumberComp(xsltTransformContextPtr ctxt, xmlNodePtr cur) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((ctxt == NULL) || (cur == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_NUMBER);
    if (comp == NULL)
	return;
    cur->_private = comp;

    if ((ctxt == NULL) || (cur == NULL))
	return;

    comp->numdata.doc = cur->doc;
    comp->numdata.node = cur;
    comp->numdata.value = xmlGetNsProp(cur, (const xmlChar *)"value",
	                                XSLT_NAMESPACE);
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"format", XSLT_NAMESPACE);
    if (prop != NULL) {
	comp->numdata.format = prop;
    } else {
	comp->numdata.format = xmlStrdup(BAD_CAST("1"));
    }
    
    comp->numdata.count = xmlGetNsProp(cur, (const xmlChar *)"count",
					XSLT_NAMESPACE);
    comp->numdata.from = xmlGetNsProp(cur, (const xmlChar *)"from",
					XSLT_NAMESPACE);
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"level", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, BAD_CAST("single")) ||
	    xmlStrEqual(prop, BAD_CAST("multiple")) ||
	    xmlStrEqual(prop, BAD_CAST("any"))) {
	    comp->numdata.level = prop;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			 "xsl:number : invalid value %s for level\n", prop);
	    xmlFree(prop);
	}
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"lang", XSLT_NAMESPACE);
    if (prop != NULL) {
	TODO; /* xsl:number lang attribute */
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"letter-value", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, BAD_CAST("alphabetic"))) {
	    TODO; /* xsl:number letter-value attribute alphabetic */
	} else if (xmlStrEqual(prop, BAD_CAST("traditional"))) {
	    TODO; /* xsl:number letter-value attribute traditional */
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		     "xsl:number : invalid value %s for letter-value\n", prop);
	}
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-separator", XSLT_NAMESPACE);
    if (prop != NULL) {
	comp->numdata.groupingCharacter = prop[0];
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-size", XSLT_NAMESPACE);
    if (prop != NULL) {
	sscanf((char *)prop, "%d", &comp->numdata.digitsPerGroup);
	xmlFree(prop);
    } else {
	comp->numdata.groupingCharacter = 0;
    }

    /* Set default values */
    if (comp->numdata.value == NULL) {
	if (comp->numdata.level == NULL) {
	    comp->numdata.level = xmlStrdup(BAD_CAST("single"));
	}
    }
    
}

/**
 * xsltApplyImportsComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt apply-imports node
 *
 * Process the xslt apply-imports node on the source node
 */
static void
xsltApplyImportsComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_APPLYIMPORTS);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
}

/**
 * xsltCallTemplateComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt call-template node
 *
 * Process the xslt call-template node on the source node
 */
static void
xsltCallTemplateComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlNsPtr ns = NULL;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_CALLTEMPLATE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * The full template resolution can be done statically
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:call-template : name is missing\n");
    } else {

	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname == NULL) {
	    ncname = prop;
	    prop = NULL;
	    prefix = NULL;
	}
	if (prefix != NULL) {
	    ns = xmlSearchNs(ctxt->insert->doc, ctxt->insert, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "no namespace bound to prefix %s\n", prefix);
	    }
	}
        if (ns != NULL)
	    comp->templ = xsltFindTemplate(ctxt, ncname, ns->href);
	else
	    comp->templ = xsltFindTemplate(ctxt, ncname, NULL);

	if (comp->templ == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:call-template : template %s not found\n", ncname);
	}
    }

    /* TODO: with-param could be optimized too */

    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}

/**
 * xsltApplyTemplatesComp:
 * @ctxt:  a XSLT process context
 * @inst:  the apply-templates node
 *
 * Process the apply-templates node on the source node
 */
static void
xsltApplyTemplatesComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;
    xmlChar *prop;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_APPLYTEMPLATES);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    /*
     * Get mode if any
     */
    prop = xmlGetNsProp(inst, (const xmlChar *)"mode", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *prefix = NULL;

	comp->mode = xmlSplitQName2(prop, &prefix);
	if (comp->mode != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(inst->doc, inst, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    xmlFree(prefix);
		    xmlFree(comp->mode);
		    comp->mode = prop;
		    comp->modeURI = NULL;
		} else {
		    comp->modeURI = xmlStrdup(ns->href);
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		xmlFree(prop);
		comp->modeURI = NULL;
	    }
	} else {
	    comp->mode = prop;
	    comp->modeURI = NULL;
	}
    }
    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);

    /* TODO: handle (or skip) the xsl:sort and xsl:with-param */
}

/**
 * xsltChooseComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt choose node
 *
 * Process the xslt choose node on the source node
 */
static void
xsltChooseComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_CHOOSE);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;
}

/**
 * xsltIfComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt if node
 *
 * Process the xslt if node on the source node
 */
static void
xsltIfComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_IF);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->test = xmlGetNsProp(inst, (const xmlChar *)"test", XSLT_NAMESPACE);
    if (comp->test == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltIf: test is not defined\n");
	return;
    }
}

/**
 * xsltForEachComp:
 * @ctxt:  a XSLT process context
 * @inst:  the xslt for-each node
 *
 * Process the xslt for-each node on the source node
 */
static void
xsltForEachComp(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    xsltStylePreCompPtr comp;

    if ((ctxt == NULL) || (inst == NULL))
	return;
    comp = xsltNewStylePreComp(ctxt, XSLT_FUNC_FOREACH);
    if (comp == NULL)
	return;
    inst->_private = comp;
    comp->inst = inst;

    comp->select = xmlGetNsProp(inst, (const xmlChar *)"select",
	                        XSLT_NAMESPACE);

    /* TODO: handle and skip the xsl:sort */
}


/************************************************************************
 *									*
 *		    Generic interface					*
 *									*
 ************************************************************************/

/**
 * xsltFreeStylePreComps:
 * @ctxt:  an XSLT transformation context
 *
 * Free up the memory allocated by all precomputed blocks
 */
void
xsltFreeStylePreComps(xsltTransformContextPtr ctxt) {
    xsltStylePreCompPtr cur, next;

    if (ctxt == NULL)
	return;
    cur = ctxt->preComps;
    while (cur != NULL) {
	next = cur->next;
	xsltFreeStylePreComp(cur);
	cur = next;
    }
}

/**
 * xsltDocumentCompute:
 * @ctxt:  an XSLT processing context
 * @inst:  the instruction in the stylesheet
 *
 * Precompute an XSLT stylesheet element
 */
void
xsltStylePreCompute(xsltTransformContextPtr ctxt, xmlNodePtr inst) {
    if (inst->_private != NULL) 
        return;
    if (IS_XSLT_ELEM(inst)) {
	xsltStylePreCompPtr cur;

	if (IS_XSLT_NAME(inst, "apply-templates")) {
	    xsltApplyTemplatesComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "value-of")) {
	    xsltValueOfComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "copy")) {
	    xsltCopyComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "copy-of")) {
	    xsltCopyOfComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "if")) {
	    xsltIfComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "choose")) {
	    xsltChooseComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "for-each")) {
	    xsltForEachComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "apply-imports")) {
	    xsltApplyImportsComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "attribute")) {
	    xsltAttributeComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "element")) {
	    xsltElementComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "text")) {
	    xsltTextComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "sort")) {
	    xsltSortComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "comment")) {
	    xsltCommentComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "number")) {
	    xsltNumberComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "processing-instruction")) {
	    xsltProcessingInstructionComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "call-template")) {
	    xsltCallTemplateComp(ctxt, inst);
	} else if (IS_XSLT_NAME(inst, "param")) {
	    /* TODO: is there any use optimizing param too ? */
	    return;
	} else if (IS_XSLT_NAME(inst, "variable")) {
	    /* TODO: is there any use optimizing variable too ? */
	    return;
	} else if (IS_XSLT_NAME(inst, "message")) {
	    /* no optimization needed */
	    return;
	} else if (IS_XSLT_NAME(inst, "document")) {
	    xsltDocumentComp(ctxt, inst);
	} else {
	    xsltGenericError(xsltGenericDebugContext,
		 "xsltStylePreCompute: unknown xslt:%s\n", inst->name);
	}
	/*
	 * Add the namespace lookup here, this code can be shared by
	 * all precomputations.
	 */
	cur = inst->_private;
	if (cur != NULL) {
	    int i = 0;

	    cur->nsList = xmlGetNsList(inst->doc, inst);
            if (cur->nsList != NULL) {
		while (cur->nsList[i] != NULL)
		    i++;
	    }
	    cur->nsNr = i;
	}
    } else {
	if (IS_XSLT_NAME(inst, "document")) {
	    xsltDocumentComp(ctxt, inst);
	}
    }
}
