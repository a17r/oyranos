{% extends "Base_s_.h" %}

{% block LocalIncludeFiles %}
{{ block.super }}
#include "oyFilterNode_s.h"
{% endblock %}

{% block ChildMembers %}
{{ block.super }}
/* Include "Conversion.members.h" { */
{% include "Conversion.members.h" %}
/* } Include "Conversion.members.h" */
{% endblock %}
