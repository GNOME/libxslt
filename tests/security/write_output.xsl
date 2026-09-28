<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:param name="output" select="'output.xml'"/>
  <xsl:template match="/">
    <xsl:document href="{$output}">
      <result>written</result>
    </xsl:document>
    <done/>
  </xsl:template>
</xsl:stylesheet>
