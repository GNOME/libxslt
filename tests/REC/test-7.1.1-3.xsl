<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:doc="http://example.org/doc"
  exclude-result-prefixes="doc" >

<xsl:template match="/">
<doc:tst>FAILURE</doc:tst>
<tst>SUCCESS</tst>
</xsl:template>

</xsl:stylesheet>
