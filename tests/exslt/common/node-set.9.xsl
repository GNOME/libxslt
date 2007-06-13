<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:import
href="http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl"/>
<xsl:template match="comment()">  <!-- pass through comments -->
 <xsl:comment><xsl:value-of select="."/></xsl:comment>
</xsl:template>
</xsl:stylesheet>
