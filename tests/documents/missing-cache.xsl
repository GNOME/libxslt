<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/doc">
    <out>
      <xsl:for-each select="item">
        <xsl:copy-of select="document('missing-cache-doc.xml')/*"/>
      </xsl:for-each>
    </out>
  </xsl:template>
</xsl:stylesheet>
