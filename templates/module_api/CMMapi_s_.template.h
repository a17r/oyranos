{% extends "Base_s_.h" %}

{% block LocalIncludeFiles %}
{{ block.super }}
#include "oyranos_module.h"
{% endblock %}

{% block ChildMembers %}
{{ block.super }}
/* Include "CMMapi.members.h" { */
{% include "CMMapi.members.h" %}
/* } Include "CMMapi.members.h" */
{% endblock %}
