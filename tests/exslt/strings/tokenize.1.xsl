<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:str="http://exslt.org/strings"
    exclude-result-prefixes="str">

<xsl:template match="/">
<out>;
  str:tokenize('2001-06-03T11:40:23', '-T:')
  <xsl:copy-of select="str:tokenize('2001-06-03T11:40:23', '-T:')"/>;

  str:tokenize('date math str')
  <xsl:copy-of select="str:tokenize('date math str')"/>;
</out>
</xsl:template>

</xsl:stylesheet>