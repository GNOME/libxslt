<xsl:stylesheet version="1.0"
	      xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output
 method="xml"
 indent="yes"
 encoding="iso-8859-1"
/>

<xsl:decimal-format
 name = "special"
 decimal-separator = "*"
/>

<xsl:template match="functions">
 <pi>
  one <xsl:value-of select="format-number(pi, 'prefix#,#,###.##suffix')"/>
  two <xsl:value-of select="format-number(negpi, '_#,#,###.##_')"/>
  three <xsl:value-of select="format-number(negpi, '_#,#,000.000##_')"/>
  four <xsl:value-of select="format-number(negpi, '_#.#_;_(#.#)_')"/>
  five <xsl:value-of select="format-number(pi, 'prefix#,#,###*##suffix','special')"/>
 </pi>
</xsl:template>

</xsl:stylesheet>
