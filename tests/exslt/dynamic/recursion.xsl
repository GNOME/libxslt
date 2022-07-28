<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
 xmlns:dyn="http://exslt.org/dynamic"
 exclude-result-prefixes="dyn"
>

<xsl:template match="/doc">
  <result>
    <xsl:apply-templates select="*"/>
  </result>
</xsl:template>

<xsl:template match="eval">
  <xsl:value-of select="dyn:evaluate(.)"/>
</xsl:template>

<xsl:template match="map">
  <xsl:value-of select="dyn:map(., .)"/>
</xsl:template>

</xsl:stylesheet>

