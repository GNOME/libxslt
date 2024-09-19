<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:variable name="v1" select="generate-id(doc/item[1])"/>
<xsl:variable name="v2" select="generate-id(doc/item[2])"/>
<xsl:variable name="v3" select="generate-id(doc/item[3])"/>
<xsl:variable name="v4" select="generate-id(doc/item[4])"/>
<xsl:variable name="v5" select="generate-id(doc/item[5])"/>
<xsl:variable name="v6" select="generate-id(doc/item[6])"/>

<xsl:template match="/">
    <result>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$v1"/>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$v2"/>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$v3"/>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$v4"/>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$v5"/>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$v6"/>
        <xsl:text>&#xa;</xsl:text>
    </result>
</xsl:template>

</xsl:stylesheet>
