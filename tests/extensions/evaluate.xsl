<?xml version='1.0'?>
<xsl:stylesheet 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:libxslt="http://xmlsoft.org/XSLT/namespace"
  version='1.0'>

<xsl:variable name="expression" select="libxslt:expression('doc/two')"/>
  
  <xsl:template match="/">
    <xsl:variable name="string">doc/one</xsl:variable>
    <xsl:value-of select="libxslt:evaluate($string)"/>
    <xsl:value-of select="count(libxslt:evaluate('/doc/one')/../*)"/>
    <xsl:value-of select="libxslt:evaluate(/doc/three)"/>
    <xsl:value-of select="libxslt:eval($expression)"/>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="four">
    <xsl:variable name="string">doc/one</xsl:variable>
    <xsl:value-of select="libxslt:evaluate($string)"/>
    <xsl:value-of select="libxslt:eval($expression)"/>
  </xsl:template>
  
  <xsl:template match="text()"/>

</xsl:stylesheet>
