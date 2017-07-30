<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:f="http://exslt.org/functions"
  extension-element-prefixes="f">
  <f:function name="f:f">
    <xsl:param name="n"/>
    <xsl:for-each select="namespace::*">
      <xsl:sort/>
    </xsl:for-each>
    <xsl:choose>
        <xsl:when test="$n > 0">
            <f:result select="f:f($n - 1)"/>
        </xsl:when>
        <xsl:otherwise>
            <f:result select="1"/>
        </xsl:otherwise>
    </xsl:choose>
  </f:function>
  <xsl:template match="/*">
    <xsl:value-of select="f:f(4)"/>
  </xsl:template>
</xsl:stylesheet>
