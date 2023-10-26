<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:x="urn:math">

    <xsl:import href="bug-223-imp.xsl"/>
    <xsl:include href="bug-223-inc.xsl"/>

    <xsl:output omit-xml-declaration="yes"/>

    <xsl:template match="/">
        <xsl:value-of select="x:pow(count(cd))"/>
        <xsl:value-of select="' '"/>
        <xsl:value-of select="x:sqrt(count(cd))"/>
    </xsl:template>

</xsl:stylesheet>
