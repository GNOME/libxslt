<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:test="http://example.org/"
                version='1.0'>

<xsl:output cdata-section-elements="example"/>

<xsl:template match="/">
<example><![CDATA[<foo>]]></example>
</xsl:template>
</xsl:stylesheet>
