<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:param name="uri" select="'xinclude_doc.xml'"/>
  <xsl:template match="/">
    <output>
      <xsl:copy-of select="document($uri)"/>
    </output>
  </xsl:template>
</xsl:stylesheet>
