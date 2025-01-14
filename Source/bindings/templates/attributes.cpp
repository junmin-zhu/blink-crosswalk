{##############################################################################}
{% macro attribute_getter(attribute, world_suffix) %}
{% filter conditional(attribute.conditional_string) %}
static void {{attribute.name}}AttributeGetter{{world_suffix}}(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    {% if attribute.is_unforgeable %}
    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain({{v8_class_name}}::GetTemplate(info.GetIsolate(), worldType(info.GetIsolate())));
    if (holder.IsEmpty())
        return;
    {{cpp_class_name}}* imp = {{v8_class_name}}::toNative(holder);
    {% endif %}
    {% if attribute.cached_attribute_validation_method %}
    v8::Handle<v8::String> propertyName = v8::String::NewSymbol("{{attribute.name}}");
    {{cpp_class_name}}* imp = {{v8_class_name}}::toNative(info.Holder());
    if (!imp->{{attribute.cached_attribute_validation_method}}()) {
        v8::Handle<v8::Value> jsValue = info.Holder()->GetHiddenValue(propertyName);
        if (!jsValue.IsEmpty()) {
            v8SetReturnValue(info, jsValue);
            return;
        }
    }
    {% elif not (attribute.is_static or attribute.is_unforgeable) %}
    {{cpp_class_name}}* imp = {{v8_class_name}}::toNative(info.Holder());
    {% endif %}
    {% if attribute.is_call_with_script_execution_context %}
    ExecutionContext* scriptContext = getExecutionContext();
    {% endif %}
    {# Special cases #}
    {% if attribute.is_check_security_for_node %}
    {# FIXME: consider using a local variable to not call getter twice #}
    ExceptionState es(info.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToNode({{attribute.cpp_value}}, es)) {
        v8SetReturnValueNull(info);
        es.throwIfNeeded();
        return;
    }
    {% endif %}
    {% if attribute.is_getter_raises_exception %}
    ExceptionState es(info.GetIsolate());
    {{attribute.cpp_type}} {{attribute.cpp_value}} = {{attribute.cpp_value_original}};
    if (UNLIKELY(es.throwIfNeeded()))
        return;
    {% endif %}
    {% if attribute.is_nullable %}
    bool isNull = false;
    {{attribute.cpp_type}} {{attribute.cpp_value}} = {{attribute.cpp_value_original}};
    if (isNull) {
        v8SetReturnValueNull(info);
        return;
    }
    {% elif attribute.idl_type == 'EventHandler' or
            attribute.cached_attribute_validation_method %}
    {# FIXME: consider merging all these assign to local variable statements #}
    {{attribute.cpp_type}} {{attribute.cpp_value}} = {{attribute.cpp_value_original}};
    {% endif %}
    {% if attribute.cached_attribute_validation_method %}
    info.Holder()->SetHiddenValue(propertyName, {{attribute.cpp_value}}.v8Value());
    {% endif %}
    {# End special cases #}
    {% if attribute.is_keep_alive_for_gc %}
    {{attribute.cpp_type}} result = {{attribute.cpp_value}};
    if (result && DOMDataStore::setReturnValueFromWrapper<{{attribute.v8_type}}>(info.GetReturnValue(), result.get()))
        return;
    v8::Handle<v8::Value> wrapper = toV8(result.get(), info.Holder(), info.GetIsolate());
    if (!wrapper.IsEmpty()) {
        V8HiddenPropertyName::setNamedHiddenReference(info.Holder(), "{{attribute.name}}", wrapper);
        {{attribute.v8_set_return_value}};
    }
    {% else %}
    {{attribute.v8_set_return_value}};
    {% endif %}
}
{% endfilter %}
{% endmacro %}


{##############################################################################}
{% macro attribute_getter_callback(attribute, world_suffix) %}
{% filter conditional(attribute.conditional_string) %}
static void {{attribute.name}}AttributeGetterCallback{{world_suffix}}(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    {% if attribute.deprecate_as %}
    UseCounter::countDeprecation(activeExecutionContext(), UseCounter::{{attribute.deprecate_as}});
    {% endif %}
    {% if attribute.measure_as %}
    UseCounter::count(activeDOMWindow(), UseCounter::{{attribute.measure_as}});
    {% endif %}
    {% if world_suffix in attribute.activity_logging_world_list_for_getter %}
    V8PerContextData* contextData = V8PerContextData::from(info.GetIsolate()->GetCurrentContext());
    if (contextData && contextData->activityLogger())
        contextData->activityLogger()->log("{{interface_name}}.{{attribute.name}}", 0, 0, "Getter");
    {% endif %}
    {% if attribute.has_custom_getter %}
    {{v8_class_name}}::{{attribute.name}}AttributeGetterCustom(info);
    {% else %}
    {{cpp_class_name}}V8Internal::{{attribute.name}}AttributeGetter{{world_suffix}}(info);
    {% endif %}
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
{% endfilter %}
{% endmacro %}


{##############################################################################}
{% macro attribute_setter(attribute, world_suffix) %}
{% filter conditional(attribute.conditional_string) %}
static void {{attribute.name}}AttributeSetter{{world_suffix}}(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    {% if attribute.has_strict_type_checking %}
    {# Type checking for interface types (if interface not implemented, throw
       TypeError), per http://www.w3.org/TR/WebIDL/#es-interface #}
    if (!isUndefinedOrNull(jsValue) && !V8{{attribute.idl_type}}::HasInstance(jsValue, info.GetIsolate(), worldType(info.GetIsolate()))) {
        throwTypeError(ExceptionMessages::failedToSet("{{attribute.name}}", "{{interface_name}}", "The provided value is not of type '{{attribute.idl_type}}'."), info.GetIsolate());
        return;
    }
    {% endif %}
    {% if not attribute.is_static %}
    {{cpp_class_name}}* imp = {{v8_class_name}}::toNative(info.Holder());
    {% endif %}
    {% if attribute.idl_type == 'EventHandler' and interface_name == 'Window' %}
    if (!imp->document())
        return;
    {% endif %}
    {% if attribute.idl_type != 'EventHandler' %}
    {{attribute.v8_value_to_local_cpp_value}};
    {% else %}{# EventHandler hack #}
    transferHiddenDependency(info.Holder(), {{attribute.event_handler_getter_expression}}, jsValue, {{v8_class_name}}::eventListenerCacheIndex, info.GetIsolate());
    {% endif %}
    {% if attribute.enum_validation_expression %}
    {# Setter ignores invalid enum values: http://www.w3.org/TR/WebIDL/#idl-enums #}
    String string = cppValue;
    if (!({{attribute.enum_validation_expression}}))
        return;
    {% endif %}
    {% if attribute.is_reflect %}
    CustomElementCallbackDispatcher::CallbackDeliveryScope deliveryScope;
    {% endif %}
    {% if attribute.is_setter_raises_exception %}
    ExceptionState es(info.GetIsolate());
    {% endif %}
    {% if attribute.is_call_with_script_execution_context %}
    ExecutionContext* scriptContext = getExecutionContext();
    {% endif %}
    {{attribute.cpp_setter}};
    {% if attribute.is_setter_raises_exception %}
    es.throwIfNeeded();
    {% endif %}
    {% if attribute.cached_attribute_validation_method %}
    info.Holder()->DeleteHiddenValue(v8::String::NewSymbol("{{attribute.name}}")); // Invalidate the cached value.
    {% endif %}
}
{% endfilter %}
{% endmacro %}


{##############################################################################}
{% macro attribute_setter_callback(attribute, world_suffix) %}
{% filter conditional(attribute.conditional_string) %}
static void {{attribute.name}}AttributeSetterCallback{{world_suffix}}(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    {% if attribute.deprecate_as %}
    UseCounter::countDeprecation(activeExecutionContext(), UseCounter::{{attribute.deprecate_as}});
    {% endif %}
    {% if attribute.measure_as %}
    UseCounter::count(activeDOMWindow(), UseCounter::{{attribute.measure_as}});
    {% endif %}
    {% if world_suffix in attribute.activity_logging_world_list_for_setter %}
    V8PerContextData* contextData = V8PerContextData::from(info.GetIsolate()->GetCurrentContext());
    if (contextData && contextData->activityLogger()) {
        v8::Handle<v8::Value> loggerArg[] = { jsValue };
        contextData->activityLogger()->log("{{interface_name}}.{{attribute.name}}", 1, &loggerArg[0], "Setter");
    }
    {% endif %}
    {% if attribute.is_reflect %}
    CustomElementCallbackDispatcher::CallbackDeliveryScope deliveryScope;
    {% endif %}
    {% if attribute.has_custom_setter %}
    {{v8_class_name}}::{{attribute.name}}AttributeSetterCustom(jsValue, info);
    {% else %}
    {{cpp_class_name}}V8Internal::{{attribute.name}}AttributeSetter{{world_suffix}}(jsValue, info);
    {% endif %}
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
{% endfilter %}
{% endmacro %}
