<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  xmlns:axsl="http://www.w3.org/1999/XSL/TransformAlias">

<xsl:variable name="image-dir">/images</xsl:variable>

<xsl:template match="photograph">
<img src="{$image-dir}/{href}" width="{size/@width}"/>
</xsl:template>
</xsl:stylesheet>
