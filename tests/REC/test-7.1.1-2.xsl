<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:doc="http://example.org/doc">

<xsl:template exclude-result-prefixes="doc" match="/">
<doc:tst>FAILURE</doc:tst>
<doc>SUCCESS</doc>
</xsl:template>

</xsl:stylesheet>
