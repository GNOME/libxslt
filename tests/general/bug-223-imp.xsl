<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:x="urn:math"
                xmlns:func="http://exslt.org/functions"
                extension-element-prefixes="func">

    <func:function name="x:pow">
        <xsl:param name="number"/>
        <func:result select="$number * $number"/>
    </func:function>
</xsl:stylesheet>
