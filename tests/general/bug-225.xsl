<?xml version="1.0" encoding="UTF-8"?>
<transform
  xmlns="http://www.w3.org/1999/XSL/Transform"
  xmlns:my="example:my"
  xmlns:exsl="http://exslt.org/common"
  xmlns:exslfunc="http://exslt.org/functions"
  xmlns:html="http://www.w3.org/1999/xhtml"
  extension-element-prefixes="exsl exslfunc"
  version="1.0"
>
  <exslfunc:function name="my:preprocessed">
    <!-- it doesn’t matter what this function returns: it could be an empty node-set -->
    <variable name="result-fragment"/>
    <exslfunc:result select="exsl:node-set($result-fragment)"/>
  </exslfunc:function>
  <variable name="source" select="/"/>
  <variable name="result-fragment">
    <!-- this redirection seems to be part of the problem -->
    <!-- this variable does not actually have to be used (but in real situations, it would be) -->
    <apply-templates select="$source/node()"/>
  </variable>
  <variable name="result" select="exsl:node-set($result-fragment)"/>
  <variable name="my:info" select="my:preprocessed()"/>
  <template match="html:div">
    <!-- this template is applied as part of calculating the value of $result-fragment -->
    <!-- the reference to $my:info does not actually need to be used (but in real situations, it would be) -->
    <variable name="form" select="$my:info"/>
    <copy>
      <copy-of select="@*|node()"/>
    </copy>
  </template>
</transform>
